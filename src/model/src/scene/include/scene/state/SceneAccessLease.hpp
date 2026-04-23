#pragma once

#include <memory>

#include "model/scene/types/access.hpp"

namespace edgevision::model::scene {

    class SceneReadAccess;
    class SceneWriteAccess;
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
        /// Acquires shared scene access if it is immediately available.
        static std::shared_ptr<SceneAccessLease> tryAcquireRead(
            std::shared_ptr<SharedSceneState> state
        );
        /// Acquires exclusive scene access, blocking until it is granted.
        static std::shared_ptr<SceneAccessLease> acquireWrite(
            std::shared_ptr<SharedSceneState> state
        );
        /// Acquires exclusive scene access if it is immediately available.
        static std::shared_ptr<SceneAccessLease> tryAcquireWrite(
            std::shared_ptr<SharedSceneState> state
        );

        SceneAccessLease(const SceneAccessLease&) = delete;
        SceneAccessLease& operator=(const SceneAccessLease&) = delete;

        [[nodiscard]] SharedSceneState& state();
        [[nodiscard]] const SharedSceneState& state() const;
        [[nodiscard]] SceneLockMode mode() const;
        [[nodiscard]] SceneVersionId version() const;
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

    class SceneAccessInternals final {
      public:
        [[nodiscard]] static SceneAccessLease& lease(const SceneReadAccess& access);
        [[nodiscard]] static SceneAccessLease& lease(const SceneWriteAccess& access);
    };

} // namespace edgevision::model::scene
