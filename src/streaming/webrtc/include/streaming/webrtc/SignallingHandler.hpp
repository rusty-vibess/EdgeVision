#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include <nlohmann/json.hpp>

namespace edgevision::streaming::webrtc {

    using SendFunc = std::function<void(const std::string&)>;

    /// Callbacks invoked from the WS server thread. All implementations
    /// must be thread-safe.
    struct SignallingCallbacks {
        std::function<void(const std::string& sdpOffer, SendFunc reply)> onOffer;
        std::function<void(const nlohmann::json& candidate)> onIce;
        std::function<void()> onClientConnected;
        std::function<void()> onClientDisconnected;
    };

    /// Single-client WebSocket listener. Wraps websocketpp.
    class SignallingHandler {
      public:
        SignallingHandler(
            std::string host,
            std::uint16_t port,
            SignallingCallbacks callbacks
        );

        SignallingHandler(const SignallingHandler&) = delete;
        SignallingHandler& operator=(const SignallingHandler&) = delete;

        ~SignallingHandler();

        void start();
        void stop();

      private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace edgevision::streaming::webrtc
