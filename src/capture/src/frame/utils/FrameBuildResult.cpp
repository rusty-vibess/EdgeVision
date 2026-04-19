#include "frame/utils/FrameBuildResult.hpp"

#include <optional>
#include <utility>

namespace edgevision::capture::frame {

    FrameBuildResult makeFrameBuildFailure(
        FrameBuildCode code,
        edgevision::model::frame::FrameId frameId,
        std::string message
    ) {
        return FrameBuildResult{code, frameId, std::move(message), std::nullopt};
    }

    FrameBuildResult makeFrameBuildSuccess(edgevision::model::frame::Frame frame) {
        const edgevision::model::frame::FrameId frameId = frame.frameId;
        return FrameBuildResult{FrameBuildCode::Built, frameId, "Frame built", std::move(frame)};
    }

} // namespace edgevision::capture::frame
