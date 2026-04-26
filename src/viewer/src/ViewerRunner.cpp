#include "viewer/ViewerRunner.hpp"

#include <algorithm>
#include <chrono>
#include <optional>
#include <thread>

#include "model/viewer/ViewerPoseStore.hpp"
#include "viewer/SceneViewer.hpp"

namespace edgevision::viewer {
    namespace {
        [[nodiscard]] std::chrono::milliseconds makeLoopPeriod(
            const edgevision::config::ViewerRuntimeConfig& config
        ) {
            return std::chrono::milliseconds(std::max(config.loopPeriodMs, 1));
        }
    } // namespace

    ViewerRunner::ViewerRunner(
        SceneViewer& viewer,
        edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
        const edgevision::config::ViewerRuntimeConfig& config
    )
        : m_viewer(viewer), m_viewerPoseStore(viewerPoseStore), m_config(config) {}

    ViewerRunner::~ViewerRunner() {
        requestStop();
        join();
    }

    bool ViewerRunner::start() {
        if (m_thread.joinable() || m_running.load()) {
            return false;
        }

        m_stopRequested.store(false);
        setRunning(true);
        m_thread = std::thread(&ViewerRunner::runLoop, this);
        return true;
    }

    void ViewerRunner::requestStop() {
        m_stopRequested.store(true);
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_status.stopRequested = true;
    }

    void ViewerRunner::join() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    bool ViewerRunner::running() const {
        return m_running.load();
    }

    ViewerRunnerStatus ViewerRunner::status() const {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        return m_status;
    }

    void ViewerRunner::runLoop() {
        const std::chrono::milliseconds loopPeriod = makeLoopPeriod(m_config);
        edgevision::model::viewer::ViewerPoseGeneration lastPoseGeneration = 0;

        while (!m_stopRequested.load()) {
            if (m_config.loopPolicy == edgevision::config::ViewerLoopPolicy::PoseDriven) {
                const std::optional<edgevision::model::viewer::ViewerPose> viewerPose =
                    m_viewerPoseStore.waitForNewer(lastPoseGeneration, loopPeriod);
                if (!viewerPose.has_value()) {
                    recordIdle(ViewerRunnerActivity::WaitingForPose);
                    continue;
                }

                recordRenderAttempt(*viewerPose);
                const std::optional<edgevision::model::viewer::RenderOutput> output =
                    m_viewer.renderPose(*viewerPose);
                if (!output.has_value()) {
                    recordIdle(ViewerRunnerActivity::WaitingForPose);
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
                recordIdle(ViewerRunnerActivity::Idle);
            } else {
                recordRenderAttempt(*viewerPose);
                const std::optional<edgevision::model::viewer::RenderOutput> output =
                    m_viewer.renderPose(*viewerPose);
                if (!output.has_value()) {
                    recordIdle(ViewerRunnerActivity::Idle);
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

    void ViewerRunner::recordIdle(ViewerRunnerActivity activity) {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        ++m_status.idleIterationCount;
        m_status.activity = activity;
    }

    void ViewerRunner::recordRenderAttempt(
        const edgevision::model::viewer::ViewerPose& viewerPose
    ) {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        ++m_status.renderAttemptCount;
        m_status.lastPoseGeneration = viewerPose.generation;
    }

    void ViewerRunner::recordOutput(const edgevision::model::viewer::RenderOutput& output) {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        ++m_status.publishedOutputCount;
        if (output.stale) {
            ++m_status.staleOutputCount;
        }

        m_status.activity = ViewerRunnerActivity::RenderedOutput;
        m_status.lastPoseGeneration = output.poseGeneration;
        m_status.lastSceneVersionId = output.sceneVersionId;
        m_status.lastOutputWasStale = output.stale;
    }

    void ViewerRunner::setRunning(bool running) {
        m_running.store(running);
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_status.running = running;
        m_status.stopRequested = m_stopRequested.load();
    }

} // namespace edgevision::viewer
