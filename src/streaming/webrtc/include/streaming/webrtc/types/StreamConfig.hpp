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
        // Set false for TSDF-only mode (drops RGB NVENC encoder, ~1 W saving on Jetson)
        bool enableRgb = true;
        // CPU core affinity for pump threads. -1 = OS default.
        // Jetson Orin Nano (6 cores): SLAM/capture typically owns 0-3;
        // pin WebRTC to 4-5 to avoid stealing cycles from reconstruction.
        int pumpRgbCore  = 4;
        int pumpTsdfCore = 5;
    };

} // namespace edgevision::streaming::webrtc
