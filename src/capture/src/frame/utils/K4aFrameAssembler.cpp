#include "frame/utils/K4aFrameAssembler.hpp"

#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

namespace edgevision::capture::frame {
    namespace {
        using edgevision::model::frame::CameraIntrinsics;
        using edgevision::model::frame::Frame;
        using edgevision::model::frame::FrameBuffer;
        using edgevision::model::frame::FrameId;
        using edgevision::model::frame::FrameImage;
        using edgevision::model::frame::FrameTimestamp;
        using edgevision::model::frame::ImageSize;
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
        FrameId frameId,
        FrameTimestamp timestamp
    ) {
        Frame frame{};
        frame.frameId = frameId;
        frame.timestamp = timestamp;
        frame.rgb = makeFrameImage(api, colorImage);
        frame.depth = makeFrameImage(api, depthImage);
        frame.intrinsics = intrinsics;
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

} // namespace edgevision::capture::frame
