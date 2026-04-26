#include "streaming/webrtc/WebRtcServer.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <optional>
#include <pthread.h>
#include <sched.h>
#include <thread>

#include <gst/app/gstappsrc.h>
#include <gst/sdp/sdp.h>
#include <gst/webrtc/webrtc.h>
#include <nlohmann/json.hpp>

#include "streaming/webrtc/FrameSource.hpp"
#include "streaming/webrtc/PipelineBuilder.hpp"
#include "streaming/webrtc/SignallingHandler.hpp"
#include "streaming/webrtc/types/PoseUpdate.hpp"

namespace edgevision::streaming::webrtc {

    using nlohmann::json;

    namespace {

        constexpr auto kDataChannelLabel = "control";

        // Simple pose mailbox. Pose-pump thread reads the latest pose for
        // each TSDF render; client may send poses faster or slower than
        // 30 Hz — we always use the most recent one.
        class PoseMailbox {
          public:
            void set(const PoseUpdate& p) {
                std::lock_guard<std::mutex> g(m_mu);
                m_value = p;
                m_present = true;
            }
            std::optional<PoseUpdate> get() const {
                std::lock_guard<std::mutex> g(m_mu);
                if (!m_present) {
                    return std::nullopt;
                }
                return m_value;
            }

          private:
            mutable std::mutex m_mu;
            PoseUpdate m_value;
            bool m_present = false;
        };

        std::optional<PoseUpdate> parsePose(const std::string& text) {
            try {
                auto j = json::parse(text);
                if (j.value("type", "") != "pose") {
                    return std::nullopt;
                }
                PoseUpdate p;
                p.seq = j.value("seq", 0u);
                const auto m = j.at("matrix");
                if (!m.is_array() || m.size() != 16) {
                    return std::nullopt;
                }
                for (std::size_t i = 0; i < 16; ++i) {
                    p.matrix[i] = m[i].get<float>();
                }
                p.width = j.value("width", 854u);
                p.height = j.value("height", 480u);
                p.fovX = j.value("fov_x", 1.0472f);
                p.fovY = j.value("fov_y", 0.6435f);
                return p;
            } catch (...) {
                return std::nullopt;
            }
        }

        ViewMode parseViewMode(const std::string& s) {
            if (s == "rgb") return ViewMode::Rgb;
            if (s == "tsdf") return ViewMode::Tsdf;
            return ViewMode::Dual;
        }

        void pinToCore(std::thread& t, int core) {
#ifdef __linux__
            if (core < 0) { return; }
            cpu_set_t mask;
            CPU_ZERO(&mask);
            CPU_SET(core, &mask);
            if (pthread_setaffinity_np(t.native_handle(), sizeof(mask), &mask) != 0) {
                std::cerr << "pinToCore(" << core << ") failed\n";
            }
#else
            (void)t; (void)core;
#endif
        }

    } // namespace

    struct WebRtcServer::Impl {
        Impl(
            StreamConfig config,
            edgevision::model::frame::FrameStore& frameStore,
            edgevision::model::scene::SharedScene& scene
        )
            : m_config(config),
              m_rgbSource(frameStore),
              m_tsdfSource(scene, nullptr) {}

        ~Impl() { stop(); }

        void start() {
            if (m_running.exchange(true)) {
                return;
            }
            gst_init(nullptr, nullptr);

            SignallingCallbacks cb;
            cb.onOffer = [this](const std::string& sdp, SendFunc reply) {
                std::lock_guard<std::mutex> g(m_pcMu);
                onOffer(sdp, std::move(reply));
            };
            cb.onIce = [this](const json& candidate) { onIceFromClient(candidate); };
            cb.onClientConnected = [] {};
            cb.onClientDisconnected = [this] { onClientDisconnected(); };

            m_signalling = std::make_unique<SignallingHandler>(
                m_config.signallingHost, m_config.signallingPort, std::move(cb)
            );
            m_signalling->start();

            if (m_config.enableRgb) {
                m_pumpRgb = std::thread([this] { pumpRgb(); });
                pinToCore(m_pumpRgb, m_config.pumpRgbCore);
            }
            m_pumpTsdf = std::thread([this] { pumpTsdf(); });
            pinToCore(m_pumpTsdf, m_config.pumpTsdfCore);
        }

        void stop() {
            if (!m_running.exchange(false)) {
                return;
            }
            if (m_signalling) {
                m_signalling->stop();
            }
            if (m_pumpRgb.joinable()) m_pumpRgb.join();
            if (m_pumpTsdf.joinable()) m_pumpTsdf.join();
            tearDownPipeline();
        }

        void setTsdfRenderer(
            std::function<bool(const PoseUpdate&, std::vector<std::uint8_t>&)> renderer
        ) {
            m_tsdfSource.setRenderer(std::move(renderer));
        }

      private:
        void onOffer(const std::string& sdp, SendFunc reply) {
            tearDownPipeline();
            m_handles = buildPipeline(m_config);
            if (m_handles.pipeline == nullptr) {
                std::cerr << "buildPipeline failed\n";
                return;
            }
            m_replyToClient = std::move(reply);

            // Wire local-ICE → reply.
            g_signal_connect(
                m_handles.webrtcbin, "on-ice-candidate",
                G_CALLBACK(&Impl::onLocalIceTrampoline), this
            );
            // Negotiation needed → setLocalDescription path is taken when
            // the remote-offer is set; we don't need to call create-offer
            // ourselves because we are the answerer.
            g_signal_connect(
                m_handles.webrtcbin, "on-data-channel",
                G_CALLBACK(&Impl::onDataChannelTrampoline), this
            );

            // Set REMOTE description (the client's offer).
            applyRemoteDescription(sdp, GST_WEBRTC_SDP_TYPE_OFFER);

            // Build local answer.
            GstPromise* promise = gst_promise_new_with_change_func(
                &Impl::onAnswerCreated, this, nullptr
            );
            g_signal_emit_by_name(m_handles.webrtcbin, "create-answer", nullptr, promise);

            gst_element_set_state(m_handles.pipeline, GST_STATE_PLAYING);
        }

        static void onAnswerCreated(GstPromise* promise, gpointer userData) {
            auto* self = static_cast<Impl*>(userData);
            const GstStructure* reply = gst_promise_get_reply(promise);
            GstWebRTCSessionDescription* answer = nullptr;
            gst_structure_get(reply, "answer",
                              GST_TYPE_WEBRTC_SESSION_DESCRIPTION,
                              &answer, nullptr);
            gst_promise_unref(promise);
            if (answer == nullptr) {
                std::cerr << "create-answer returned no answer\n";
                return;
            }

            GstPromise* setPromise = gst_promise_new();
            g_signal_emit_by_name(self->m_handles.webrtcbin,
                                  "set-local-description", answer, setPromise);
            gst_promise_interrupt(setPromise);
            gst_promise_unref(setPromise);

            gchar* sdpStr = gst_sdp_message_as_text(answer->sdp);
            json reply2 = {{"type", "answer"}, {"sdp", sdpStr}};
            if (self->m_replyToClient) {
                self->m_replyToClient(reply2.dump());
            }
            g_free(sdpStr);
            gst_webrtc_session_description_free(answer);
        }

        void applyRemoteDescription(
            const std::string& sdp, GstWebRTCSDPType type
        ) {
            GstSDPMessage* msg = nullptr;
            gst_sdp_message_new_from_text(sdp.c_str(), &msg);
            auto* desc = gst_webrtc_session_description_new(type, msg);
            GstPromise* promise = gst_promise_new();
            g_signal_emit_by_name(m_handles.webrtcbin,
                                  "set-remote-description", desc, promise);
            gst_promise_interrupt(promise);
            gst_promise_unref(promise);
            gst_webrtc_session_description_free(desc);
        }

        static void onLocalIceTrampoline(
            GstElement* /*src*/, guint mlineindex, gchar* candidate,
            gpointer userData
        ) {
            auto* self = static_cast<Impl*>(userData);
            json msg = {
                {"type", "ice"},
                {"candidate", candidate},
                {"sdpMLineIndex", static_cast<int>(mlineindex)},
                {"sdpMid", std::to_string(mlineindex)},
            };
            if (self->m_replyToClient) {
                self->m_replyToClient(msg.dump());
            }
        }

        void onIceFromClient(const json& candidate) {
            const std::string c = candidate.value("candidate", "");
            const std::optional<int> mlineIdx =
                candidate.contains("sdpMLineIndex")
                    ? std::optional<int>(candidate.at("sdpMLineIndex").get<int>())
                    : std::nullopt;
            if (c.empty() || !mlineIdx.has_value()) {
                return;
            }
            std::lock_guard<std::mutex> g(m_pcMu);
            if (m_handles.webrtcbin == nullptr) {
                return;
            }
            g_signal_emit_by_name(m_handles.webrtcbin, "add-ice-candidate",
                                  static_cast<guint>(*mlineIdx), c.c_str());
        }

        static void onDataChannelTrampoline(
            GstElement* /*src*/, GObject* channel, gpointer userData
        ) {
            auto* self = static_cast<Impl*>(userData);
            self->m_dataChannel = channel;
            g_signal_connect(channel, "on-message-string",
                             G_CALLBACK(&Impl::onMessageStringTrampoline), self);
            g_signal_connect(channel, "on-open",
                             G_CALLBACK(&Impl::onChannelOpenTrampoline), self);
        }

        static void onChannelOpenTrampoline(GObject* /*ch*/, gpointer userData) {
            auto* self = static_cast<Impl*>(userData);
            self->sendControlText(json{
                {"type", "hello"},
                {"from", "edgevision-jetson"},
            }.dump());
            self->sendControlText(json{
                {"type", "track_info"},
                {"tracks", json::array({
                    json{{"mid", "0"}, {"kind", "rgb"}},
                    json{{"mid", "1"}, {"kind", "tsdf"}},
                })},
            }.dump());
        }

        static void onMessageStringTrampoline(
            GObject* /*ch*/, gchar* text, gpointer userData
        ) {
            auto* self = static_cast<Impl*>(userData);
            const std::string s(text == nullptr ? "" : text);
            if (auto p = parsePose(s); p.has_value()) {
                self->m_pose.set(*p);
                return;
            }
            try {
                auto j = json::parse(s);
                const std::string t = j.value("type", "");
                if (t == "view_mode") {
                    self->m_viewMode = parseViewMode(j.value("mode", "dual"));
                } else if (t == "reset_origin") {
                    // No server-side state to reset for now; client-side only.
                }
            } catch (...) {
                // ignore non-JSON
            }
        }

        void sendControlText(const std::string& text) {
            if (m_dataChannel == nullptr) {
                return;
            }
            g_signal_emit_by_name(m_dataChannel, "send-string", text.c_str());
        }

        void onClientDisconnected() {
            std::lock_guard<std::mutex> g(m_pcMu);
            tearDownPipeline();
        }

        void tearDownPipeline() {
            m_replyToClient = nullptr;
            m_dataChannel = nullptr;
            if (m_handles.pipeline != nullptr) {
                gst_element_set_state(m_handles.pipeline, GST_STATE_NULL);
                if (m_handles.webrtcbin) gst_object_unref(m_handles.webrtcbin);
                if (m_handles.rgbAppSrc) gst_object_unref(m_handles.rgbAppSrc);
                if (m_handles.tsdfAppSrc) gst_object_unref(m_handles.tsdfAppSrc);
                gst_object_unref(m_handles.pipeline);
                m_handles = {};
            }
        }

        // ---- frame pumps ------------------------------------------------

        void pumpRgb() {
            const auto frameDuration = std::chrono::milliseconds(1000 / m_config.fps);
            std::vector<std::uint8_t> buffer;
            while (m_running.load()) {
                const auto t0 = std::chrono::steady_clock::now();
                {
                    std::lock_guard<std::mutex> g(m_pcMu);
                    if (m_handles.rgbAppSrc != nullptr &&
                        m_viewMode.load() != ViewMode::Tsdf &&
                        m_rgbSource.pull(m_config.width, m_config.height, buffer)) {
                        pushBuffer(m_handles.rgbAppSrc, buffer);
                    }
                }
                std::this_thread::sleep_until(t0 + frameDuration);
            }
        }

        void pumpTsdf() {
            const auto frameDuration = std::chrono::milliseconds(1000 / m_config.fps);
            std::vector<std::uint8_t> buffer;
            while (m_running.load()) {
                const auto t0 = std::chrono::steady_clock::now();
                // Ref the appsrc under lock so tearDownPipeline can't free it
                // while we hold a live pointer into render() below.
                GstElement* appsrc = nullptr;
                {
                    std::lock_guard<std::mutex> g(m_pcMu);
                    if (m_handles.tsdfAppSrc != nullptr &&
                        m_viewMode.load() != ViewMode::Rgb) {
                        appsrc = GST_ELEMENT(gst_object_ref(m_handles.tsdfAppSrc));
                    }
                }
                if (appsrc != nullptr) {
                    auto pose = m_pose.get();
                    if (pose.has_value() && m_tsdfSource.render(*pose, buffer)) {
                        pushBuffer(appsrc, buffer);
                    }
                    gst_object_unref(appsrc);
                }
                std::this_thread::sleep_until(t0 + frameDuration);
            }
        }

        void pushBuffer(GstElement* appsrc, const std::vector<std::uint8_t>& bgr) {
            const gsize size = bgr.size();
            GstBuffer* buf = gst_buffer_new_allocate(nullptr, size, nullptr);
            GstMapInfo info{};
            gst_buffer_map(buf, &info, GST_MAP_WRITE);
            std::memcpy(info.data, bgr.data(), size);
            gst_buffer_unmap(buf, &info);

            GstFlowReturn ret = GST_FLOW_OK;
            g_signal_emit_by_name(appsrc, "push-buffer", buf, &ret);
            gst_buffer_unref(buf);
            if (ret != GST_FLOW_OK) {
                std::cerr << "push-buffer returned " << ret << std::endl;
            }
        }

        StreamConfig m_config;
        RgbFrameSource m_rgbSource;
        TsdfFrameSource m_tsdfSource;
        std::unique_ptr<SignallingHandler> m_signalling;
        std::atomic<bool> m_running{false};
        std::thread m_pumpRgb;
        std::thread m_pumpTsdf;
        std::mutex m_pcMu;
        PipelineHandles m_handles{};
        SendFunc m_replyToClient{};
        GObject* m_dataChannel = nullptr;
        PoseMailbox m_pose;
        std::atomic<ViewMode> m_viewMode{ViewMode::Dual};
    };

    WebRtcServer::WebRtcServer(
        StreamConfig config,
        edgevision::model::frame::FrameStore& frameStore,
        edgevision::model::scene::SharedScene& scene
    )
        : m_impl(std::make_unique<Impl>(config, frameStore, scene)) {}

    WebRtcServer::~WebRtcServer() = default;
    void WebRtcServer::start() { m_impl->start(); }
    void WebRtcServer::stop() { m_impl->stop(); }
    void WebRtcServer::setTsdfRenderer(
        std::function<bool(const PoseUpdate&, std::vector<std::uint8_t>&)> renderer
    ) {
        m_impl->setTsdfRenderer(std::move(renderer));
    }

    std::unique_ptr<WebRtcServer> startWebRtcServer(
        StreamConfig config,
        edgevision::model::frame::FrameStore& frameStore,
        edgevision::model::scene::SharedScene& scene
    ) {
        auto srv = std::make_unique<WebRtcServer>(config, frameStore, scene);
        srv->start();
        return srv;
    }

} // namespace edgevision::streaming::webrtc
