#pragma once

#include <cstddef>
#include <optional>

#include "model/scene/types/access.hpp"
#include "model/viewer/types/identity.hpp"

namespace edgevision::viewer {

    /// Summarizes the scene viewer runner's current loop activity.
    enum class SceneViewerRunnerActivity {
        Idle,
        WaitingForPose,
        RenderedOutput,
    };

    /// Captures the latest observable status of the background scene viewer loop.
    struct SceneViewerRunnerStatus {
        /// Whether the runner thread is currently active.
        bool running = false;
        /// Whether a stop has been requested.
        bool stopRequested = false;
        /// The most recent high-level activity observed in the loop.
        SceneViewerRunnerActivity activity = SceneViewerRunnerActivity::Idle;
        /// Total number of render attempts made by the loop.
        std::size_t renderAttemptCount = 0;
        /// Total number of outputs published by the loop.
        std::size_t publishedOutputCount = 0;
        /// Total number of published outputs marked cached.
        std::size_t cachedOutputCount = 0;
        /// Total number of idle loop iterations observed.
        std::size_t idleIterationCount = 0;
        /// The last pose generation the loop attempted or published.
        std::optional<edgevision::model::viewer::ViewerPoseGeneration> lastPoseGeneration{};
        /// The last scene version rendered by the loop.
        std::optional<edgevision::model::scene::SceneVersionId> lastSceneVersionId{};
        /// Whether the last published output was marked cached.
        bool lastOutputWasCached = false;
    };

} // namespace edgevision::viewer
