#pragma once

#include <string>

#include "model/frame/types/identity.hpp"

namespace edgevision::model::frame {

    enum class FrameSubmissionCode {
        Accepted,
        InvalidFrameId,
        InvalidTimestamp,
        FrameIdNotNewer,
        TimestampNotNewer,
        MissingRgbBuffer,
        MissingDepthBuffer,
        InvalidRgbDimensions,
        InvalidDepthDimensions,
        InvalidIntrinsics,
        PacketQueueFull,
        HistoryFull,
    };

    struct FrameSubmissionResult {
        FrameSubmissionCode code = FrameSubmissionCode::InvalidFrameId;
        FrameId frameId = 0;
        std::string message{};

        [[nodiscard]] bool accepted() const {
            return code == FrameSubmissionCode::Accepted;
        }
    };

    [[nodiscard]] const char* frameSubmissionCodeToString(FrameSubmissionCode code);

} // namespace edgevision::model::frame
