#include "scene/state/SceneAccessLease.hpp"

#include <memory>
#include <mutex>
#include <shared_mutex>
#include <stdexcept>
#include <utility>

#include "model/scene/SharedScene.hpp"
#include "scene/state/SharedSceneState.hpp"

namespace edgevision::model::scene {

    std::shared_ptr<SceneAccessLease> SceneAccessLease::acquireRead(
        std::shared_ptr<SharedSceneState> state
    ) {
        std::shared_lock lock(state->mutex);
        const SceneVersionId version = state->sceneVersionId;
        return std::shared_ptr<SceneAccessLease>(
            new SceneAccessLease(std::move(state), std::move(lock), version)
        );
    }

    std::shared_ptr<SceneAccessLease> SceneAccessLease::tryAcquireRead(
        std::shared_ptr<SharedSceneState> state
    ) {
        std::shared_lock lock(state->mutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            return {};
        }

        const SceneVersionId version = state->sceneVersionId;
        return std::shared_ptr<SceneAccessLease>(
            new SceneAccessLease(std::move(state), std::move(lock), version)
        );
    }

    std::shared_ptr<SceneAccessLease> SceneAccessLease::acquireWrite(
        std::shared_ptr<SharedSceneState> state
    ) {
        std::unique_lock lock(state->mutex);
        const SceneVersionId version = state->sceneVersionId;
        return std::shared_ptr<SceneAccessLease>(
            new SceneAccessLease(std::move(state), std::move(lock), version)
        );
    }

    std::shared_ptr<SceneAccessLease> SceneAccessLease::tryAcquireWrite(
        std::shared_ptr<SharedSceneState> state
    ) {
        std::unique_lock lock(state->mutex, std::try_to_lock);
        if (!lock.owns_lock()) {
            return {};
        }

        const SceneVersionId version = state->sceneVersionId;
        return std::shared_ptr<SceneAccessLease>(
            new SceneAccessLease(std::move(state), std::move(lock), version)
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
        std::shared_lock<std::shared_mutex> lock,
        SceneVersionId version
    )
        : m_state(std::move(state)),
          m_mode(SceneLockMode::Read),
          m_version(version),
          m_readLock(std::move(lock)) {}

    SceneAccessLease::SceneAccessLease(
        std::shared_ptr<SharedSceneState> state,
        std::unique_lock<std::shared_mutex> lock,
        SceneVersionId version
    )
        : m_state(std::move(state)),
          m_mode(SceneLockMode::Write),
          m_version(version),
          m_writeLock(std::move(lock)) {}

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
