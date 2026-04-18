#pragma once

#include "model/frame/types/camera.hpp"
#include "model/frame/types/identity.hpp"
#include "model/frame/types/image.hpp"

namespace edgevision::model::frame {

    struct Frame {
        FrameId frameId = 0;
        FrameTimestamp timestamp{};
        FrameImage rgb{};
        FrameColorFormat colorFormat = FrameColorFormat::Unknown;
        FrameImage depth{};
        FrameDepthFormat depthFormat = FrameDepthFormat::Unknown;
        CameraIntrinsics intrinsics{};
        CameraConfig cameraConfig{};
    };

} // namespace edgevision::model::frame
