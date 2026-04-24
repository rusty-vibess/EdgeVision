#pragma once

#include <memory>
#include <optional>

#include "model/scene/types/access.hpp"
#include "model/scene/types/infinitam.hpp"

namespace edgevision::model::scene {

    class SceneAccessInternals;
    class SceneAccessLease;
    class SharedSceneState;

    /// Shared read access to a published scene version.
    class SceneReadAccess final {
      public:
        ~SceneReadAccess();

        SceneReadAccess(SceneReadAccess&& other) noexcept;
        SceneReadAccess& operator=(SceneReadAccess&& other) noexcept;

        SceneReadAccess(const SceneReadAccess&) = delete;
        SceneReadAccess& operator=(const SceneReadAccess&) = delete;

        /// Returns the scene version visible through this access handle.
        [[nodiscard]] SceneVersionId version() const;
        /// Returns the voxel region covered by this access handle.
        [[nodiscard]] const VoxelRegion& region() const;
        /// Returns a read-only view of the current scene contents.
        [[nodiscard]] const InfiniTamScene& scene() const;

        /// Returns a child view constrained to `region`.
        [[nodiscard]] SceneReadAccess child(const VoxelRegion& region) const;

      private:
        friend class SceneAccessInternals;
        friend class SharedScene;

        SceneReadAccess(std::shared_ptr<SceneAccessLease> lease, VoxelRegion region);

        std::shared_ptr<SceneAccessLease> m_lease{};
        VoxelRegion m_region{};
    };

    /// Exclusive write access to a scene version in progress.
    class SceneWriteAccess final {
      public:
        ~SceneWriteAccess();

        SceneWriteAccess(SceneWriteAccess&& other) noexcept;
        SceneWriteAccess& operator=(SceneWriteAccess&& other) noexcept;

        SceneWriteAccess(const SceneWriteAccess&) = delete;
        SceneWriteAccess& operator=(const SceneWriteAccess&) = delete;

        /// Returns the scene version visible through this access handle.
        [[nodiscard]] SceneVersionId version() const;
        /// Returns the voxel region covered by this access handle.
        [[nodiscard]] const VoxelRegion& region() const;
        /// Returns mutable access to the scene contents.
        [[nodiscard]] InfiniTamScene& scene();
        /// Returns a read-only view of the scene contents.
        [[nodiscard]] const InfiniTamScene& scene() const;

        /// Returns a child view constrained to `region`.
        [[nodiscard]] SceneWriteAccess child(const VoxelRegion& region) const;

      private:
        friend class SceneAccessInternals;
        friend class SharedScene;

        SceneWriteAccess(std::shared_ptr<SceneAccessLease> lease, VoxelRegion region);

        std::shared_ptr<SceneAccessLease> m_lease{};
        VoxelRegion m_region{};
    };

    /// Owns the central voxel scene and arbitrates read/write access.
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
        /// Returns shared access if it is immediately available.
        [[nodiscard]] std::optional<SceneReadAccess> tryRead() const;

        /// Returns exclusive access to the scene. Blocks until all access is released.
        [[nodiscard]] SceneWriteAccess write();
        /// Returns exclusive access if it is immediately available.
        [[nodiscard]] std::optional<SceneWriteAccess> tryWrite();

        /// Returns the last published scene version.
        [[nodiscard]] SceneVersionId version() const;
        /// Publishes `access` and advances the scene version.
        [[nodiscard]] SceneVersionId publish(SceneWriteAccess& access);

      private:
        std::shared_ptr<SharedSceneState> m_state{};
    };

} // namespace edgevision::model::scene
