#pragma once

#include <memory>

#include "model/scene/SceneAccess.hpp"
#include "model/scene/types/access.hpp"

namespace edgevision::model::scene {

    class SharedSceneState;

    /// Owns the central voxel scene and provides blocking read/write access.
    class SharedScene final {
      public:
        explicit SharedScene(SceneReadPolicy readPolicy = SceneReadPolicy::Greedy);
        ~SharedScene();

        SharedScene(SharedScene&& other) noexcept;
        SharedScene& operator=(SharedScene&& other) noexcept;

        SharedScene(const SharedScene&) = delete;
        SharedScene& operator=(const SharedScene&) = delete;

        /// Returns shared access to the current scene. Blocks until no writer is active.
        [[nodiscard]] SceneReadAccess read() const;

        /// Returns exclusive access to the scene. Blocks until all access is released.
        [[nodiscard]] SceneWriteAccess write();

        /// Returns the last published scene version.
        [[nodiscard]] SceneVersionId version() const;
        /// Publishes `access` and advances the scene version.
        [[nodiscard]] SceneVersionId publish(SceneWriteAccess& access);

      private:
        std::shared_ptr<SharedSceneState> m_state{};
    };

} // namespace edgevision::model::scene
