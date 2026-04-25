#pragma once

#include <array>
#include <cstdint>

namespace edgevision::streaming::webrtc {

    /// Pose received over the DataChannel. Matrix is column-major c2w in
    /// Colmap convention — exactly what edgevision-mobile sends after
    /// `Pose.toColumnMajorMatrix()`.
    struct PoseUpdate {
        std::uint64_t seq = 0;
        std::array<float, 16> matrix{};
        std::uint32_t width = 854;
        std::uint32_t height = 480;
        float fovX = 1.0472f;
        float fovY = 0.6435f;
    };

    enum class ViewMode { Rgb, Tsdf, Dual };

} // namespace edgevision::streaming::webrtc
