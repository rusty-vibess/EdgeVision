#pragma once

#include <optional>
#include <string>

#include "capture/frame/types/build_result.hpp"
#include "model/frame/types/identity.hpp"
#include "model/frame/types/submission.hpp"

namespace edgevision::capture::frame {

    enum class FrameIngestCode {
        Submitted,
        CaptureUnavailable,
        CalibrationUnavailable,
        BuildFailed,
        ModelRejected,
    };

    struct FrameIngestResult {
        FrameIngestCode code = FrameIngestCode::CaptureUnavailable;
        edgevision::model::frame::FrameId frameId = 0;
        std::string message{};
        std::optional<FrameBuildCode> buildCode{};
        std::optional<edgevision::model::frame::FrameSubmissionCode> submissionCode{};

        [[nodiscard]] bool submitted() const {
            return code == FrameIngestCode::Submitted;
        }
    };

} // namespace edgevision::capture::frame
