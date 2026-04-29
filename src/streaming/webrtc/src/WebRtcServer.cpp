#include "streaming/webrtc/WebRtcServer.hpp"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <gst/app/gstappsrc.h>
#include <gst/sdp/sdp.h>
#include <gst/webrtc/webrtc.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <pthread.h>
#include <sched.h>
#include <sstream>
#include <string>
#include <string_view>
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

        using WebRtcSessionId = std::uint64_t;

        constexpr guint kVideoMLineIndex = 0;
        constexpr auto kNegotiationWatchdogTimeout = std::chrono::seconds(5);
        constexpr char kH264CodecPreferencesCaps[] =
            "application/x-rtp,media=(string)video,encoding-name=(string)H264,"
            "payload=(int)96,clock-rate=(int)90000,packetization-mode=(string)1,"
            "level-asymmetry-allowed=(string)1,profile-level-id=(string)42e01f";

        struct DirectIpv4Link {
            std::string serverIp;
            std::string clientIp;
        };

        [[nodiscard]] std::string toLowerAscii(std::string value) {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        [[nodiscard]] bool disableDataChannelFromEnv() {
            const char* value = std::getenv("EDGEVISION_WEBRTC_DISABLE_DATA_CHANNEL");
            return value != nullptr && value[0] != '\0' && std::strcmp(value, "0") != 0
                && std::strcmp(value, "false") != 0 && std::strcmp(value, "FALSE") != 0;
        }

        [[nodiscard]] std::optional<DirectIpv4Link> directIpv4LinkFromEnv() {
            const char* value = std::getenv("EDGEVISION_WEBRTC_DIRECT_IPV4");
            if (value == nullptr || value[0] == '\0') {
                return std::nullopt;
            }

            const std::string_view raw(value);
            const std::size_t separator = raw.find(',');
            if (separator == std::string_view::npos || separator == 0
                || separator + 1 >= raw.size()) {
                std::cerr << "WebRTC ignoring invalid EDGEVISION_WEBRTC_DIRECT_IPV4='" << raw
                          << "'; expected server_ip,client_ip\n";
                return std::nullopt;
            }

            DirectIpv4Link link{
                std::string{raw.substr(0, separator)},
                std::string{raw.substr(separator + 1)},
            };
            std::cerr << "WebRTC direct IPv4 ICE filter enabled server=" << link.serverIp
                      << " client=" << link.clientIp << '\n';
            return link;
        }

        [[nodiscard]] std::string directCandidateDropReason(
            const std::string& candidate,
            const std::string& requiredIp
        ) {
            const std::string lower = toLowerAscii(candidate);
            if (lower.find(" udp ") == std::string::npos) {
                return "not UDP";
            }
            if (lower.find(" typ host") == std::string::npos) {
                return "not host";
            }
            const std::string requiredIpToken = " " + requiredIp + " ";
            if (candidate.find(requiredIpToken) == std::string::npos) {
                return "not required IP " + requiredIp;
            }
            return {};
        }

        [[nodiscard]] std::string stripLineEnd(std::string line) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }

        [[nodiscard]] const char* dataChannelStateName(GstWebRTCDataChannelState state) {
            switch (state) {
                case GST_WEBRTC_DATA_CHANNEL_STATE_NEW:
                    return "new";
                case GST_WEBRTC_DATA_CHANNEL_STATE_CONNECTING:
                    return "connecting";
                case GST_WEBRTC_DATA_CHANNEL_STATE_OPEN:
                    return "open";
                case GST_WEBRTC_DATA_CHANNEL_STATE_CLOSING:
                    return "closing";
                case GST_WEBRTC_DATA_CHANNEL_STATE_CLOSED:
                    return "closed";
                default:
                    return "unknown";
            }
        }

        [[nodiscard]] const char* iceConnectionStateName(GstWebRTCICEConnectionState state) {
            switch (state) {
                case GST_WEBRTC_ICE_CONNECTION_STATE_NEW:
                    return "new";
                case GST_WEBRTC_ICE_CONNECTION_STATE_CHECKING:
                    return "checking";
                case GST_WEBRTC_ICE_CONNECTION_STATE_CONNECTED:
                    return "connected";
                case GST_WEBRTC_ICE_CONNECTION_STATE_COMPLETED:
                    return "completed";
                case GST_WEBRTC_ICE_CONNECTION_STATE_FAILED:
                    return "failed";
                case GST_WEBRTC_ICE_CONNECTION_STATE_DISCONNECTED:
                    return "disconnected";
                case GST_WEBRTC_ICE_CONNECTION_STATE_CLOSED:
                    return "closed";
                default:
                    return "unknown";
            }
        }

        [[nodiscard]] const char* peerConnectionStateName(GstWebRTCPeerConnectionState state) {
            switch (state) {
                case GST_WEBRTC_PEER_CONNECTION_STATE_NEW:
                    return "new";
                case GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTING:
                    return "connecting";
                case GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTED:
                    return "connected";
                case GST_WEBRTC_PEER_CONNECTION_STATE_DISCONNECTED:
                    return "disconnected";
                case GST_WEBRTC_PEER_CONNECTION_STATE_FAILED:
                    return "failed";
                case GST_WEBRTC_PEER_CONNECTION_STATE_CLOSED:
                    return "closed";
                default:
                    return "unknown";
            }
        }

        [[nodiscard]] const char* signalingStateName(GstWebRTCSignalingState state) {
            switch (state) {
                case GST_WEBRTC_SIGNALING_STATE_STABLE:
                    return "stable";
                case GST_WEBRTC_SIGNALING_STATE_CLOSED:
                    return "closed";
                case GST_WEBRTC_SIGNALING_STATE_HAVE_LOCAL_OFFER:
                    return "have-local-offer";
                case GST_WEBRTC_SIGNALING_STATE_HAVE_REMOTE_OFFER:
                    return "have-remote-offer";
                case GST_WEBRTC_SIGNALING_STATE_HAVE_LOCAL_PRANSWER:
                    return "have-local-pranswer";
                case GST_WEBRTC_SIGNALING_STATE_HAVE_REMOTE_PRANSWER:
                    return "have-remote-pranswer";
                default:
                    return "unknown";
            }
        }

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
            edgevision::config::ImageSize imageSize,
            edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
            edgevision::model::viewer::RenderOutputStore& renderOutputStore
        )
            : m_config(std::move(config)),
              m_imageSize(imageSize),
              m_viewerPoseStore(viewerPoseStore),
              m_renderOutputStore(renderOutputStore),
              m_directIpv4Link(directIpv4LinkFromEnv()) {}

        ~Impl() {
            stop();
        }

        void start() {
            if (m_running.exchange(true)) {
                return;
            }

            gst_init(nullptr, nullptr);

            SignallingCallbacks callbacks;
            callbacks.onAnswer = [this](const std::string& sdp) { onAnswer(sdp); };
            callbacks.onIce = [this](const json& candidate) { onIceFromClient(candidate); };
            callbacks.onClientConnected = [this](SendFunc reply) {
                static_cast<void>(m_viewerPoseStore.resetRelativeBaseline());
                onClientConnected(std::move(reply));
            };
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
        [[nodiscard]] bool isCurrentSessionLocked(
            WebRtcSessionId sessionId,
            GstElement* webrtcbin
        ) const {
            return sessionId != 0 && sessionId == m_activeSessionId
                && m_handles.webrtcbin != nullptr && m_handles.webrtcbin == webrtcbin;
        }

        void logNegotiationStageLocked(WebRtcSessionId sessionId, std::string_view stage) const {
            std::cerr << "WebRTC session " << sessionId << " stage: " << stage << '\n';
        }

        void armNegotiationWatchdogLocked(WebRtcSessionId sessionId, std::string stage) {
            if (sessionId == 0 || sessionId != m_activeSessionId) {
                return;
            }

            m_negotiationWatchdogArmed = true;
            m_negotiationWatchdogSessionId = sessionId;
            m_negotiationWatchdogStage = std::move(stage);
            m_negotiationWatchdogDeadline =
                std::chrono::steady_clock::now() + kNegotiationWatchdogTimeout;
            std::cerr << "WebRTC session " << sessionId << " watchdog armed for "
                      << m_negotiationWatchdogStage << '\n';
        }

        void clearNegotiationWatchdogLocked(WebRtcSessionId sessionId) {
            if (m_negotiationWatchdogSessionId != sessionId) {
                return;
            }

            m_negotiationWatchdogArmed = false;
            m_negotiationWatchdogSessionId = 0;
            m_negotiationWatchdogStage.clear();
        }

        [[nodiscard]] std::optional<std::string> checkNegotiationWatchdogLocked() {
            if (!m_negotiationWatchdogArmed
                || std::chrono::steady_clock::now() < m_negotiationWatchdogDeadline) {
                return std::nullopt;
            }

            const std::string reason =
                "WebRTC negotiation timed out at " + m_negotiationWatchdogStage;
            std::cerr << reason << " for session " << m_negotiationWatchdogSessionId << '\n';
            clearNegotiationWatchdogLocked(m_negotiationWatchdogSessionId);
            return reason;
        }

        [[nodiscard]] bool allowLocalCandidate(const std::string& candidate) const {
            if (!m_directIpv4Link.has_value()) {
                return true;
            }

            const std::string reason =
                directCandidateDropReason(candidate, m_directIpv4Link->serverIp);
            if (reason.empty()) {
                return true;
            }

            std::cerr << "WebRTC direct ICE dropped local candidate: " << reason << " candidate='"
                      << candidate << "'\n";
            return false;
        }

        [[nodiscard]] bool allowRemoteCandidate(const std::string& candidate) const {
            if (!m_directIpv4Link.has_value()) {
                return true;
            }

            const std::string reason =
                directCandidateDropReason(candidate, m_directIpv4Link->clientIp);
            if (reason.empty()) {
                return true;
            }

            std::cerr << "WebRTC direct ICE dropped remote candidate: " << reason << " candidate='"
                      << candidate << "'\n";
            return false;
        }

        [[nodiscard]] std::string filterRemoteAnswerSdpForDirectIpv4(
            const std::string& sdp
        ) const {
            if (!m_directIpv4Link.has_value()) {
                return sdp;
            }

            std::istringstream input(sdp);
            std::ostringstream output;
            std::string line;
            while (std::getline(input, line)) {
                const std::string normalizedLine = stripLineEnd(line);
                if (normalizedLine.rfind("a=candidate:", 0) == 0
                    && !allowRemoteCandidate(normalizedLine)) {
                    continue;
                }

                output << normalizedLine << "\r\n";
            }

            return output.str();
        }

        void onClientConnected(SendFunc reply) {
            WebRtcSessionId sessionId = 0;
            {
                std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
                tearDownPipeline();
                sessionId = m_nextSessionId++;
                m_activeSessionId = sessionId;
                logNegotiationStageLocked(sessionId, "client connected");
                m_handles = buildPipeline(m_config, m_imageSize);
                if (m_handles.pipeline == nullptr || m_handles.webrtcbin == nullptr) {
                    std::cerr << "buildPipeline failed\n";
                    tearDownPipeline();
                    return;
                }
                m_activeWebRtcbin.store(m_handles.webrtcbin);
                connectPipelineBusLoggingLocked();

                const GstStateChangeReturn readyResult =
                    gst_element_set_state(m_handles.pipeline, GST_STATE_READY);
                std::cerr << "WebRTC pipeline READY transition returned "
                          << static_cast<int>(readyResult) << '\n';
                if (readyResult == GST_STATE_CHANGE_FAILURE) {
                    std::cerr << "WebRTC failed to enter READY before local offer setup\n";
                    tearDownPipeline();
                    return;
                }

                m_replyToClient = std::move(reply);
                g_signal_connect(
                    m_handles.webrtcbin,
                    "on-ice-candidate",
                    G_CALLBACK(&Impl::onLocalIceTrampoline),
                    this
                );
                connectWebRtcStateLoggingLocked();
                if (!prepareVideoSendPathLocked()) {
                    std::cerr << "WebRTC failed while preparing local video send path\n";
                    tearDownPipeline();
                    return;
                }
                if (disableDataChannelFromEnv()) {
                    std::cerr << "WebRTC data channel disabled by "
                                 "EDGEVISION_WEBRTC_DISABLE_DATA_CHANNEL\n";
                } else if (!createControlDataChannelLocked()) {
                    std::cerr << "WebRTC failed while creating control data channel\n";
                    tearDownPipeline();
                    return;
                }
                armNegotiationWatchdogLocked(sessionId, "create-offer");
            }

            createLocalOffer(sessionId);
        }

        void onAnswer(const std::string& sdp) {
            WebRtcSessionId sessionId = 0;
            std::string filteredSdp;
            {
                std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
                if (m_handles.webrtcbin == nullptr || !m_localOfferSent) {
                    std::cerr << "ignoring answer before a local offer exists\n";
                    return;
                }
                sessionId = m_activeSessionId;
                logNegotiationStageLocked(sessionId, "answer received");
                armNegotiationWatchdogLocked(sessionId, "set-remote-description");
                filteredSdp = filterRemoteAnswerSdpForDirectIpv4(sdp);
            }

            std::cerr << "WebRTC remote answer SDP:\n" << filteredSdp << '\n';
            applyRemoteDescription(
                filteredSdp, GST_WEBRTC_SDP_TYPE_ANSWER, &Impl::onRemoteAnswerApplied, sessionId
            );
        }

        struct OfferContext {
            OfferContext(Impl* owner, GstElement* webrtc, WebRtcSessionId session)
                : self(owner), webrtcbin(webrtc), sessionId(session) {}

            OfferContext(const OfferContext&) = delete;
            OfferContext& operator=(const OfferContext&) = delete;

            ~OfferContext() {
                if (webrtcbin != nullptr) {
                    gst_object_unref(webrtcbin);
                }
            }

            Impl* self = nullptr;
            GstElement* webrtcbin = nullptr;
            WebRtcSessionId sessionId = 0;
        };

        void createLocalOffer(WebRtcSessionId sessionId) {
            GstElement* webrtcbin = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
                if (!isCurrentSessionLocked(sessionId, m_handles.webrtcbin)) {
                    return;
                }
                webrtcbin = GST_ELEMENT(gst_object_ref(m_handles.webrtcbin));
            }

            auto* context = new OfferContext(this, webrtcbin, sessionId);
            GstPromise* offerPromise =
                gst_promise_new_with_change_func(&Impl::onOfferCreated, context, nullptr);
            g_signal_emit_by_name(webrtcbin, "create-offer", nullptr, offerPromise);
        }

        static void onOfferCreated(GstPromise* promise, gpointer userData) {
            std::unique_ptr<OfferContext> context(static_cast<OfferContext*>(userData));
            Impl* self = context->self;
            const GstPromiseResult result = gst_promise_wait(promise);
            const GstStructure* reply =
                result == GST_PROMISE_RESULT_REPLIED ? gst_promise_get_reply(promise) : nullptr;
            GstWebRTCSessionDescription* offer = nullptr;
            if (reply != nullptr) {
                gst_structure_get(
                    reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, nullptr
                );
            }
            gst_promise_unref(promise);
            std::cout << "WebRTC trigger create-offer promise fired\n";
            if (offer == nullptr) {
                std::cerr << "create-offer returned no offer\n";
                std::lock_guard<std::mutex> lock(self->m_peerConnectionMutex);
                if (self->isCurrentSessionLocked(context->sessionId, context->webrtcbin)) {
                    self->tearDownPipeline();
                }
                return;
            }

            self->setLocalDescriptionAndSendOffer(offer, context->webrtcbin, context->sessionId);
        }

        void setLocalDescriptionAndSendOffer(
            GstWebRTCSessionDescription* offer,
            GstElement* offerWebRtcbin,
            WebRtcSessionId sessionId
        ) {
            const auto mediaMids = extractMediaMids(offer->sdp);
            const auto videoMid = firstMediaMid(offer->sdp, "video");
            gchar* sdpText = gst_sdp_message_as_text(offer->sdp);

            GstElement* webrtcbin = nullptr;
            GstElement* pipeline = nullptr;
            SendFunc reply;
            {
                std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
                if (!isCurrentSessionLocked(sessionId, offerWebRtcbin)
                    || m_handles.pipeline == nullptr) {
                    g_free(sdpText);
                    gst_webrtc_session_description_free(offer);
                    return;
                }

                logNegotiationStageLocked(sessionId, "offer created");
                armNegotiationWatchdogLocked(sessionId, "set-local-description");
                m_localMediaMids = mediaMids;
                m_localVideoMid = videoMid;
                m_remoteDescriptionApplied = false;
                m_mediaPipelineStarted = false;
                m_videoPushEnabled.store(false);
                reply = m_replyToClient;
                webrtcbin = GST_ELEMENT(gst_object_ref(m_handles.webrtcbin));
                pipeline = GST_ELEMENT(gst_object_ref(m_handles.pipeline));
            }

            auto* context = new LocalOfferContext(
                this,
                webrtcbin,
                pipeline,
                offer,
                sessionId,
                sdpText == nullptr ? std::string{} : std::string{sdpText},
                std::move(reply)
            );
            GstPromise* setPromise =
                gst_promise_new_with_change_func(&Impl::onLocalDescriptionSet, context, nullptr);
            g_signal_emit_by_name(webrtcbin, "set-local-description", offer, setPromise);
            g_free(sdpText);
        }

        struct LocalOfferContext {
            LocalOfferContext(
                Impl* owner,
                GstElement* webrtc,
                GstElement* pipe,
                GstWebRTCSessionDescription* sessionOffer,
                WebRtcSessionId session,
                std::string text,
                SendFunc replyFn
            )
                : self(owner),
                  webrtcbin(webrtc),
                  pipeline(pipe),
                  offer(sessionOffer),
                  sessionId(session),
                  sdpText(std::move(text)),
                  reply(std::move(replyFn)) {}

            LocalOfferContext(const LocalOfferContext&) = delete;
            LocalOfferContext& operator=(const LocalOfferContext&) = delete;

            ~LocalOfferContext() {
                if (offer != nullptr) {
                    gst_webrtc_session_description_free(offer);
                }
                if (pipeline != nullptr) {
                    gst_object_unref(pipeline);
                }
                if (webrtcbin != nullptr) {
                    gst_object_unref(webrtcbin);
                }
            }

            Impl* self = nullptr;
            GstElement* webrtcbin = nullptr;
            GstElement* pipeline = nullptr;
            GstWebRTCSessionDescription* offer = nullptr;
            WebRtcSessionId sessionId = 0;
            std::string sdpText;
            SendFunc reply;
        };

        static void onLocalDescriptionSet(GstPromise* promise, gpointer userData) {
            std::unique_ptr<LocalOfferContext> context(static_cast<LocalOfferContext*>(userData));
            Impl* self = context->self;
            const GstPromiseResult result = gst_promise_wait(promise);
            gst_promise_unref(promise);
            std::cout << "WebRTC trigger set-local-description promise fired result="
                      << static_cast<int>(result) << '\n';

            if (result != GST_PROMISE_RESULT_REPLIED) {
                std::cerr << "set-local-description did not complete successfully\n";
                std::lock_guard<std::mutex> lock(self->m_peerConnectionMutex);
                if (self->isCurrentSessionLocked(context->sessionId, context->webrtcbin)) {
                    self->tearDownPipeline();
                }
            } else {
                bool sessionStillActive = false;
                {
                    std::lock_guard<std::mutex> lock(self->m_peerConnectionMutex);
                    sessionStillActive =
                        self->isCurrentSessionLocked(context->sessionId, context->webrtcbin)
                        && self->m_handles.pipeline == context->pipeline;
                    if (sessionStillActive) {
                        self->m_localOfferSent = true;
                        self->logNegotiationStageLocked(
                            context->sessionId, "local description set"
                        );
                        self->armNegotiationWatchdogLocked(context->sessionId, "remote answer");
                    }
                }
                if (!sessionStillActive) {
                    return;
                }

                std::cerr << "WebRTC local offer SDP:\n" << context->sdpText << '\n';

                const json response = {{"type", "offer"}, {"sdp", context->sdpText}};
                if (context->reply) {
                    context->reply(response.dump());
                }
            }
        }

        struct RemoteDescriptionContext {
            RemoteDescriptionContext(Impl* owner, GstElement* webrtc, WebRtcSessionId session)
                : self(owner), webrtcbin(webrtc), sessionId(session) {}

            RemoteDescriptionContext(const RemoteDescriptionContext&) = delete;
            RemoteDescriptionContext& operator=(const RemoteDescriptionContext&) = delete;

            ~RemoteDescriptionContext() {
                if (webrtcbin != nullptr) {
                    gst_object_unref(webrtcbin);
                }
            }

            Impl* self = nullptr;
            GstElement* webrtcbin = nullptr;
            WebRtcSessionId sessionId = 0;
        };

        void applyRemoteDescription(
            const std::string& sdp,
            GstWebRTCSDPType type,
            GstPromiseChangeFunc callback,
            WebRtcSessionId sessionId
        ) {
            GstElement* webrtcbin = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
                if (!isCurrentSessionLocked(sessionId, m_handles.webrtcbin)) {
                    return;
                }
                webrtcbin = GST_ELEMENT(gst_object_ref(m_handles.webrtcbin));
            }

            GstSDPMessage* message = nullptr;
            const GstSDPResult parseResult = gst_sdp_message_new_from_text(sdp.c_str(), &message);
            if (parseResult != GST_SDP_OK || message == nullptr) {
                std::cerr << "failed to parse remote WebRTC SDP\n";
                gst_object_unref(webrtcbin);
                return;
            }

            auto* description = gst_webrtc_session_description_new(type, message);
            if (description == nullptr) {
                std::cerr << "failed to create remote WebRTC session description\n";
                gst_sdp_message_free(message);
                gst_object_unref(webrtcbin);
                return;
            }

            auto* context = new RemoteDescriptionContext(this, webrtcbin, sessionId);
            GstPromise* promise = gst_promise_new_with_change_func(callback, context, nullptr);
            g_signal_emit_by_name(webrtcbin, "set-remote-description", description, promise);
            gst_webrtc_session_description_free(description);
        }

        static void onRemoteAnswerApplied(GstPromise* promise, gpointer userData) {
            std::unique_ptr<RemoteDescriptionContext> context(
                static_cast<RemoteDescriptionContext*>(userData)
            );
            Impl* self = context->self;
            const GstPromiseResult result = gst_promise_wait(promise);
            gst_promise_unref(promise);
            std::cout << "WebRTC trigger set-remote-description answer promise fired result="
                      << static_cast<int>(result) << '\n';

            std::lock_guard<std::mutex> lock(self->m_peerConnectionMutex);
            if (!self->isCurrentSessionLocked(context->sessionId, context->webrtcbin)
                || self->m_handles.pipeline == nullptr) {
                return;
            }
            if (result != GST_PROMISE_RESULT_REPLIED) {
                std::cerr << "set-remote-description did not complete successfully\n";
                self->tearDownPipeline();
                return;
            }

            self->m_remoteDescriptionApplied = true;
            self->logNegotiationStageLocked(context->sessionId, "remote description set");
            self->clearNegotiationWatchdogLocked(context->sessionId);
            std::cerr << "WebRTC remote answer applied; flushing "
                      << self->m_pendingRemoteIce.size() << " queued ICE candidate(s)\n";
            self->flushPendingRemoteIceLocked();
            if (!self->startMediaPipelineLocked()) {
                std::cerr << "WebRTC failed to start media pipeline after remote answer\n";
                self->tearDownPipeline();
            }
        }

        [[nodiscard]] bool startMediaPipelineLocked() {
            if (m_mediaPipelineStarted) {
                return true;
            }
            if (m_handles.pipeline == nullptr) {
                return false;
            }

            const GstStateChangeReturn playingResult =
                gst_element_set_state(m_handles.pipeline, GST_STATE_PLAYING);
            std::cerr << "WebRTC pipeline PLAYING transition after answer returned "
                      << static_cast<int>(playingResult) << '\n';
            if (playingResult == GST_STATE_CHANGE_FAILURE) {
                return false;
            }

            m_mediaPipelineStarted = true;
            return true;
        }

        void connectWebRtcStateLoggingLocked() {
            if (m_handles.webrtcbin == nullptr) {
                return;
            }
            g_signal_connect(
                m_handles.webrtcbin,
                "notify::signaling-state",
                G_CALLBACK(&Impl::onWebRtcStateNotifyTrampoline),
                this
            );
            g_signal_connect(
                m_handles.webrtcbin,
                "notify::ice-connection-state",
                G_CALLBACK(&Impl::onWebRtcStateNotifyTrampoline),
                this
            );
            g_signal_connect(
                m_handles.webrtcbin,
                "notify::connection-state",
                G_CALLBACK(&Impl::onWebRtcStateNotifyTrampoline),
                this
            );
        }

        static void onWebRtcStateNotifyTrampoline(
            GObject* object,
            GParamSpec* pspec,
            gpointer userData
        ) {
            auto* self = static_cast<Impl*>(userData);
            if (self == nullptr || self->m_activeWebRtcbin.load() != GST_ELEMENT(object)) {
                return;
            }

            const char* propertyName = g_param_spec_get_name(pspec);
            if (std::strcmp(propertyName, "signaling-state") == 0) {
                GstWebRTCSignalingState state = GST_WEBRTC_SIGNALING_STATE_STABLE;
                g_object_get(object, propertyName, &state, nullptr);
                std::cout << "WebRTC signal notify::" << propertyName << " -> "
                          << signalingStateName(state) << '\n';
                std::cerr << "WebRTC " << propertyName << " -> " << signalingStateName(state)
                          << '\n';
                return;
            }

            if (std::strcmp(propertyName, "ice-connection-state") == 0) {
                GstWebRTCICEConnectionState state = GST_WEBRTC_ICE_CONNECTION_STATE_NEW;
                g_object_get(object, propertyName, &state, nullptr);
                std::cout << "WebRTC signal notify::" << propertyName << " -> "
                          << iceConnectionStateName(state) << '\n';
                std::cerr << "WebRTC " << propertyName << " -> " << iceConnectionStateName(state)
                          << '\n';
                return;
            }

            if (std::strcmp(propertyName, "connection-state") == 0) {
                GstWebRTCPeerConnectionState state = GST_WEBRTC_PEER_CONNECTION_STATE_NEW;
                g_object_get(object, propertyName, &state, nullptr);
                std::cout << "WebRTC signal notify::" << propertyName << " -> "
                          << peerConnectionStateName(state) << '\n';
                std::cerr << "WebRTC " << propertyName << " -> " << peerConnectionStateName(state)
                          << '\n';
                setVideoPushEnabledForConnectionState(self, state);
                return;
            }

            gint state = 0;
            g_object_get(object, propertyName, &state, nullptr);
            std::cerr << "WebRTC " << propertyName << " -> " << state << '\n';
        }

        static void setVideoPushEnabledForConnectionState(
            Impl* self,
            GstWebRTCPeerConnectionState state
        ) {
            if (self == nullptr) {
                return;
            }

            const bool shouldEnable = state == GST_WEBRTC_PEER_CONNECTION_STATE_CONNECTED;
            const bool wasEnabled = self->m_videoPushEnabled.exchange(shouldEnable);
            if (wasEnabled == shouldEnable) {
                return;
            }

            std::cout << "WebRTC trigger video push " << (shouldEnable ? "enabled" : "disabled")
                      << " by connection-state " << peerConnectionStateName(state) << '\n';
            std::cerr << "WebRTC video push " << (shouldEnable ? "enabled" : "disabled")
                      << " after connection-state " << peerConnectionStateName(state) << '\n';
        }

        void connectPipelineBusLoggingLocked() {
            if (m_handles.pipeline == nullptr) {
                return;
            }

            GstBus* bus = gst_element_get_bus(m_handles.pipeline);
            if (bus == nullptr) {
                std::cerr << "WebRTC pipeline bus was unavailable for logging\n";
                return;
            }

            gst_bus_set_sync_handler(bus, &Impl::onBusSyncMessage, this, nullptr);
            gst_object_unref(bus);
        }

        void disconnectPipelineBusLoggingLocked() {
            if (m_handles.pipeline == nullptr) {
                return;
            }

            GstBus* bus = gst_element_get_bus(m_handles.pipeline);
            if (bus == nullptr) {
                return;
            }

            gst_bus_set_sync_handler(bus, nullptr, nullptr, nullptr);
            gst_object_unref(bus);
        }

        static GstBusSyncReply onBusSyncMessage(GstBus*, GstMessage* message, gpointer) {
            switch (GST_MESSAGE_TYPE(message)) {
                case GST_MESSAGE_ERROR:
                    logBusMessage("ERROR", message);
                    break;
                case GST_MESSAGE_WARNING:
                    logBusMessage("WARNING", message);
                    break;
                default:
                    break;
            }

            return GST_BUS_PASS;
        }

        static void logBusMessage(const char* level, GstMessage* message) {
            GError* error = nullptr;
            gchar* debug = nullptr;
            if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
                gst_message_parse_error(message, &error, &debug);
            } else if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_WARNING) {
                gst_message_parse_warning(message, &error, &debug);
            }

            const gchar* source = GST_MESSAGE_SRC_NAME(message);
            std::cerr << "WebRTC pipeline " << level << " from "
                      << (source == nullptr ? "<unknown>" : source) << ": "
                      << (error == nullptr ? "<no message>" : error->message) << '\n';
            if (debug != nullptr) {
                std::cerr << "WebRTC pipeline " << level << " debug: " << debug << '\n';
            }

            if (error != nullptr) {
                g_error_free(error);
            }
            g_free(debug);
        }

        [[nodiscard]] bool prepareVideoSendPathLocked() {
            if (m_videoSendPathReady) {
                return true;
            }

            if (m_videoWebRtcSinkPad == nullptr) {
                m_videoWebRtcSinkPad = requestVerifiedVideoSinkPadLocked(kVideoMLineIndex);
                if (m_videoWebRtcSinkPad == nullptr) {
                    std::cerr << "WebRTC failed to prepare sendonly video slot 0\n";
                    return false;
                }
                if (!configurePadTransceiverCodecPreferencesLocked(
                        m_videoWebRtcSinkPad, kVideoMLineIndex
                    )) {
                    std::cerr << "WebRTC failed to set video slot 0 H264 codec preferences\n";
                    return false;
                }
                if (!configurePadTransceiverDirectionLocked(
                        m_videoWebRtcSinkPad,
                        kVideoMLineIndex,
                        GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY
                    )) {
                    std::cerr << "WebRTC failed to set video slot 0 sendonly\n";
                    return false;
                }
            }

            if (!linkVideoPayloaderToSinkPadLocked(m_videoWebRtcSinkPad, kVideoMLineIndex)) {
                return false;
            }

            m_videoSendPathReady = true;
            return true;
        }

        [[nodiscard]] bool createControlDataChannelLocked() {
            if (m_dataChannel != nullptr) {
                return true;
            }
            if (m_handles.webrtcbin == nullptr) {
                return false;
            }

            GstWebRTCDataChannel* channel = nullptr;
            g_signal_emit_by_name(
                m_handles.webrtcbin, "create-data-channel", "control", nullptr, &channel
            );
            if (channel == nullptr) {
                return false;
            }

            m_dataChannel = channel;
            m_activeDataChannel.store(channel);
            logDataChannelState(channel, "created");
            g_signal_connect(
                channel, "on-message-string", G_CALLBACK(&Impl::onMessageStringTrampoline), this
            );
            g_signal_connect(channel, "on-open", G_CALLBACK(&Impl::onChannelOpenTrampoline), this);
            g_signal_connect(
                channel, "on-close", G_CALLBACK(&Impl::onDataChannelCloseTrampoline), this
            );
            g_signal_connect(
                channel, "on-error", G_CALLBACK(&Impl::onDataChannelErrorTrampoline), this
            );
            return true;
        }

        [[nodiscard]] GstPad* requestVerifiedVideoSinkPadLocked(guint requestedMLineIndex) {
            if (m_handles.webrtcbin == nullptr) {
                std::cerr << "WebRTC sink pad requested before webrtcbin was ready\n";
                return nullptr;
            }

            gchar* padName = g_strdup_printf("sink_%u", requestedMLineIndex);
            GstPad* sinkPad = gst_element_request_pad_simple(m_handles.webrtcbin, padName);
            g_free(padName);
            if (sinkPad == nullptr) {
                std::cerr << "WebRTC failed to request sink pad for m-line " << requestedMLineIndex
                          << '\n';
                return nullptr;
            }

            GstWebRTCRTPTransceiver* transceiver = nullptr;
            g_object_get(sinkPad, "transceiver", &transceiver, nullptr);
            if (transceiver == nullptr) {
                std::cerr << "WebRTC sink pad for m-line " << requestedMLineIndex
                          << " had no associated transceiver\n";
                gst_element_release_request_pad(m_handles.webrtcbin, sinkPad);
                gst_object_unref(sinkPad);
                return nullptr;
            }

            gchar* mid = nullptr;
            gint kind = GST_WEBRTC_KIND_UNKNOWN;
            guint actualMLineIndex = static_cast<guint>(-1);
            g_object_get(
                transceiver, "mid", &mid, "kind", &kind, "mlineindex", &actualMLineIndex, nullptr
            );
            std::cerr << "WebRTC sink pad for requested m-line " << requestedMLineIndex
                      << " associated to transceiver kind=" << kind
                      << " mlineindex=" << actualMLineIndex
                      << " mid=" << (mid == nullptr ? "<null>" : mid) << '\n';

            const bool matches = actualMLineIndex == requestedMLineIndex;
            g_free(mid);
            gst_object_unref(transceiver);
            if (!matches) {
                std::cerr << "WebRTC sink pad association mismatch: requested m-line "
                          << requestedMLineIndex << ", got kind=" << kind
                          << " mlineindex=" << actualMLineIndex << '\n';
                gst_element_release_request_pad(m_handles.webrtcbin, sinkPad);
                gst_object_unref(sinkPad);
                return nullptr;
            }

            return sinkPad;
        }

        [[nodiscard]] bool configurePadTransceiverCodecPreferencesLocked(
            GstPad* sinkPad,
            guint requestedMLineIndex
        ) {
            if (sinkPad == nullptr) {
                std::cerr << "WebRTC codec preferences requested without a sink pad\n";
                return false;
            }

            GstWebRTCRTPTransceiver* transceiver = nullptr;
            g_object_get(sinkPad, "transceiver", &transceiver, nullptr);
            if (transceiver == nullptr) {
                std::cerr << "WebRTC sink pad for m-line " << requestedMLineIndex
                          << " had no transceiver for codec preferences\n";
                return false;
            }

            GstCaps* codecPreferences = gst_caps_from_string(kH264CodecPreferencesCaps);
            if (codecPreferences == nullptr) {
                std::cerr << "WebRTC failed to build H264 codec preferences caps for m-line "
                          << requestedMLineIndex << '\n';
                gst_object_unref(transceiver);
                return false;
            }

            g_object_set(transceiver, "codec-preferences", codecPreferences, nullptr);
            gst_caps_unref(codecPreferences);
            gst_object_unref(transceiver);
            return true;
        }

        [[nodiscard]] bool configurePadTransceiverDirectionLocked(
            GstPad* sinkPad,
            guint requestedMLineIndex,
            GstWebRTCRTPTransceiverDirection direction
        ) {
            if (sinkPad == nullptr) {
                std::cerr << "WebRTC transceiver direction requested without a sink pad\n";
                return false;
            }

            GstWebRTCRTPTransceiver* transceiver = nullptr;
            g_object_get(sinkPad, "transceiver", &transceiver, nullptr);
            if (transceiver == nullptr) {
                std::cerr << "WebRTC sink pad for m-line " << requestedMLineIndex
                          << " had no transceiver for direction configuration\n";
                return false;
            }

            g_object_set(transceiver, "direction", direction, nullptr);
            gst_object_unref(transceiver);
            return true;
        }

        [[nodiscard]] bool linkVideoPayloaderToSinkPadLocked(
            GstPad* sinkPad,
            guint requestedMLineIndex
        ) {
            if (sinkPad == nullptr || m_handles.videoPayloader == nullptr) {
                std::cerr
                    << "WebRTC video link requested before sink pad or payloader was ready\n";
                return false;
            }
            if (gst_pad_is_linked(sinkPad)) {
                return true;
            }

            GstPad* payloaderSrcPad = gst_element_get_static_pad(m_handles.videoPayloader, "src");
            if (payloaderSrcPad == nullptr) {
                std::cerr << "WebRTC payloader src pad was unavailable for linking\n";
                return false;
            }

            const GstPadLinkReturn linkResult = gst_pad_link(payloaderSrcPad, sinkPad);
            gst_object_unref(payloaderSrcPad);
            if (linkResult != GST_PAD_LINK_OK) {
                std::cerr << "WebRTC failed to link payloader to m-line " << requestedMLineIndex
                          << " (pad link result " << static_cast<int>(linkResult) << ")\n";
                return false;
            }

            return true;
        }

        static void onLocalIceTrampoline(
            GstElement* webrtcbin,
            guint mediaLineIndex,
            gchar* candidate,
            gpointer userData
        ) {
            auto* self = static_cast<Impl*>(userData);
            if (self == nullptr || self->m_activeWebRtcbin.load() != webrtcbin) {
                return;
            }

            const std::string candidateValue(candidate == nullptr ? "" : candidate);
            if (candidateValue.empty() || !self->allowLocalCandidate(candidateValue)) {
                return;
            }

            std::cout << "WebRTC signal on-ice-candidate fired m-line=" << mediaLineIndex << '\n';
            SendFunc reply;
            std::string mid;
            {
                std::lock_guard<std::mutex> lock(self->m_peerConnectionMutex);
                if (self->m_handles.webrtcbin != webrtcbin) {
                    return;
                }
                reply = self->m_replyToClient;
                mid = self->localMidForMediaLineIndexLocked(mediaLineIndex);
            }
            if (reply) {
                const json message = {
                    {"type", "ice"},
                    {"candidate", candidateValue},
                    {"sdpMLineIndex", static_cast<int>(mediaLineIndex)},
                    {"sdpMid", mid},
                };
                reply(message.dump());
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

            if (!allowRemoteCandidate(candidateValue)) {
                return;
            }

            std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
            if (m_handles.webrtcbin == nullptr) {
                return;
            }

            if (!m_remoteDescriptionApplied) {
                m_pendingRemoteIce.push_back(candidate);
                return;
            }

            addIceCandidateLocked(static_cast<guint>(*mediaLineIndex), candidateValue);
        }

        static void onChannelOpenTrampoline(GObject* object, gpointer userData) {
            auto* self = static_cast<Impl*>(userData);
            auto* openedChannel = GST_WEBRTC_DATA_CHANNEL(object);
            if (self == nullptr || self->m_activeDataChannel.load() != openedChannel) {
                return;
            }

            std::cout << "WebRTC signal data-channel on-open fired\n";
            std::string videoMid;
            GstWebRTCDataChannel* channel = nullptr;
            {
                std::lock_guard<std::mutex> lock(self->m_peerConnectionMutex);
                videoMid = self->m_localVideoMid;
                if (self->m_dataChannel == openedChannel) {
                    channel = GST_WEBRTC_DATA_CHANNEL(gst_object_ref(openedChannel));
                }
            }
            if (channel != nullptr) {
                logDataChannelState(channel, "on-open");
                gst_object_unref(channel);
            }
            self->sendControlText(
                json{
                    {"type", "hello"},
                    {"from", "edgevision-jetson"},
                }
                    .dump()
            );
            if (!videoMid.empty()) {
                self->sendControlText(
                    json{
                        {"type", "track_info"},
                        {"tracks",
                         json::array({json{{"mid", videoMid}, {"kind", "render_output"}}})},
                    }
                        .dump()
                );
            }
        }

        static void onDataChannelCloseTrampoline(GObject* channel, gpointer) {
            std::cout << "WebRTC signal data-channel on-close fired\n";
            logDataChannelState(GST_WEBRTC_DATA_CHANNEL(channel), "on-close");
        }

        static void onDataChannelErrorTrampoline(GObject* channel, GError* error, gpointer) {
            std::cout << "WebRTC signal data-channel on-error fired\n";
            logDataChannelState(GST_WEBRTC_DATA_CHANNEL(channel), "on-error");
            if (error != nullptr) {
                std::cerr << "WebRTC data channel error: " << error->message << '\n';
            }
        }

        static void onMessageStringTrampoline(GObject* object, gchar* text, gpointer userData) {
            auto* self = static_cast<Impl*>(userData);
            if (self == nullptr
                || self->m_activeDataChannel.load() != GST_WEBRTC_DATA_CHANNEL(object)) {
                return;
            }

            std::cout << "WebRTC signal data-channel on-message-string fired\n";
            const std::string payload(text == nullptr ? "" : text);
            if (const auto pose = utils::parsePoseUpdateMessage(payload); pose.has_value()) {
                self->applyPoseUpdate(*pose);
            }
        }

        void applyPoseUpdate(const PoseUpdate& pose) {
            edgevision::model::scene::Pose4f relativePose{};
            relativePose.matrix = pose.matrix;
            static_cast<void>(m_viewerPoseStore.applyRelativePose(relativePose));
        }

        void sendControlText(const std::string& text) {
            GstWebRTCDataChannel* dataChannel = nullptr;
            {
                std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
                if (m_dataChannel == nullptr) {
                    return;
                }
                dataChannel = GST_WEBRTC_DATA_CHANNEL(gst_object_ref(m_dataChannel));
            }

            logDataChannelState(dataChannel, "send-string");
            g_signal_emit_by_name(dataChannel, "send-string", text.c_str());
            gst_object_unref(dataChannel);
        }

        static void logDataChannelState(GstWebRTCDataChannel* channel, const char* context) {
            if (channel == nullptr) {
                std::cerr << "WebRTC data channel " << context << " state=<null>\n";
                return;
            }

            GstWebRTCDataChannelState readyState = GST_WEBRTC_DATA_CHANNEL_STATE_NEW;
            g_object_get(channel, "ready-state", &readyState, nullptr);
            std::cerr << "WebRTC data channel " << context
                      << " ready-state=" << dataChannelStateName(readyState) << '\n';
        }

        void addIceCandidateLocked(guint mediaLineIndex, const std::string& candidateValue) {
            g_signal_emit_by_name(
                m_handles.webrtcbin, "add-ice-candidate", mediaLineIndex, candidateValue.c_str()
            );
        }

        void flushPendingRemoteIceLocked() {
            for (const auto& candidate : m_pendingRemoteIce) {
                const std::string candidateValue = candidate.value("candidate", "");
                const std::optional<int> mediaLineIndex = candidate.contains("sdpMLineIndex")
                    ? std::optional<int>(candidate.at("sdpMLineIndex").get<int>())
                    : std::nullopt;
                if (candidateValue.empty() || !mediaLineIndex.has_value()) {
                    continue;
                }

                addIceCandidateLocked(static_cast<guint>(*mediaLineIndex), candidateValue);
            }
            m_pendingRemoteIce.clear();
        }

        [[nodiscard]] std::string localMidForMediaLineIndexLocked(guint mediaLineIndex) const {
            if (mediaLineIndex < m_localMediaMids.size()
                && !m_localMediaMids[mediaLineIndex].empty()) {
                return m_localMediaMids[mediaLineIndex];
            }
            return std::to_string(mediaLineIndex);
        }

        [[nodiscard]] static std::vector<std::string> extractMediaMids(
            const GstSDPMessage* message
        ) {
            std::vector<std::string> mids;
            if (message == nullptr) {
                return mids;
            }

            const guint mediaCount = gst_sdp_message_medias_len(message);
            mids.reserve(mediaCount);
            for (guint index = 0; index < mediaCount; ++index) {
                const GstSDPMedia* media = gst_sdp_message_get_media(message, index);
                if (media == nullptr) {
                    mids.emplace_back();
                    continue;
                }

                const gchar* mid = gst_sdp_media_get_attribute_val(media, "mid");
                mids.emplace_back(mid == nullptr ? "" : mid);
            }
            return mids;
        }

        [[nodiscard]] static std::string firstMediaMid(
            const GstSDPMessage* message,
            const char* mediaType
        ) {
            if (message == nullptr) {
                return {};
            }

            const guint mediaCount = gst_sdp_message_medias_len(message);
            for (guint index = 0; index < mediaCount; ++index) {
                const GstSDPMedia* media = gst_sdp_message_get_media(message, index);
                if (media == nullptr) {
                    continue;
                }
                const gchar* currentMediaType = gst_sdp_media_get_media(media);
                if (currentMediaType == nullptr || std::strcmp(currentMediaType, mediaType) != 0) {
                    continue;
                }

                const gchar* mid = gst_sdp_media_get_attribute_val(media, "mid");
                return mid == nullptr ? std::string{} : std::string{mid};
            }

            return {};
        }

        void onClientDisconnected() {
            std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
            tearDownPipeline();
        }

        void tearDownPipeline() {
            m_activeSessionId = 0;
            m_activeWebRtcbin.store(nullptr);
            m_activeDataChannel.store(nullptr);
            m_replyToClient = nullptr;
            if (m_dataChannel != nullptr) {
                gst_webrtc_data_channel_close(m_dataChannel);
                gst_object_unref(m_dataChannel);
                m_dataChannel = nullptr;
            }
            m_localOfferSent = false;
            m_remoteDescriptionApplied = false;
            m_videoSendPathReady = false;
            m_mediaPipelineStarted = false;
            m_videoPushEnabled.store(false);
            m_pendingRemoteIce.clear();
            m_localMediaMids.clear();
            m_localVideoMid.clear();
            m_negotiationWatchdogArmed = false;
            m_negotiationWatchdogSessionId = 0;
            m_negotiationWatchdogStage.clear();
            if (m_videoWebRtcSinkPad != nullptr) {
                if (m_handles.webrtcbin != nullptr) {
                    gst_element_release_request_pad(m_handles.webrtcbin, m_videoWebRtcSinkPad);
                }
                gst_object_unref(m_videoWebRtcSinkPad);
                m_videoWebRtcSinkPad = nullptr;
            }
            if (m_handles.pipeline == nullptr) {
                return;
            }

            disconnectPipelineBusLoggingLocked();
            gst_element_set_state(m_handles.pipeline, GST_STATE_NULL);
            if (m_handles.webrtcbin != nullptr) {
                gst_object_unref(m_handles.webrtcbin);
            }
            if (m_handles.videoAppSrc != nullptr) {
                gst_object_unref(m_handles.videoAppSrc);
            }
            if (m_handles.videoPayloader != nullptr) {
                gst_object_unref(m_handles.videoPayloader);
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
                std::optional<std::string> negotiationTimeoutReason;
                {
                    std::lock_guard<std::mutex> lock(m_peerConnectionMutex);
                    negotiationTimeoutReason = checkNegotiationWatchdogLocked();
                    if (!negotiationTimeoutReason && m_mediaPipelineStarted
                        && m_videoPushEnabled.load() && m_handles.videoAppSrc != nullptr) {
                        appSrc = GST_ELEMENT(gst_object_ref(m_handles.videoAppSrc));
                    }
                }

                if (negotiationTimeoutReason.has_value() && m_signalling) {
                    m_signalling->closeActiveClient(*negotiationTimeoutReason);
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
            if (output.imageSize.width != m_imageSize.width
                || output.imageSize.height != m_imageSize.height) {
                if (!m_warnedFrameShapeMismatch) {
                    std::cerr << "WebRTC output size mismatch: expected " << m_imageSize.width
                              << 'x' << m_imageSize.height << " but received "
                              << output.imageSize.width << 'x' << output.imageSize.height << '\n';
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
        edgevision::config::ImageSize m_imageSize{};
        edgevision::model::viewer::ViewerPoseStore& m_viewerPoseStore;
        edgevision::model::viewer::RenderOutputStore& m_renderOutputStore;
        std::optional<DirectIpv4Link> m_directIpv4Link;
        std::unique_ptr<SignallingHandler> m_signalling;
        std::atomic<bool> m_running{false};
        std::thread m_videoPump;
        std::mutex m_peerConnectionMutex;
        PipelineHandles m_handles{};
        WebRtcSessionId m_nextSessionId = 1;
        WebRtcSessionId m_activeSessionId = 0;
        std::atomic<GstElement*> m_activeWebRtcbin{nullptr};
        std::atomic<GstWebRTCDataChannel*> m_activeDataChannel{nullptr};
        SendFunc m_replyToClient{};
        GstWebRTCDataChannel* m_dataChannel = nullptr;
        GstPad* m_videoWebRtcSinkPad = nullptr;
        bool m_localOfferSent = false;
        bool m_remoteDescriptionApplied = false;
        bool m_videoSendPathReady = false;
        bool m_mediaPipelineStarted = false;
        std::atomic<bool> m_videoPushEnabled{false};
        std::vector<json> m_pendingRemoteIce;
        std::vector<std::string> m_localMediaMids;
        std::string m_localVideoMid;
        bool m_negotiationWatchdogArmed = false;
        WebRtcSessionId m_negotiationWatchdogSessionId = 0;
        std::chrono::steady_clock::time_point m_negotiationWatchdogDeadline{};
        std::string m_negotiationWatchdogStage;
        bool m_warnedFrameShapeMismatch = false;
    };

    WebRtcServer::WebRtcServer(
        edgevision::config::WebRtcStreamingConfig config,
        edgevision::config::ImageSize imageSize,
        edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
        edgevision::model::viewer::RenderOutputStore& renderOutputStore
    )
        : m_impl(
              std::make_unique<Impl>(
                  std::move(config),
                  imageSize,
                  viewerPoseStore,
                  renderOutputStore
              )
          ) {}

    WebRtcServer::~WebRtcServer() = default;

    void WebRtcServer::start() {
        m_impl->start();
    }

    void WebRtcServer::stop() {
        m_impl->stop();
    }

    std::unique_ptr<WebRtcServer> startWebRtcServer(
        edgevision::config::WebRtcStreamingConfig config,
        edgevision::config::ImageSize imageSize,
        edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
        edgevision::model::viewer::RenderOutputStore& renderOutputStore
    ) {
        auto server = std::make_unique<WebRtcServer>(
            std::move(config), imageSize, viewerPoseStore, renderOutputStore
        );
        server->start();
        return server;
    }

} // namespace edgevision::streaming::webrtc
