#include "frame/utils/K4aFrameAssembler.hpp"

#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

namespace edgevision::capture::frame {
    namespace {
        constexpr float kDepthScaleToMeters = 0.001f;

        using edgevision::model::frame::CameraConfig;
        using edgevision::model::frame::CameraIntrinsics;
        using edgevision::model::frame::Frame;
        using edgevision::model::frame::FrameBuffer;
        using edgevision::model::frame::FrameColorFormat;
        using edgevision::model::frame::FrameDepthFormat;
        using edgevision::model::frame::FrameId;
        using edgevision::model::frame::FrameImage;
        using edgevision::model::frame::FrameTimestamp;
        using edgevision::model::frame::ImageSize;

        [[nodiscard]] int frameRateFps(k4a_fps_t frameRate) {
            switch (frameRate) {
                case K4A_FRAMES_PER_SECOND_5:
                    return 5;
                case K4A_FRAMES_PER_SECOND_15:
                    return 15;
                case K4A_FRAMES_PER_SECOND_30:
                    return 30;
            }

            return 0;
        }
    } // namespace

    CameraIntrinsics K4aFrameAssembler::makeIntrinsics(const k4a_calibration_t& calibration) {
        const auto& params = calibration.color_camera_calibration.intrinsics.parameters.param;
        return CameraIntrinsics{params.fx, params.fy, params.cx, params.cy};
    }

    Frame K4aFrameAssembler::assemble(
        const K4aInterface& api,
        k4a_image_t colorImage,
        k4a_image_t depthImage,
        CameraIntrinsics intrinsics,
        const k4a_device_configuration_t& config,
        FrameId frameId,
        FrameTimestamp timestamp
    ) {
        Frame frame{};
        frame.frameId = frameId;
        frame.timestamp = timestamp;
        frame.rgb = makeFrameImage(api, colorImage);
        frame.colorFormat = FrameColorFormat::Bgra32;
        frame.depth = makeFrameImage(api, depthImage);
        frame.depthFormat = FrameDepthFormat::Depth16Millimeters;
        frame.intrinsics = intrinsics;
        frame.cameraConfig = makeCameraConfig(frame, config);
        return frame;
    }

    FrameImage K4aFrameAssembler::makeFrameImage(const K4aInterface& api, k4a_image_t image) {
        FrameImage frameImage{};
        frameImage.size =
            ImageSize{api.imageGetWidthPixels(image), api.imageGetHeightPixels(image)};
        frameImage.strideBytes = api.imageGetStrideBytes(image);
        frameImage.buffer = copyImageBuffer(api, image);
        return frameImage;
    }

    FrameBuffer K4aFrameAssembler::copyImageBuffer(const K4aInterface& api, k4a_image_t image) {
        const std::size_t byteCount = api.imageGetSize(image);
        const auto* source = reinterpret_cast<const std::byte*>(api.imageGetBuffer(image));
        auto storage = std::make_shared<std::vector<std::byte>>(byteCount);
        std::memcpy(storage->data(), source, byteCount);
        return FrameBuffer(storage->data(), storage->size(), storage);
    }

    CameraConfig K4aFrameAssembler::makeCameraConfig(
        const Frame& frame,
        const k4a_device_configuration_t& config
    ) {
        CameraConfig cameraConfig{};
        cameraConfig.rgbResolution = frame.rgb.size;
        cameraConfig.depthResolution = frame.depth.size;
        cameraConfig.colorFormat = frame.colorFormat;
        cameraConfig.depthFormat = frame.depthFormat;
        cameraConfig.depthScaleToMeters = kDepthScaleToMeters;
        cameraConfig.frameRateFps = frameRateFps(config.camera_fps);
        cameraConfig.depthDelayOffColorUsec = config.depth_delay_off_color_usec;
        cameraConfig.synchronizedImagesOnly = config.synchronized_images_only;
        return cameraConfig;
    }

} // namespace edgevision::capture::frame
