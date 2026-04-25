#pragma once

#include <cstdint>
#include <string>

namespace edgevision::streaming::webrtc {

    struct StreamConfig {
        std::uint16_t signallingPort = 6689;
        std::string signallingHost = "0.0.0.0";
        std::uint32_t width = 854;
        std::uint32_t height = 480;
        std::uint32_t fps = 30;
        std::uint32_t bitrateBps = 1500000;
        std::string stunServer = "stun://stun.l.google.com:19302";
    };

} // namespace edgevision::streaming::webrtc
