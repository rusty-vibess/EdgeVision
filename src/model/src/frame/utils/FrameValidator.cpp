#include "frame/utils/FrameValidator.hpp"

#include <cmath>
#include <cstdint>
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

        [[nodiscard]] bool hasSaneImageBytes(const FrameImage& image) {
            if (!isValidSize(image.size) || image.strideBytes <= 0 || image.buffer.empty()) {
                return false;
            }

            if (image.strideBytes < image.size.width) {
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

        if (!hasSaneImageBytes(frame.rgb)) {
            return makeResult(
                FrameSubmissionCode::InvalidRgbDimensions,
                frame.frameId,
                "Frame RGB dimensions, stride, or byte count are invalid"
            );
        }

        if (!hasSaneImageBytes(frame.depth)) {
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
