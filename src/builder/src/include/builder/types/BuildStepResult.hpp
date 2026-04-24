#pragma once

#include <optional>

#include "model/frame/types/identity.hpp"
#include "model/scene/types/access.hpp"

namespace edgevision::builder {

    enum class BuildStepStatus {
        NoFrameAvailable,
        FrameConsumed,
        FrameFailed,
    };

    enum class BuildFailureReason {
        None,
        ViewConversionFailed,
        TrackingLost,
    };

    struct BuildStepResult {
        BuildStepStatus status = BuildStepStatus::NoFrameAvailable;
        BuildFailureReason failureReason = BuildFailureReason::None;
        edgevision::model::frame::FrameId frameId = 0;
        std::optional<edgevision::model::scene::SceneVersionId> sceneVersionId{};
    };

} // namespace edgevision::builder
