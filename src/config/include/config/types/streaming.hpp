#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace edgevision::config {

    struct TcpStreamingConfig {
        bool enabled = false;
        int port = 6688;
    };

    struct WebRtcStreamingConfig {
        bool enabled = true;
        std::string signallingHost = "0.0.0.0";
        std::uint16_t signallingPort = 6689;
        std::uint32_t fps = 30;
        std::uint32_t bitrateBps = 1500000;
        std::string stunServer{};
        std::vector<int> pumpCoreMask = {4, 5};
    };

    struct StreamingConfig {
        TcpStreamingConfig tcp{};
        WebRtcStreamingConfig webrtc{};
    };

} // namespace edgevision::config
