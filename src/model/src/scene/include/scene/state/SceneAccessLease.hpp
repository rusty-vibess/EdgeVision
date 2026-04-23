#pragma once

#include <memory>
#include <mutex>
#include <shared_mutex>

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
        static std::shared_ptr<SceneAccessLease> acquireRead(
            std::shared_ptr<SharedSceneState> state
        );
        static std::shared_ptr<SceneAccessLease> tryAcquireRead(
            std::shared_ptr<SharedSceneState> state
        );
        static std::shared_ptr<SceneAccessLease> acquireWrite(
            std::shared_ptr<SharedSceneState> state
        );
        static std::shared_ptr<SceneAccessLease> tryAcquireWrite(
            std::shared_ptr<SharedSceneState> state
        );

        SceneAccessLease(const SceneAccessLease&) = delete;
        SceneAccessLease& operator=(const SceneAccessLease&) = delete;

        [[nodiscard]] SharedSceneState& state();
        [[nodiscard]] const SharedSceneState& state() const;
        [[nodiscard]] SceneLockMode mode() const;
        [[nodiscard]] SceneVersionId version() const;
        void setVersion(SceneVersionId version);

      private:
        SceneAccessLease(
            std::shared_ptr<SharedSceneState> state,
            std::shared_lock<std::shared_mutex> lock,
            SceneVersionId version
        );
        SceneAccessLease(
            std::shared_ptr<SharedSceneState> state,
            std::unique_lock<std::shared_mutex> lock,
            SceneVersionId version
        );

        std::shared_ptr<SharedSceneState> m_state{};
        SceneLockMode m_mode = SceneLockMode::Read;
        SceneVersionId m_version = 0;
        std::shared_lock<std::shared_mutex> m_readLock{};
        std::unique_lock<std::shared_mutex> m_writeLock{};
    };

    class SceneAccessInternals final {
      public:
        [[nodiscard]] static SceneAccessLease& lease(const SceneReadAccess& access);
        [[nodiscard]] static SceneAccessLease& lease(const SceneWriteAccess& access);
    };

} // namespace edgevision::model::scene
