#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "config/types/viewer.hpp"
#include "viewer/types/runner.hpp"

namespace edgevision::model::scene {
    class SharedScene;
} // namespace edgevision::model::scene

namespace edgevision::model::viewer {
    struct RenderOutput;
    struct ViewerPose;
    class RenderOutputStore;
    class ViewerPoseStore;
} // namespace edgevision::model::viewer

namespace edgevision::viewer {

    class SceneViewer;

    /// Runs a background loop that renders scene viewer outputs according to runtime policy.
    class SceneViewerRunner final {
      public:
        SceneViewerRunner(
            model::viewer::ViewerPoseStore& viewerPoseStore,
            model::scene::SharedScene& sharedScene,
            model::viewer::RenderOutputStore& renderOutputStore,
            const config::ViewerRuntimeConfig& config = {}
        );
        ~SceneViewerRunner();

        SceneViewerRunner(const SceneViewerRunner&) = delete;
        SceneViewerRunner& operator=(const SceneViewerRunner&) = delete;
        SceneViewerRunner(SceneViewerRunner&& other) noexcept = delete;
        SceneViewerRunner& operator=(SceneViewerRunner&& other) noexcept = delete;

        /// Starts the background viewer loop.
        [[nodiscard]] bool start();
        /// Requests that the background viewer loop stop.
        void requestStop();
        /// Waits for the background viewer loop to exit.
        void join();

        /// Returns whether the background viewer loop is currently running.
        [[nodiscard]] bool running() const;
        /// Returns the latest scene viewer runner status snapshot.
        [[nodiscard]] SceneViewerRunnerStatus status() const;

      private:
        void runLoop();
        void recordIdle(SceneViewerRunnerActivity activity);
        void recordRenderAttempt(const model::viewer::ViewerPose& viewerPose);
        void recordOutput(const model::viewer::RenderOutput& output);
        void setRunning(bool running);

        model::viewer::ViewerPoseStore& m_viewerPoseStore;
        std::unique_ptr<SceneViewer> m_viewer{};
        config::ViewerRuntimeConfig m_config{};
        std::atomic_bool m_stopRequested{false};
        std::atomic_bool m_running{false};
        mutable std::mutex m_statusMutex{};
        std::thread m_thread{};
        SceneViewerRunnerStatus m_status{};
    };

} // namespace edgevision::viewer
