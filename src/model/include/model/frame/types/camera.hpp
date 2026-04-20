#pragma once

#include "model/frame/types/image.hpp"

namespace edgevision::model::frame {

    struct CameraIntrinsics {
        float fx = 0.0f;
        float fy = 0.0f;
        float cx = 0.0f;
        float cy = 0.0f;
    };

    struct CameraConfig {
        ImageSize rgbResolution{};
        ImageSize depthResolution{};
        FrameColorFormat colorFormat = FrameColorFormat::Unknown;
        FrameDepthFormat depthFormat = FrameDepthFormat::Unknown;
        float depthScaleToMeters = 0.0f;
        int frameRateFps = 0;
        int depthDelayOffColorUsec = 0;
        bool synchronizedImagesOnly = false;
    };

} // namespace edgevision::model::frame
