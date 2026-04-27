#pragma once

#include "model/frame/types/camera.hpp"
#include "model/frame/types/image.hpp"
#include "model/scene/types/version.hpp"
#include "model/viewer/types/identity.hpp"

namespace edgevision::model::viewer {

    /// Describes a single viewer render request.
    struct ViewerPose {
        /// Camera-to-world pose used for free-view rendering.
        edgevision::model::scene::Pose4f pose{};
        /// Pinhole intrinsics used for the requested render.
        edgevision::model::frame::CameraIntrinsics intrinsics{};
        /// Requested RGB output size.
        edgevision::model::frame::ImageSize imageSize{};
        /// Monotonic update generation used for change detection.
        ViewerPoseGeneration generation = 0;
    };

    [[nodiscard]] constexpr bool operator==(const ViewerPose& lhs, const ViewerPose& rhs) {
        return lhs.pose == rhs.pose && lhs.intrinsics.fx == rhs.intrinsics.fx
            && lhs.intrinsics.fy == rhs.intrinsics.fy && lhs.intrinsics.cx == rhs.intrinsics.cx
            && lhs.intrinsics.cy == rhs.intrinsics.cy && lhs.imageSize == rhs.imageSize
            && lhs.generation == rhs.generation;
    }

    [[nodiscard]] constexpr bool operator!=(const ViewerPose& lhs, const ViewerPose& rhs) {
        return !(lhs == rhs);
    }

} // namespace edgevision::model::viewer
