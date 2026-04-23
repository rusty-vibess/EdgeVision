#include "scene/state/SceneAccessLease.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

#include "model/scene/SharedScene.hpp"
#include "scene/state/SharedSceneState.hpp"

namespace edgevision::model::scene {
    namespace {
        [[nodiscard]] bool canAcquireRead(const SharedSceneState& state) {
            switch (state.readPolicy) {
                case SceneReadPolicy::Greedy:
                    return !state.writerActive;
                case SceneReadPolicy::Balanced:
                    return !state.writerActive
                        && (state.waitingWriterCount == 0 || state.blockedPriorityReadPending);
            }

            return false;
        }

        [[nodiscard]] bool canAcquireWrite(const SharedSceneState& state) {
            return !state.writerActive && state.activeReaderCount == 0
                && !state.blockedPriorityReadPending;
        }
    } // namespace

    SceneAccessLease::~SceneAccessLease() {
        if (!m_state) {
            return;
        }

        {
            std::lock_guard lock(m_state->mutex);
            if (m_mode == SceneLockMode::Read) {
                if (m_state->activeReaderCount > 0) {
                    --m_state->activeReaderCount;
                }
            } else {
                m_state->writerActive = false;
            }
        }

        m_state->condition.notify_all();
    }

    std::shared_ptr<SceneAccessLease> SceneAccessLease::acquireRead(
        std::shared_ptr<SharedSceneState> state
    ) {
        std::unique_lock lock(state->mutex);
        if (state->writerActive) {
            state->blockedPriorityReadPending = true;
        }

        state->condition.wait(lock, [&state]() { return canAcquireRead(*state); });
        ++state->activeReaderCount;
        const SceneVersionId version = state->sceneVersionId;
        state->blockedPriorityReadPending = false;
        // Unlock ASAP, before scope exit
        lock.unlock();

        return std::shared_ptr<SceneAccessLease>(
            new SceneAccessLease(std::move(state), SceneLockMode::Read, version)
        );
    }

    std::shared_ptr<SceneAccessLease> SceneAccessLease::tryAcquireRead(
        std::shared_ptr<SharedSceneState> state
    ) {
        std::unique_lock lock(state->mutex);
        if (!canAcquireRead(*state)) {
            return {};
        }

        ++state->activeReaderCount;
        const SceneVersionId version = state->sceneVersionId;
        state->blockedPriorityReadPending = false;
        // Unlock ASAP, before scope exit
        lock.unlock();

        return std::shared_ptr<SceneAccessLease>(
            new SceneAccessLease(std::move(state), SceneLockMode::Read, version)
        );
    }

    std::shared_ptr<SceneAccessLease> SceneAccessLease::acquireWrite(
        std::shared_ptr<SharedSceneState> state
    ) {
        std::unique_lock lock(state->mutex);
        ++state->waitingWriterCount;
        state->condition.wait(lock, [&state]() { return canAcquireWrite(*state); });
        --state->waitingWriterCount;
        state->writerActive = true;
        const SceneVersionId version = state->sceneVersionId;
        // Unlock ASAP, before scope exit
        lock.unlock();

        return std::shared_ptr<SceneAccessLease>(
            new SceneAccessLease(std::move(state), SceneLockMode::Write, version)
        );
    }

    std::shared_ptr<SceneAccessLease> SceneAccessLease::tryAcquireWrite(
        std::shared_ptr<SharedSceneState> state
    ) {
        // Try will not wait for scene access, but may briefly wait for the state mutex.
        std::unique_lock lock(state->mutex);
        if (!canAcquireWrite(*state)) {
            return {};
        }

        state->writerActive = true;
        const SceneVersionId version = state->sceneVersionId;
        // Unlock ASAP, before scope exit
        lock.unlock();

        return std::shared_ptr<SceneAccessLease>(
            new SceneAccessLease(std::move(state), SceneLockMode::Write, version)
        );
    }

    SharedSceneState& SceneAccessLease::state() {
        return *m_state;
    }

    const SharedSceneState& SceneAccessLease::state() const {
        return *m_state;
    }

    SceneLockMode SceneAccessLease::mode() const {
        return m_mode;
    }

    SceneVersionId SceneAccessLease::version() const {
        return m_version;
    }

    void SceneAccessLease::setVersion(SceneVersionId version) {
        m_version = version;
    }

    SceneAccessLease::SceneAccessLease(
        std::shared_ptr<SharedSceneState> state,
        SceneLockMode mode,
        SceneVersionId version
    )
        : m_state(std::move(state)), m_mode(mode), m_version(version) {}

    SceneAccessLease& SceneAccessInternals::lease(const SceneReadAccess& access) {
        if (!access.m_lease) {
            throw std::logic_error("Scene read access handle is empty");
        }

        return *access.m_lease;
    }

    SceneAccessLease& SceneAccessInternals::lease(const SceneWriteAccess& access) {
        if (!access.m_lease) {
            throw std::logic_error("Scene write access handle is empty");
        }

        return *access.m_lease;
    }

} // namespace edgevision::model::scene
