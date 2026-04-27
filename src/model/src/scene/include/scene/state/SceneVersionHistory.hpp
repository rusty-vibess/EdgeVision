#pragma once

#include <cstddef>
#include <deque>
#include <optional>
#include <unordered_map>

#include "model/scene/types/version.hpp"

namespace edgevision::model::scene {

    class SceneVersionHistory final {
      public:
        void add(SceneVersion sceneVersion);
        void remove(SceneVersionId versionId);

        [[nodiscard]] std::optional<SceneVersion> latest() const;
        [[nodiscard]] std::optional<SceneVersion> get(SceneVersionId versionId) const;
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] std::optional<SceneVersionId> oldestVersionId() const;

      private:
        std::optional<SceneVersion> m_latestVersion{};
        std::deque<SceneVersion> m_history{};
        std::unordered_map<SceneVersionId, SceneVersion> m_versionsById{};
    };

} // namespace edgevision::model::scene
