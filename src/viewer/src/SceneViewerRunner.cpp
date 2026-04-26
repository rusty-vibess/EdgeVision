#include "viewer/SceneViewerRunner.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>

#include "model/viewer/ViewerPoseStore.hpp"
#include "viewer/state/SceneViewer.hpp"

namespace edgevision::viewer {
    namespace {
        [[nodiscard]] std::chrono::milliseconds makeLoopPeriod(
            const edgevision::config::ViewerRuntimeConfig& config
        ) {
            return std::chrono::milliseconds(std::max(config.loopPeriodMs, 1));
        }
    } // namespace

    SceneViewerRunner::SceneViewerRunner(
        edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
        edgevision::model::scene::SharedScene& sharedScene,
        edgevision::model::viewer::RenderOutputStore& renderOutputStore,
        const edgevision::config::ViewerRuntimeConfig& config
    )
        : m_viewerPoseStore(viewerPoseStore),
          m_viewer(std::make_unique<SceneViewer>(sharedScene, renderOutputStore)),
          m_config(config) {}

    SceneViewerRunner::~SceneViewerRunner() {
        requestStop();
        join();
    }

    bool SceneViewerRunner::start() {
        if (m_thread.joinable() || m_running.load()) {
            return false;
        }

        m_stopRequested.store(false);
        setRunning(true);
        m_thread = std::thread(&SceneViewerRunner::runLoop, this);
        return true;
    }

    void SceneViewerRunner::requestStop() {
        m_stopRequested.store(true);
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_status.stopRequested = true;
    }

    void SceneViewerRunner::join() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    bool SceneViewerRunner::running() const {
        return m_running.load();
    }

    SceneViewerRunnerStatus SceneViewerRunner::status() const {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        return m_status;
    }

    void SceneViewerRunner::runLoop() {
        const std::chrono::milliseconds loopPeriod = makeLoopPeriod(m_config);
        edgevision::model::viewer::ViewerPoseGeneration lastPoseGeneration = 0;

        while (!m_stopRequested.load()) {
            if (m_config.loopPolicy == edgevision::config::ViewerLoopPolicy::PoseDriven) {
                const std::optional<edgevision::model::viewer::ViewerPose> viewerPose =
                    m_viewerPoseStore.waitForNewer(lastPoseGeneration, loopPeriod);
                if (!viewerPose.has_value()) {
                    recordIdle(SceneViewerRunnerActivity::WaitingForPose);
                    continue;
                }

                recordRenderAttempt(*viewerPose);
                const std::optional<edgevision::model::viewer::RenderOutput> output =
                    m_viewer->renderPose(*viewerPose);
                if (!output.has_value()) {
                    recordIdle(SceneViewerRunnerActivity::WaitingForPose);
                    continue;
                }

                lastPoseGeneration = output->poseGeneration;
                recordOutput(*output);
                continue;
            }

            const auto loopStart = std::chrono::steady_clock::now();
            const std::optional<edgevision::model::viewer::ViewerPose> viewerPose =
                m_viewerPoseStore.latest();
            if (!viewerPose.has_value()) {
                recordIdle(SceneViewerRunnerActivity::Idle);
            } else {
                recordRenderAttempt(*viewerPose);
                const std::optional<edgevision::model::viewer::RenderOutput> output =
                    m_viewer->renderPose(*viewerPose);
                if (!output.has_value()) {
                    recordIdle(SceneViewerRunnerActivity::Idle);
                } else {
                    lastPoseGeneration = output->poseGeneration;
                    recordOutput(*output);
                }
            }

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - loopStart
            );
            if (elapsed < loopPeriod) {
                std::this_thread::sleep_for(loopPeriod - elapsed);
            }
        }

        setRunning(false);
    }

    void SceneViewerRunner::recordIdle(SceneViewerRunnerActivity activity) {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        ++m_status.idleIterationCount;
        m_status.activity = activity;
    }

    void SceneViewerRunner::recordRenderAttempt(
        const edgevision::model::viewer::ViewerPose& viewerPose
    ) {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        ++m_status.renderAttemptCount;
        m_status.lastPoseGeneration = viewerPose.generation;
    }

    void SceneViewerRunner::recordOutput(const edgevision::model::viewer::RenderOutput& output) {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        ++m_status.publishedOutputCount;
        if (output.stale) {
            ++m_status.staleOutputCount;
        }

        m_status.activity = SceneViewerRunnerActivity::RenderedOutput;
        m_status.lastPoseGeneration = output.poseGeneration;
        m_status.lastSceneVersionId = output.sceneVersionId;
        m_status.lastOutputWasStale = output.stale;
    }

    void SceneViewerRunner::setRunning(bool running) {
        m_running.store(running);
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_status.running = running;
        m_status.stopRequested = m_stopRequested.load();
    }

} // namespace edgevision::viewer
