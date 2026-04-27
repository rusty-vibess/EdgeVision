#pragma once

#include <array>

namespace edgevision::streaming::webrtc {

    /// Pose received over the DataChannel. The payload is pose-only.
    struct PoseUpdate {
        std::array<float, 16> matrix{};
    };

} // namespace edgevision::streaming::webrtc
