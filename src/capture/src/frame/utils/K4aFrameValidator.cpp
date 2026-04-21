#include "frame/utils/K4aFrameValidator.hpp"

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "frame/utils/FrameBuildResult.hpp"
#include "model/frame/types/image.hpp"

namespace edgevision::capture::frame {
    namespace {
        constexpr int kMaxSaneImageDimension = 16384;

        using edgevision::model::frame::CameraIntrinsics;
        using edgevision::model::frame::FrameId;
        using edgevision::model::frame::FrameTimestamp;
        using edgevision::model::frame::ImageSize;

        [[nodiscard]] bool isValidSize(const ImageSize& size) {
            return size.width > 0 && size.height > 0 && size.width <= kMaxSaneImageDimension
                && size.height <= kMaxSaneImageDimension;
        }

        [[nodiscard]] bool isValidTimestamp(const FrameTimestamp& timestamp) {
            return timestamp.ticks > 0;
        }

        [[nodiscard]] bool isSaneImageShape(
            const K4aInterface& api,
            k4a_image_t image,
            int bytesPerPixel
        ) {
            const ImageSize size{api.imageGetWidthPixels(image), api.imageGetHeightPixels(image)};
            const int strideBytes = api.imageGetStrideBytes(image);
            const std::size_t byteCount = api.imageGetSize(image);
            const std::uint8_t* data = api.imageGetBuffer(image);

            if (!isValidSize(size) || strideBytes <= 0 || byteCount == 0 || data == nullptr) {
                return false;
            }

            const auto minStride =
                static_cast<std::int64_t>(size.width) * static_cast<std::int64_t>(bytesPerPixel);
            if (strideBytes < minStride) {
                return false;
            }

            const auto requiredBytes =
                static_cast<std::uint64_t>(strideBytes) * static_cast<std::uint64_t>(size.height);
            return requiredBytes <= byteCount;
        }

        [[nodiscard]] bool isValidIntrinsics(const CameraIntrinsics& intrinsics) {
            return std::isfinite(intrinsics.fx) && std::isfinite(intrinsics.fy)
                && std::isfinite(intrinsics.cx) && std::isfinite(intrinsics.cy)
                && intrinsics.fx > 0.0f && intrinsics.fy > 0.0f && intrinsics.cx >= 0.0f
                && intrinsics.cy >= 0.0f;
        }

    } // namespace

    std::optional<FrameBuildResult> K4aFrameValidator::validateRequest(
        k4a_capture_t capture,
        FrameId frameId,
        FrameTimestamp timestamp
    ) {
        if (frameId == 0) {
            return makeFrameBuildFailure(
                FrameBuildCode::InvalidFrameId, frameId, "Frame id must be non-zero"
            );
        }

        if (!isValidTimestamp(timestamp)) {
            return makeFrameBuildFailure(
                FrameBuildCode::InvalidTimestamp, frameId, "Frame timestamp must be positive"
            );
        }

        if (capture == nullptr) {
            return makeFrameBuildFailure(
                FrameBuildCode::MissingColorImage, frameId, "K4A capture handle is missing"
            );
        }

        return std::nullopt;
    }

    std::optional<FrameBuildResult> K4aFrameValidator::validateColorImage(
        const K4aInterface& api,
        k4a_image_t image,
        FrameId frameId
    ) {
        if (image == nullptr) {
            return makeFrameBuildFailure(
                FrameBuildCode::MissingColorImage, frameId, "K4A capture has no color image"
            );
        }

        if (api.imageGetFormat(image) != K4A_IMAGE_FORMAT_COLOR_BGRA32) {
            return makeFrameBuildFailure(
                FrameBuildCode::UnsupportedColorFormat,
                frameId,
                "K4A color image format is not BGRA32"
            );
        }

        if (!isSaneImageShape(api, image, 4)) {
            return makeFrameBuildFailure(
                FrameBuildCode::InvalidColorImage,
                frameId,
                "K4A color image dimensions, stride, or byte count are invalid"
            );
        }

        return std::nullopt;
    }

    std::optional<FrameBuildResult> K4aFrameValidator::validateDepthImage(
        const K4aInterface& api,
        k4a_image_t image,
        FrameId frameId
    ) {
        if (image == nullptr) {
            return makeFrameBuildFailure(
                FrameBuildCode::MissingDepthImage, frameId, "K4A capture has no depth image"
            );
        }

        if (api.imageGetFormat(image) != K4A_IMAGE_FORMAT_DEPTH16) {
            return makeFrameBuildFailure(
                FrameBuildCode::UnsupportedDepthFormat,
                frameId,
                "K4A depth image format is not DEPTH16"
            );
        }

        if (!isSaneImageShape(api, image, 2)) {
            return makeFrameBuildFailure(
                FrameBuildCode::InvalidDepthImage,
                frameId,
                "K4A depth image dimensions, stride, or byte count are invalid"
            );
        }

        return std::nullopt;
    }

    std::optional<FrameBuildResult> K4aFrameValidator::validateIntrinsics(
        const CameraIntrinsics& intrinsics,
        FrameId frameId
    ) {
        if (!isValidIntrinsics(intrinsics)) {
            return makeFrameBuildFailure(
                FrameBuildCode::InvalidIntrinsics,
                frameId,
                "K4A color camera intrinsics are invalid"
            );
        }

        return std::nullopt;
    }

} // namespace edgevision::capture::frame
