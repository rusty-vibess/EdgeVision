#pragma once

#include "model/frame/types/camera.hpp"
#include "model/frame/types/identity.hpp"
#include "model/frame/types/image.hpp"

namespace edgevision::model::frame {

    struct Frame {
        FrameId frameId = 0;
        FrameTimestamp timestamp{};
        FrameImage rgb{};
        FrameImage depth{};
        CameraIntrinsics intrinsics{};
    };

} // namespace edgevision::model::frame
