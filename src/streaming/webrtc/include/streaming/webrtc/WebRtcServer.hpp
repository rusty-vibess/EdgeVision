#pragma once

#include <memory>
#include <thread>

#include "streaming/webrtc/types/StreamConfig.hpp"

namespace edgevision::model::frame { class FrameStore; }
namespace edgevision::model::scene { class SharedScene; }

namespace edgevision::streaming::webrtc {

    /// Top-level facade. Owns gstreamer pipeline + signalling handler +
    /// frame-pump threads. Construct → start() once at boot → stop() on
    /// shutdown. Single-client at a time; new client supersedes old.
    class WebRtcServer {
      public:
        WebRtcServer(
            StreamConfig config,
            edgevision::model::frame::FrameStore& frameStore,
            edgevision::model::scene::SharedScene& scene
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

    /// Convenience entry point for main.cpp. Returns a thread that owns
    /// the server lifetime; join in main on shutdown signal.
    [[nodiscard]] std::unique_ptr<WebRtcServer> startWebRtcServer(
        StreamConfig config,
        edgevision::model::frame::FrameStore& frameStore,
        edgevision::model::scene::SharedScene& scene
    );

} // namespace edgevision::streaming::webrtc
