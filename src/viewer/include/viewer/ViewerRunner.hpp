#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include "config/types/viewer.hpp"
#include "viewer/types/runner.hpp"

namespace edgevision::model::viewer {
    struct RenderOutput;
    struct ViewerPose;
    class ViewerPoseStore;
} // namespace edgevision::model::viewer

namespace edgevision::viewer {

    class SceneViewer;

    /// Runs a background loop that renders viewer outputs according to runtime policy.
    class ViewerRunner final {
      public:
        ViewerRunner(
            SceneViewer& viewer,
            model::viewer::ViewerPoseStore& viewerPoseStore,
            const config::ViewerRuntimeConfig& config = {}
        );
        ~ViewerRunner();

        ViewerRunner(const ViewerRunner&) = delete;
        ViewerRunner& operator=(const ViewerRunner&) = delete;

        /// Starts the background viewer loop.
        [[nodiscard]] bool start();
        /// Requests that the background viewer loop stop.
        void requestStop();
        /// Waits for the background viewer loop to exit.
        void join();

        /// Returns whether the background viewer loop is currently running.
        [[nodiscard]] bool running() const;
        /// Returns the latest viewer runner status snapshot.
        [[nodiscard]] ViewerRunnerStatus status() const;

      private:
        void runLoop();
        void recordIdle(ViewerRunnerActivity activity);
        void recordRenderAttempt(const model::viewer::ViewerPose& viewerPose);
        void recordOutput(const model::viewer::RenderOutput& output);
        void setRunning(bool running);

        SceneViewer& m_viewer;
        model::viewer::ViewerPoseStore& m_viewerPoseStore;
        config::ViewerRuntimeConfig m_config{};
        std::atomic_bool m_stopRequested{false};
        std::atomic_bool m_running{false};
        mutable std::mutex m_statusMutex{};
        std::thread m_thread{};
        ViewerRunnerStatus m_status{};
    };

} // namespace edgevision::viewer
