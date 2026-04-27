#include "model/scene/SceneVersionStore.hpp"

#include <algorithm>
#include <memory>
#include <mutex>

#include "scene/state/SceneVersionHistory.hpp"

namespace edgevision::model::scene {

    SceneVersionStore::SceneVersionStore(std::size_t capacity)
        : m_capacity(std::max<std::size_t>(1, capacity)),
          m_history(std::make_unique<SceneVersionHistory>()) {}

    SceneVersionStore::~SceneVersionStore() = default;

    void SceneVersionStore::add(SceneVersion sceneVersion) {
        std::unique_lock lock(m_mutex);
        m_history->add(sceneVersion);

        while (m_history->size() > m_capacity) {
            const std::optional<SceneVersionId> oldestVersionId = m_history->oldestVersionId();
            if (!oldestVersionId.has_value()) {
                break;
            }

            m_history->remove(*oldestVersionId);
        }
    }

    std::optional<SceneVersion> SceneVersionStore::latest() const {
        std::shared_lock lock(m_mutex);
        return m_history->latest();
    }

    std::optional<SceneVersion> SceneVersionStore::get(SceneVersionId versionId) const {
        std::shared_lock lock(m_mutex);
        return m_history->get(versionId);
    }

} // namespace edgevision::model::scene
