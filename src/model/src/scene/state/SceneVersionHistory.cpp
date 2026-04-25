#include "scene/state/SceneVersionHistory.hpp"

#include <algorithm>

namespace edgevision::model::scene {

    void SceneVersionHistory::add(SceneVersion sceneVersion) {
        m_history.push_back(sceneVersion);
        m_versionsById[sceneVersion.versionId] = sceneVersion;
        m_latestVersion = sceneVersion;
    }

    void SceneVersionHistory::remove(SceneVersionId versionId) {
        const auto version = std::find_if(
            m_history.begin(), m_history.end(), [versionId](const SceneVersion& item) {
                return item.versionId == versionId;
            }
        );

        if (version != m_history.end()) {
            m_history.erase(version);
        }

        m_versionsById.erase(versionId);
    }

    std::optional<SceneVersion> SceneVersionHistory::latest() const {
        return m_latestVersion;
    }

    std::optional<SceneVersion> SceneVersionHistory::get(SceneVersionId versionId) const {
        const auto version = m_versionsById.find(versionId);
        if (version == m_versionsById.end()) {
            return std::nullopt;
        }

        return version->second;
    }

    std::size_t SceneVersionHistory::size() const {
        return m_history.size();
    }

    std::optional<SceneVersionId> SceneVersionHistory::oldestVersionId() const {
        if (m_history.empty()) {
            return std::nullopt;
        }

        return m_history.front().versionId;
    }

} // namespace edgevision::model::scene
