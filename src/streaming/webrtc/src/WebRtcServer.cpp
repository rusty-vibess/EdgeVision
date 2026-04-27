#include "streaming/webrtc/WebRtcServer.hpp"

#include <atomic>
#include <chrono>
#include <cstring>
#include <gst/app/gstappsrc.h>
#include <gst/sdp/sdp.h>
#include <gst/webrtc/webrtc.h>
#include <iostream>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <pthread.h>
#include <sched.h>
#include <thread>
#include <vector>

#include "model/viewer/RenderOutputStore.hpp"
#include "model/viewer/ViewerPoseStore.hpp"
#include "streaming/webrtc/PipelineBuilder.hpp"
#include "streaming/webrtc/SignallingHandler.hpp"
#include "streaming/webrtc/utils/PoseUpdateHandler.hpp"

namespace edgevision::streaming::webrtc {

    using nlohmann::json;

    namespace {

        void pinToCoreMask(std::thread& thread, const std::vector<int>& cores) {
#ifdef __linux__
            if (cores.empty()) {
                return;
            }

            cpu_set_t mask;
            CPU_ZERO(&mask);
            for (const int core : cores) {
                if (core >= 0) {
                    CPU_SET(core, &mask);
                }
            }

            if (pthread_setaffinity_np(thread.native_handle(), sizeof(mask), &mask) != 0) {
                std::cerr << "pinToCoreMask failed\n";
            }
#else
            (void)thread;
            (void)cores;
#endif
        }

        [[nodiscard]] bool copyRenderOutputRgb(
            const edgevision::model::viewer::RenderOutput& output,
            std::vector<std::uint8_t>& rgb
        ) {
            if (!output.hasRgb() || output.imageSize.width <= 0 || output.imageSize.height <= 0) {
                return false;
            }

            const std::size_t expectedByteCount =
                static_cast<std::size_t>(output.imageSize.width) * output.imageSize.height * 3;
            if (output.rgb.byteCount != expectedByteCount) {
                return false;
            }

            const auto* begin = reinterpret_cast<const std::uint8_t*>(output.rgb.data);
            rgb.assign(begin, begin + output.rgb.byteCount);
            return true;
        }

    } // namespace

    struct WebRtcServer::Impl {
        Impl(
            edgevision::config::WebRtcStreamingConfig config,
            edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
            edgevision::model::viewer::RenderOutputStore& renderOutputStore
        )
            : m_config(std::move(config)),
              m_viewerPoseStore(viewerPoseStore),
              m_renderOutputStore(renderOutputStore) {}

        ~Impl() {
            stop();
        }

        void start() {
            if (m_running.exchange(true)) {
                return;
            }

            gst_init(nullptr, nullptr);

            SignallingCallbacks callbacks;
            callbacks.onOffer = [this](const std::string& sdp, SendFunc reply) {
                std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
                onOffer(sdp, std::move(reply));
            };
            callbacks.onIce = [this](const json& candidate) { onIceFromClient(candidate); };
            callbacks.onClientConnected = [] {};
            callbacks.onClientDisconnected = [this] { onClientDisconnected(); };

            m_signalling = std::make_unique<SignallingHandler>(
                m_config.signallingHost, m_config.signallingPort, std::move(callbacks)
            );
            m_signalling->start();

            m_videoPump = std::thread([this] { pumpVideo(); });
            pinToCoreMask(m_videoPump, m_config.pumpCoreMask);
        }

        void stop() {
            if (!m_running.exchange(false)) {
                return;
            }

            if (m_signalling) {
                m_signalling->stop();
            }

            if (m_videoPump.joinable()) {
                m_videoPump.join();
            }

            tearDownPipeline();
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
            g_signal_connect(
                m_handles.webrtcbin,
                "on-ice-candidate",
                G_CALLBACK(&Impl::onLocalIceTrampoline),
                this
            );
            g_signal_connect(
                m_handles.webrtcbin,
                "on-data-channel",
                G_CALLBACK(&Impl::onDataChannelTrampoline),
                this
            );

            applyRemoteDescription(sdp, GST_WEBRTC_SDP_TYPE_OFFER);

            GstPromise* promise =
                gst_promise_new_with_change_func(&Impl::onAnswerCreated, this, nullptr);
            g_signal_emit_by_name(m_handles.webrtcbin, "create-answer", nullptr, promise);

            gst_element_set_state(m_handles.pipeline, GST_STATE_PLAYING);
        }

        static void onAnswerCreated(GstPromise* promise, gpointer userData) {
            auto* self = static_cast<Impl*>(userData);
            const GstStructure* reply = gst_promise_get_reply(promise);
            GstWebRTCSessionDescription* answer = nullptr;
            gst_structure_get(
                reply, "answer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &answer, nullptr
            );
            gst_promise_unref(promise);
            if (answer == nullptr) {
                std::cerr << "create-answer returned no answer\n";
                return;
            }

            GstPromise* setPromise = gst_promise_new();
            g_signal_emit_by_name(
                self->m_handles.webrtcbin, "set-local-description", answer, setPromise
            );
            gst_promise_interrupt(setPromise);
            gst_promise_unref(setPromise);

            gchar* sdpText = gst_sdp_message_as_text(answer->sdp);
            const json response = {{"type", "answer"}, {"sdp", sdpText}};
            if (self->m_replyToClient) {
                self->m_replyToClient(response.dump());
            }
            g_free(sdpText);
            gst_webrtc_session_description_free(answer);
        }

        void applyRemoteDescription(const std::string& sdp, GstWebRTCSDPType type) {
            GstSDPMessage* message = nullptr;
            gst_sdp_message_new_from_text(sdp.c_str(), &message);
            auto* description = gst_webrtc_session_description_new(type, message);
            GstPromise* promise = gst_promise_new();
            g_signal_emit_by_name(
                m_handles.webrtcbin, "set-remote-description", description, promise
            );
            gst_promise_interrupt(promise);
            gst_promise_unref(promise);
            gst_webrtc_session_description_free(description);
        }

        static void onLocalIceTrampoline(
            GstElement*,
            guint mediaLineIndex,
            gchar* candidate,
            gpointer userData
        ) {
            auto* self = static_cast<Impl*>(userData);
            const json message = {
                {"type", "ice"},
                {"candidate", candidate},
                {"sdpMLineIndex", static_cast<int>(mediaLineIndex)},
                {"sdpMid", std::to_string(mediaLineIndex)},
            };
            if (self->m_replyToClient) {
                self->m_replyToClient(message.dump());
            }
        }

        void onIceFromClient(const json& candidate) {
            const std::string candidateValue = candidate.value("candidate", "");
            const std::optional<int> mediaLineIndex = candidate.contains("sdpMLineIndex")
                ? std::optional<int>(candidate.at("sdpMLineIndex").get<int>())
                : std::nullopt;
            if (candidateValue.empty() || !mediaLineIndex.has_value()) {
                return;
            }

            std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
            if (m_handles.webrtcbin == nullptr) {
                return;
            }

            g_signal_emit_by_name(
                m_handles.webrtcbin,
                "add-ice-candidate",
                static_cast<guint>(*mediaLineIndex),
                candidateValue.c_str()
            );
        }

        static void onDataChannelTrampoline(GstElement*, GObject* channel, gpointer userData) {
            auto* self = static_cast<Impl*>(userData);
            self->m_dataChannel = channel;
            g_signal_connect(
                channel, "on-message-string", G_CALLBACK(&Impl::onMessageStringTrampoline), self
            );
            g_signal_connect(channel, "on-open", G_CALLBACK(&Impl::onChannelOpenTrampoline), self);
        }

        static void onChannelOpenTrampoline(GObject*, gpointer userData) {
            auto* self = static_cast<Impl*>(userData);
            self->sendControlText(
                json{
                    {"type", "hello"},
                    {"from", "edgevision-jetson"},
                }
                    .dump()
            );
            self->sendControlText(
                json{
                    {"type", "track_info"},
                    {"tracks", json::array({json{{"mid", "0"}, {"kind", "render_output"}}})},
                }
                    .dump()
            );
        }

        static void onMessageStringTrampoline(GObject*, gchar* text, gpointer userData) {
            auto* self = static_cast<Impl*>(userData);
            const std::string payload(text == nullptr ? "" : text);
            if (const auto pose = utils::parsePoseUpdateMessage(payload); pose.has_value()) {
                self->applyPoseUpdate(*pose);
            }
        }

        void applyPoseUpdate(const PoseUpdate& pose) {
            if (const auto nextPose =
                    utils::makeUpdatedViewerPose(m_viewerPoseStore.latest(), pose);
                nextPose.has_value()) {
                static_cast<void>(m_viewerPoseStore.update(*nextPose));
            }
        }

        void sendControlText(const std::string& text) {
            if (m_dataChannel == nullptr) {
                return;
            }

            g_signal_emit_by_name(m_dataChannel, "send-string", text.c_str());
        }

        void onClientDisconnected() {
            std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
            tearDownPipeline();
        }

        void tearDownPipeline() {
            m_replyToClient = nullptr;
            m_dataChannel = nullptr;
            if (m_handles.pipeline == nullptr) {
                return;
            }

            gst_element_set_state(m_handles.pipeline, GST_STATE_NULL);
            if (m_handles.webrtcbin != nullptr) {
                gst_object_unref(m_handles.webrtcbin);
            }
            if (m_handles.videoAppSrc != nullptr) {
                gst_object_unref(m_handles.videoAppSrc);
            }
            gst_object_unref(m_handles.pipeline);
            m_handles = {};
        }

        void pumpVideo() {
            const auto frameDuration =
                std::chrono::milliseconds(1000 / std::max(m_config.fps, 1u));
            std::vector<std::uint8_t> rgb;
            while (m_running.load()) {
                const auto loopStart = std::chrono::steady_clock::now();
                GstElement* appSrc = nullptr;
                {
                    std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
                    if (m_handles.videoAppSrc != nullptr) {
                        appSrc = GST_ELEMENT(gst_object_ref(m_handles.videoAppSrc));
                    }
                }

                if (appSrc != nullptr) {
                    if (const auto output = m_renderOutputStore.latest(); output.has_value()) {
                        pushRenderOutput(appSrc, *output, rgb);
                    }
                    gst_object_unref(appSrc);
                }

                std::this_thread::sleep_until(loopStart + frameDuration);
            }
        }

        void pushRenderOutput(
            GstElement* appSrc,
            const edgevision::model::viewer::RenderOutput& output,
            std::vector<std::uint8_t>& rgb
        ) {
            if (output.imageSize.width != static_cast<int>(m_config.width)
                || output.imageSize.height != static_cast<int>(m_config.height)) {
                if (!m_warnedFrameShapeMismatch) {
                    std::cerr << "WebRTC output size mismatch: expected " << m_config.width << 'x'
                              << m_config.height << " but received " << output.imageSize.width
                              << 'x' << output.imageSize.height << '\n';
                    m_warnedFrameShapeMismatch = true;
                }
                return;
            }

            if (!copyRenderOutputRgb(output, rgb)) {
                return;
            }

            pushBuffer(appSrc, rgb);
        }

        void pushBuffer(GstElement* appSrc, const std::vector<std::uint8_t>& rgb) {
            if (rgb.empty()) {
                return;
            }

            GstBuffer* buffer = gst_buffer_new_allocate(nullptr, rgb.size(), nullptr);
            GstMapInfo info{};
            gst_buffer_map(buffer, &info, GST_MAP_WRITE);
            std::memcpy(info.data, rgb.data(), rgb.size());
            gst_buffer_unmap(buffer, &info);

            GstFlowReturn flowReturn = GST_FLOW_OK;
            g_signal_emit_by_name(appSrc, "push-buffer", buffer, &flowReturn);
            gst_buffer_unref(buffer);
            if (flowReturn != GST_FLOW_OK) {
                std::cerr << "push-buffer returned " << flowReturn << '\n';
            }
        }

        edgevision::config::WebRtcStreamingConfig m_config;
        edgevision::model::viewer::ViewerPoseStore& m_viewerPoseStore;
        edgevision::model::viewer::RenderOutputStore& m_renderOutputStore;
        std::unique_ptr<SignallingHandler> m_signalling;
        std::atomic<bool> m_running{false};
        std::thread m_videoPump;
        std::mutex m_peerConnectionMutex;
        PipelineHandles m_handles{};
        SendFunc m_replyToClient{};
        GObject* m_dataChannel = nullptr;
        bool m_warnedFrameShapeMismatch = false;
    };

    WebRtcServer::WebRtcServer(
        edgevision::config::WebRtcStreamingConfig config,
        edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
        edgevision::model::viewer::RenderOutputStore& renderOutputStore
    )
        : m_impl(std::make_unique<Impl>(std::move(config), viewerPoseStore, renderOutputStore)) {}

    WebRtcServer::~WebRtcServer() = default;

    void WebRtcServer::start() {
        m_impl->start();
    }

    void WebRtcServer::stop() {
        m_impl->stop();
    }

    std::unique_ptr<WebRtcServer> startWebRtcServer(
        edgevision::config::WebRtcStreamingConfig config,
        edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
        edgevision::model::viewer::RenderOutputStore& renderOutputStore
    ) {
        auto server =
            std::make_unique<WebRtcServer>(std::move(config), viewerPoseStore, renderOutputStore);
        server->start();
        return server;
    }

} // namespace edgevision::streaming::webrtc
