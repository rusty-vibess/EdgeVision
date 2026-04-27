#pragma once

#include <cstddef>
#include <optional>

#include "model/frame/types/identity.hpp"
#include "model/scene/types/access.hpp"

namespace edgevision::builder {

    enum class BuildFailureReason {
        None,
        ViewConversionFailed,
        TrackingLost,
    };

    struct SceneBuilderRunnerStatus {
        bool running = false;
        bool stopRequested = false;
        std::size_t buildAttemptCount = 0;
        std::size_t consumedFrameCount = 0;
        std::size_t failedFrameCount = 0;
        edgevision::model::frame::FrameId lastFrameId = 0;
        std::optional<edgevision::model::scene::SceneVersionId> lastSceneVersionId{};
        std::optional<BuildFailureReason> lastFailureReason{};
    };

} // namespace edgevision::builder
