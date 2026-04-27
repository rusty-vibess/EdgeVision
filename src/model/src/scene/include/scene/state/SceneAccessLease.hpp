#pragma once

#include <memory>

#include "model/scene/types/access.hpp"
#include "model/scene/types/infinitam.hpp"

namespace edgevision::model::scene {

    class SharedSceneState;

    enum class SceneLockMode {
        Read,
        Write,
    };

    class SceneAccessLease final {
      public:
        ~SceneAccessLease();

        /// Acquires shared scene access, blocking until it is granted.
        static std::shared_ptr<SceneAccessLease> acquireRead(
            std::shared_ptr<SharedSceneState> state
        );
        /// Acquires exclusive scene access, blocking until it is granted.
        static std::shared_ptr<SceneAccessLease> acquireWrite(
            std::shared_ptr<SharedSceneState> state
        );

        SceneAccessLease(const SceneAccessLease&) = delete;
        SceneAccessLease& operator=(const SceneAccessLease&) = delete;

        [[nodiscard]] InfiniTamScene& scene();
        [[nodiscard]] const InfiniTamScene& scene() const;
        [[nodiscard]] SceneLockMode mode() const;
        [[nodiscard]] SceneVersionId version() const;
        [[nodiscard]] bool owns(const SharedSceneState* state) const;
        /// Updates the version observed through this lease after publication.
        void setVersion(SceneVersionId version);

      private:
        SceneAccessLease(
            std::shared_ptr<SharedSceneState> state,
            SceneLockMode mode,
            SceneVersionId version
        );

        std::shared_ptr<SharedSceneState> m_state{};
        SceneLockMode m_mode = SceneLockMode::Read;
        SceneVersionId m_version = 0;
    };

} // namespace edgevision::model::scene
