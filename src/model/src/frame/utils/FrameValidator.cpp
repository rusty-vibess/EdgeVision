#include "frame/utils/FrameValidator.hpp"

#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>

namespace edgevision::model::frame {
    namespace {
        constexpr int kMaxSaneImageDimension = 16384;

        [[nodiscard]] FrameSubmissionResult makeResult(
            FrameSubmissionCode code,
            FrameId frameId,
            std::string message
        ) {
            return FrameSubmissionResult{code, frameId, std::move(message)};
        }

        [[nodiscard]] bool isValidSize(const ImageSize& size) {
            return size.width > 0 && size.height > 0 && size.width <= kMaxSaneImageDimension
                && size.height <= kMaxSaneImageDimension;
        }

        [[nodiscard]] bool isValidTimestamp(const FrameTimestamp& timestamp) {
            return timestamp.ticks > 0;
        }

        [[nodiscard]] std::optional<int> bytesPerColorPixel(FrameColorFormat format) {
            switch (format) {
                case FrameColorFormat::Rgb8:
                    return 3;
                case FrameColorFormat::Bgra32:
                    return 4;
                case FrameColorFormat::Unknown:
                    return std::nullopt;
            }

            return std::nullopt;
        }

        [[nodiscard]] std::optional<int> bytesPerDepthPixel(FrameDepthFormat format) {
            switch (format) {
                case FrameDepthFormat::Depth16Millimeters:
                    return 2;
                case FrameDepthFormat::Unknown:
                    return std::nullopt;
            }

            return std::nullopt;
        }

        [[nodiscard]] bool hasSaneImageBytes(const FrameImage& image, int bytesPerPixel) {
            if (!isValidSize(image.size) || image.strideBytes <= 0 || image.buffer.empty()) {
                return false;
            }

            const auto minStride = static_cast<std::int64_t>(image.size.width)
                * static_cast<std::int64_t>(bytesPerPixel);
            if (image.strideBytes < minStride) {
                return false;
            }

            const auto requiredBytes = static_cast<std::uint64_t>(image.strideBytes)
                * static_cast<std::uint64_t>(image.size.height);
            return requiredBytes <= image.buffer.byteCount;
        }

        [[nodiscard]] bool isValidIntrinsics(const CameraIntrinsics& intrinsics) {
            return std::isfinite(intrinsics.fx) && std::isfinite(intrinsics.fy)
                && std::isfinite(intrinsics.cx) && std::isfinite(intrinsics.cy)
                && intrinsics.fx > 0.0f && intrinsics.fy > 0.0f && intrinsics.cx >= 0.0f
                && intrinsics.cy >= 0.0f;
        }

        [[nodiscard]] bool isValidCameraConfig(const CameraConfig& config, const Frame& frame) {
            return config.rgbResolution == frame.rgb.size
                && config.depthResolution == frame.depth.size
                && config.colorFormat == frame.colorFormat
                && config.depthFormat == frame.depthFormat && config.depthScaleToMeters > 0.0f
                && std::isfinite(config.depthScaleToMeters) && config.frameRateFps >= 0;
        }
    } // namespace

    FrameSubmissionResult FrameValidator::validate(
        const Frame& frame,
        const FrameValidationContext& context
    ) {
        if (frame.frameId == 0) {
            return makeResult(
                FrameSubmissionCode::InvalidFrameId, frame.frameId, "Frame id must be non-zero"
            );
        }

        if (!isValidTimestamp(frame.timestamp)) {
            return makeResult(
                FrameSubmissionCode::InvalidTimestamp,
                frame.frameId,
                "Frame timestamp must be positive"
            );
        }

        if (context.latestFrameId.has_value() && frame.frameId <= *context.latestFrameId) {
            return makeResult(
                FrameSubmissionCode::FrameIdNotNewer,
                frame.frameId,
                "Frame id is not newer than the latest accepted frame"
            );
        }

        if (context.latestTimestamp.has_value()
            && frame.timestamp.ticks <= context.latestTimestamp->ticks) {
            return makeResult(
                FrameSubmissionCode::TimestampNotNewer,
                frame.frameId,
                "Frame timestamp is not newer than the latest accepted frame"
            );
        }

        const std::optional<int> colorBytesPerPixel = bytesPerColorPixel(frame.colorFormat);
        if (!colorBytesPerPixel.has_value()) {
            return makeResult(
                FrameSubmissionCode::UnsupportedColorFormat,
                frame.frameId,
                "Frame color format is unsupported"
            );
        }

        const std::optional<int> depthBytesPerPixel = bytesPerDepthPixel(frame.depthFormat);
        if (!depthBytesPerPixel.has_value()) {
            return makeResult(
                FrameSubmissionCode::UnsupportedDepthFormat,
                frame.frameId,
                "Frame depth format is unsupported"
            );
        }

        if (frame.rgb.buffer.empty()) {
            return makeResult(
                FrameSubmissionCode::MissingRgbBuffer, frame.frameId, "Frame RGB buffer is missing"
            );
        }

        if (frame.depth.buffer.empty()) {
            return makeResult(
                FrameSubmissionCode::MissingDepthBuffer,
                frame.frameId,
                "Frame depth buffer is missing"
            );
        }

        if (!hasSaneImageBytes(frame.rgb, *colorBytesPerPixel)) {
            return makeResult(
                FrameSubmissionCode::InvalidRgbDimensions,
                frame.frameId,
                "Frame RGB dimensions, stride, or byte count are invalid"
            );
        }

        if (!hasSaneImageBytes(frame.depth, *depthBytesPerPixel)) {
            return makeResult(
                FrameSubmissionCode::InvalidDepthDimensions,
                frame.frameId,
                "Frame depth dimensions, stride, or byte count are invalid"
            );
        }

        if (!isValidIntrinsics(frame.intrinsics)) {
            return makeResult(
                FrameSubmissionCode::InvalidIntrinsics,
                frame.frameId,
                "Frame camera intrinsics are invalid"
            );
        }

        if (!isValidCameraConfig(frame.cameraConfig, frame)) {
            return makeResult(
                FrameSubmissionCode::InvalidCameraConfig,
                frame.frameId,
                "Frame camera configuration does not match the frame"
            );
        }

        if (context.historyFull && !context.hasEvictableFrame) {
            return makeResult(
                FrameSubmissionCode::HistoryFull,
                frame.frameId,
                "Frame history is full and no stored frame can be evicted"
            );
        }

        return makeResult(FrameSubmissionCode::Accepted, frame.frameId, "Frame accepted");
    }

} // namespace edgevision::model::frame
