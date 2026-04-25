#pragma once

#include <optional>

#include "builder/types/runner.hpp"
#include "model/frame/types/frame.hpp"
#include "model/scene/types/access.hpp"

namespace edgevision::builder {

    enum class FrameBuildStatus {
        FrameConsumed,
        FrameFailed,
    };

    struct FrameBuildResult {
        FrameBuildStatus status = FrameBuildStatus::FrameFailed;
        BuildFailureReason failureReason = BuildFailureReason::None;
        edgevision::model::frame::FrameId frameId = 0;
        std::optional<edgevision::model::scene::SceneVersionId> sceneVersionId{};
    };

} // namespace edgevision::builder
