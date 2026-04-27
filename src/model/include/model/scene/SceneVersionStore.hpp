#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <shared_mutex>

#include "model/scene/types/version.hpp"

namespace edgevision::model::scene {

    class SceneVersionHistory;

    class SceneVersionStore final {
      public:
        explicit SceneVersionStore(std::size_t capacity = 32);
        ~SceneVersionStore();

        SceneVersionStore(const SceneVersionStore&) = delete;
        SceneVersionStore& operator=(const SceneVersionStore&) = delete;

        void add(SceneVersion sceneVersion);

        [[nodiscard]] std::optional<SceneVersion> latest() const;
        [[nodiscard]] std::optional<SceneVersion> get(SceneVersionId versionId) const;

      private:
        std::size_t m_capacity = 32;
        mutable std::shared_mutex m_mutex{};
        std::unique_ptr<SceneVersionHistory> m_history{};
    };

} // namespace edgevision::model::scene
