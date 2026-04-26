#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace edgevision::streaming::webrtc {

    struct StreamConfig {
        std::uint16_t signallingPort = 6689;
        std::string signallingHost = "0.0.0.0";
        std::uint32_t width = 854;
        std::uint32_t height = 480;
        std::uint32_t fps = 30;
        std::uint32_t bitrateBps = 1500000;
        std::string stunServer = "stun://stun.l.google.com:19302";
        // Default false: RGB goes via the existing TCP RenderServer (port 6688)
        // to keep the single-stream software-x264 budget free for TSDF.
        // Orin Nano has no NVENC — encoding is CPU-bound on cores 4-5.
        bool enableRgb = false;
        // Shared CPU mask for both pump threads. Jetson Orin Nano: leave
        // cores 0-3 for SLAM/builder/reconstruction, pin WebRTC to 4-5.
        // Empty mask = OS default scheduling.
        std::vector<int> pumpCoreMask = {4, 5};
    };

} // namespace edgevision::streaming::webrtc
