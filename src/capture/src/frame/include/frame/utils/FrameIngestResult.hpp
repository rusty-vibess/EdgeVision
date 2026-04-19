#pragma once

#include <string>

#include "capture/frame/types/build_result.hpp"
#include "capture/frame/types/ingest_result.hpp"
#include "model/frame/types/identity.hpp"
#include "model/frame/types/submission.hpp"

namespace edgevision::capture::frame {

    [[nodiscard]] FrameIngestResult makeFrameIngestResult(
        FrameIngestCode code,
        edgevision::model::frame::FrameId frameId,
        std::string message
    );

    [[nodiscard]] FrameIngestResult makeBuildFailedResult(
        edgevision::model::frame::FrameId frameId,
        const FrameBuildResult& buildResult
    );

    [[nodiscard]] FrameIngestResult makeModelRejectedResult(
        const edgevision::model::frame::FrameSubmissionResult& submissionResult
    );

    [[nodiscard]] FrameIngestResult makeSubmittedResult(
        const edgevision::model::frame::FrameSubmissionResult& submissionResult
    );

} // namespace edgevision::capture::frame
