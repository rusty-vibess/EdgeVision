#include "builder/SceneBuilderRunner.hpp"

#include <algorithm>
#include <chrono>
#include <memory>
#include <optional>
#include <utility>

#include "builder/state/SceneBuilder.hpp"
#include "model/frame/FrameStore.hpp"

namespace edgevision::builder {

    SceneBuilderRunner::SceneBuilderRunner(
        edgevision::model::frame::FrameStore& frameStore,
        edgevision::model::scene::SharedScene& sharedScene,
        edgevision::model::scene::SceneVersionStore& sceneVersionStore,
        const edgevision::config::BuilderRuntimeConfig& config
    )
        : m_frameStore(frameStore),
          m_builder(std::make_unique<SceneBuilder>(sharedScene, sceneVersionStore, config)),
          m_config(config) {}

    SceneBuilderRunner::~SceneBuilderRunner() {
        requestStop();
        join();
    }

    bool SceneBuilderRunner::start() {
        if (m_thread.joinable() || m_running.load()) {
            return false;
        }

        m_stopRequested.store(false);
        setRunning(true);
        m_thread = std::thread(&SceneBuilderRunner::runLoop, this);
        return true;
    }

    void SceneBuilderRunner::requestStop() {
        m_stopRequested.store(true);
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_status.stopRequested = true;
    }

    void SceneBuilderRunner::join() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    bool SceneBuilderRunner::running() const {
        return m_running.load();
    }

    SceneBuilderRunnerStatus SceneBuilderRunner::status() const {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        return m_status;
    }

    void SceneBuilderRunner::runLoop() {
        const std::chrono::milliseconds readyFrameTimeout(
            std::max(m_config.readyFrameTimeoutMs, 0)
        );

        while (!m_stopRequested.load()) {
            std::optional<edgevision::model::frame::Frame> frame =
                m_frameStore.waitDequeueReadyFrame(readyFrameTimeout);
            if (!frame.has_value()) {
                continue;
            }

            const FrameBuildResult result = m_builder->buildFrame(std::move(*frame));
            if (result.status == FrameBuildStatus::FrameConsumed) {
                [[maybe_unused]] const bool acknowledged =
                    m_frameStore.markFrameConsumed(result.frameId);
            } else {
                [[maybe_unused]] const bool acknowledged =
                    m_frameStore.markFrameFailed(result.frameId);
            }

            recordResult(result);
        }

        setRunning(false);
    }

    void SceneBuilderRunner::recordResult(const FrameBuildResult& result) {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        ++m_status.buildAttemptCount;
        m_status.lastFrameId = result.frameId;
        m_status.lastSceneVersionId = result.sceneVersionId;

        if (result.status == FrameBuildStatus::FrameConsumed) {
            ++m_status.consumedFrameCount;
            m_status.lastFailureReason.reset();
            return;
        }

        ++m_status.failedFrameCount;
        m_status.lastSceneVersionId.reset();
        m_status.lastFailureReason = result.failureReason;
    }

    void SceneBuilderRunner::setRunning(bool running) {
        m_running.store(running);
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_status.running = running;
        m_status.stopRequested = m_stopRequested.load();
    }

} // namespace edgevision::builder
