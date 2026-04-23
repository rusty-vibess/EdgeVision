#include "model/scene/SharedScene.hpp"

#include <memory>
#include <mutex>
#include <stdexcept>
#include <utility>

#include "scene/state/SceneAccessLease.hpp"
#include "scene/state/SharedSceneState.hpp"

namespace edgevision::model::scene {

    SceneReadAccess::~SceneReadAccess() = default;
    SceneReadAccess::SceneReadAccess(SceneReadAccess&& other) noexcept = default;
    SceneReadAccess& SceneReadAccess::operator=(SceneReadAccess&& other) noexcept = default;

    SceneVersionId SceneReadAccess::version() const {
        return SceneAccessInternals::lease(*this).version();
    }

    const VoxelRegion& SceneReadAccess::region() const {
        return m_region;
    }

    const InfiniTamScene& SceneReadAccess::scene() const {
        return *SceneAccessInternals::lease(*this).state().scene;
    }

    SceneReadAccess SceneReadAccess::child(const VoxelRegion& region) const {
        if (!m_lease) {
            throw std::logic_error("Cannot derive child access from an empty read handle");
        }

        return SceneReadAccess(m_lease, region);
    }

    SceneReadAccess::SceneReadAccess(std::shared_ptr<SceneAccessLease> lease, VoxelRegion region)
        : m_lease(std::move(lease)), m_region(region) {}

    SceneWriteAccess::~SceneWriteAccess() = default;
    SceneWriteAccess::SceneWriteAccess(SceneWriteAccess&& other) noexcept = default;
    SceneWriteAccess& SceneWriteAccess::operator=(SceneWriteAccess&& other) noexcept = default;

    SceneVersionId SceneWriteAccess::version() const {
        return SceneAccessInternals::lease(*this).version();
    }

    const VoxelRegion& SceneWriteAccess::region() const {
        return m_region;
    }

    InfiniTamScene& SceneWriteAccess::scene() {
        return *SceneAccessInternals::lease(*this).state().scene;
    }

    const InfiniTamScene& SceneWriteAccess::scene() const {
        return *SceneAccessInternals::lease(*this).state().scene;
    }

    SceneWriteAccess SceneWriteAccess::child(const VoxelRegion& region) const {
        if (!m_lease) {
            throw std::logic_error("Cannot derive child access from an empty write handle");
        }

        return SceneWriteAccess(m_lease, region);
    }

    SceneWriteAccess::SceneWriteAccess(std::shared_ptr<SceneAccessLease> lease, VoxelRegion region)
        : m_lease(std::move(lease)), m_region(region) {}

    SharedScene::SharedScene(SceneReadPolicy readPolicy)
        : m_state(std::make_shared<SharedSceneState>(readPolicy)) {}

    SharedScene::~SharedScene() = default;
    SharedScene::SharedScene(SharedScene&& other) noexcept = default;
    SharedScene& SharedScene::operator=(SharedScene&& other) noexcept = default;

    SceneReadAccess SharedScene::read() const {
        const std::shared_ptr<SceneAccessLease> lease = SceneAccessLease::acquireRead(m_state);
        return SceneReadAccess(lease, wholeSceneRegion());
    }

    std::optional<SceneReadAccess> SharedScene::tryRead() const {
        const std::shared_ptr<SceneAccessLease> lease = SceneAccessLease::tryAcquireRead(m_state);
        if (!lease) {
            return std::nullopt;
        }

        return SceneReadAccess(lease, wholeSceneRegion());
    }

    SceneWriteAccess SharedScene::write() {
        const std::shared_ptr<SceneAccessLease> lease = SceneAccessLease::acquireWrite(m_state);
        return SceneWriteAccess(lease, wholeSceneRegion());
    }

    std::optional<SceneWriteAccess> SharedScene::tryWrite() {
        const std::shared_ptr<SceneAccessLease> lease = SceneAccessLease::tryAcquireWrite(m_state);
        if (!lease) {
            return std::nullopt;
        }

        return SceneWriteAccess(lease, wholeSceneRegion());
    }

    SceneVersionId SharedScene::version() const {
        std::lock_guard lock(m_state->mutex);
        return m_state->sceneVersionId;
    }

    SceneVersionId SharedScene::publish(SceneWriteAccess& access) {
        SceneAccessLease& lease = SceneAccessInternals::lease(access);
        if (lease.mode() != SceneLockMode::Write || &lease.state() != m_state.get()) {
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
