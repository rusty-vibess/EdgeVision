#pragma once

#include <memory>

#include "config/types/scene.hpp"
#include "model/scene/SceneAccess.hpp"
#include "model/scene/types/access.hpp"

namespace edgevision::model::scene {

    class SharedSceneState;

    namespace testing {
        class SharedSceneTestAccess;
    } // namespace testing

    /// Owns the central voxel scene and provides blocking read/write access.
    class SharedScene final {
      public:
        explicit SharedScene(
            const edgevision::config::SceneConfig& config = edgevision::config::SceneConfig{}
        );
        explicit SharedScene(edgevision::config::SceneReadPolicy readPolicy);
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
        friend class testing::SharedSceneTestAccess;

        std::shared_ptr<SharedSceneState> m_state{};
    };

} // namespace edgevision::model::scene
