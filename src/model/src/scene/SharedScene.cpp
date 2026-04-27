#include "model/scene/SharedScene.hpp"

#include <mutex>
#include <stdexcept>

#include "scene/state/SceneAccessLease.hpp"
#include "scene/state/SharedSceneState.hpp"

namespace edgevision::model::scene {
    SharedScene::SharedScene(const edgevision::config::SceneConfig& config)
        : m_state(std::make_shared<SharedSceneState>(config.readPolicy)) {}

    SharedScene::SharedScene(edgevision::config::SceneReadPolicy readPolicy)
        : SharedScene(edgevision::config::SceneConfig{readPolicy}) {}

    SharedScene::~SharedScene() = default;
    SharedScene::SharedScene(SharedScene&& other) noexcept = default;
    SharedScene& SharedScene::operator=(SharedScene&& other) noexcept = default;

    SceneReadAccess SharedScene::read() const {
        const std::shared_ptr<SceneAccessLease> lease = SceneAccessLease::acquireRead(m_state);
        return SceneReadAccess(lease);
    }

    SceneWriteAccess SharedScene::write() {
        const std::shared_ptr<SceneAccessLease> lease = SceneAccessLease::acquireWrite(m_state);
        return SceneWriteAccess(lease);
    }

    SceneVersionId SharedScene::version() const {
        std::lock_guard lock(m_state->mutex);
        return m_state->sceneVersionId;
    }

    SceneVersionId SharedScene::publish(SceneWriteAccess& access) {
        if (!access.m_lease) {
            throw std::invalid_argument("Scene version publication requires a valid write access");
        }

        SceneAccessLease& lease = *access.m_lease;
        if (lease.mode() != SceneLockMode::Write || !lease.owns(m_state.get())) {
            throw std::invalid_argument(
                "Scene version publication requires this scene write access"
            );
        }

        std::lock_guard lock(m_state->mutex);
        ++m_state->sceneVersionId;
        lease.setVersion(m_state->sceneVersionId);
        return m_state->sceneVersionId;
    }

} // namespace edgevision::model::scene
