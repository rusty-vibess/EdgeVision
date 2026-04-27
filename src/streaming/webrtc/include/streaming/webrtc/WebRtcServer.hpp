#pragma once

#include <cstdint>
#include <memory>

#include "config/types/streaming.hpp"

namespace edgevision::model::viewer {
    class RenderOutputStore;
    class ViewerPoseStore;
} // namespace edgevision::model::viewer

namespace edgevision::streaming::webrtc {

    /// Top-level facade. Owns gstreamer pipeline + signalling handler +
    /// frame-pump threads. Construct → start() once at boot → stop() on
    /// shutdown. Single-client at a time; new client supersedes old.
    class WebRtcServer {
      public:
        WebRtcServer(
            edgevision::config::WebRtcStreamingConfig config,
            edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
            edgevision::model::viewer::RenderOutputStore& renderOutputStore
        );
        WebRtcServer(const WebRtcServer&) = delete;
        WebRtcServer& operator=(const WebRtcServer&) = delete;
        ~WebRtcServer();

        void start();
        void stop();

      private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

    /// Convenience entry point for main.cpp.
    [[nodiscard]] std::unique_ptr<WebRtcServer> startWebRtcServer(
        edgevision::config::WebRtcStreamingConfig config,
        edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
        edgevision::model::viewer::RenderOutputStore& renderOutputStore
    );

} // namespace edgevision::streaming::webrtc
