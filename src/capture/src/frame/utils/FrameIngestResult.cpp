#include "frame/utils/FrameIngestResult.hpp"

#include <optional>
#include <utility>

namespace edgevision::capture::frame {

    FrameIngestResult makeFrameIngestResult(
        FrameIngestCode code,
        edgevision::model::frame::FrameId frameId,
        std::string message
    ) {
        return FrameIngestResult{code, frameId, std::move(message), std::nullopt, std::nullopt};
    }

    FrameIngestResult makeBuildFailedResult(
        edgevision::model::frame::FrameId frameId,
        const FrameBuildResult& buildResult
    ) {
        FrameIngestResult result{};
        result.code = FrameIngestCode::BuildFailed;
        result.frameId = frameId;
        result.message = buildResult.message;
        result.buildCode = buildResult.code;
        return result;
    }

    FrameIngestResult makeModelRejectedResult(
        const edgevision::model::frame::FrameSubmissionResult& submissionResult
    ) {
        FrameIngestResult result{};
        result.code = FrameIngestCode::ModelRejected;
        result.frameId = submissionResult.frameId;
        result.message = submissionResult.message;
        result.submissionCode = submissionResult.code;
        return result;
    }

    FrameIngestResult makeSubmittedResult(
        const edgevision::model::frame::FrameSubmissionResult& submissionResult
    ) {
        FrameIngestResult result{};
        result.code = FrameIngestCode::Submitted;
        result.frameId = submissionResult.frameId;
        result.message = submissionResult.message;
        result.submissionCode = submissionResult.code;
        return result;
    }

} // namespace edgevision::capture::frame
