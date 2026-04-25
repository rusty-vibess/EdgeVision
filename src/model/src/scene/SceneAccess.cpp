#include "model/scene/SceneAccess.hpp"

#include <stdexcept>
#include <utility>

#include "scene/state/SceneAccessLease.hpp"

namespace edgevision::model::scene {
    namespace {
        [[nodiscard]] SceneAccessLease& requireLease(
            const std::shared_ptr<SceneAccessLease>& lease,
            const char* message
        ) {
            if (!lease) {
                throw std::logic_error(message);
            }

            return *lease;
        }
    } // namespace

    SceneReadAccess::~SceneReadAccess() = default;
    SceneReadAccess::SceneReadAccess(SceneReadAccess&& other) noexcept = default;
    SceneReadAccess& SceneReadAccess::operator=(SceneReadAccess&& other) noexcept = default;

    SceneVersionId SceneReadAccess::version() const {
        return requireLease(m_lease, "Scene read access handle is empty").version();
    }

    const InfiniTamScene& SceneReadAccess::scene() const {
        return requireLease(m_lease, "Scene read access handle is empty").scene();
    }

    SceneReadAccess::SceneReadAccess(std::shared_ptr<SceneAccessLease> lease)
        : m_lease(std::move(lease)) {}

    // -----------------------------------------------------------------------------------------

    SceneWriteAccess::~SceneWriteAccess() = default;
    SceneWriteAccess::SceneWriteAccess(SceneWriteAccess&& other) noexcept = default;
    SceneWriteAccess& SceneWriteAccess::operator=(SceneWriteAccess&& other) noexcept = default;

    SceneVersionId SceneWriteAccess::version() const {
        return requireLease(m_lease, "Scene write access handle is empty").version();
    }

    InfiniTamScene& SceneWriteAccess::scene() {
        return requireLease(m_lease, "Scene write access handle is empty").scene();
    }

    SceneWriteAccess::SceneWriteAccess(std::shared_ptr<SceneAccessLease> lease)
        : m_lease(std::move(lease)) {}

} // namespace edgevision::model::scene
