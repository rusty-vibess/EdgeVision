#include "model/frame/types/submission.hpp"

namespace edgevision::model::frame {

    const char* frameSubmissionCodeToString(FrameSubmissionCode code) {
        switch (code) {
            case FrameSubmissionCode::Accepted:
                return "accepted";
            case FrameSubmissionCode::InvalidFrameId:
                return "invalid_frame_id";
            case FrameSubmissionCode::InvalidTimestamp:
                return "invalid_timestamp";
            case FrameSubmissionCode::FrameIdNotNewer:
                return "frame_id_not_newer";
            case FrameSubmissionCode::TimestampNotNewer:
                return "timestamp_not_newer";
            case FrameSubmissionCode::MissingRgbBuffer:
                return "missing_rgb_buffer";
            case FrameSubmissionCode::MissingDepthBuffer:
                return "missing_depth_buffer";
            case FrameSubmissionCode::InvalidRgbDimensions:
                return "invalid_rgb_dimensions";
            case FrameSubmissionCode::InvalidDepthDimensions:
                return "invalid_depth_dimensions";
            case FrameSubmissionCode::InvalidIntrinsics:
                return "invalid_intrinsics";
            case FrameSubmissionCode::PacketQueueFull:
                return "packet_queue_full";
            case FrameSubmissionCode::HistoryFull:
                return "history_full";
        }

        return "unknown";
    }

} // namespace edgevision::model::frame
