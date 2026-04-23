#pragma once

#include <memory>
#include <optional>

#include "model/scene/types/access.hpp"
#include "model/scene/types/infinitam.hpp"

namespace edgevision::model::scene {

    class SceneAccessInternals;
    class SceneAccessLease;
    class SharedSceneState;

    class SceneReadAccess final {
      public:
        ~SceneReadAccess();

        SceneReadAccess(SceneReadAccess&& other) noexcept;
        SceneReadAccess& operator=(SceneReadAccess&& other) noexcept;

        SceneReadAccess(const SceneReadAccess&) = delete;
        SceneReadAccess& operator=(const SceneReadAccess&) = delete;

        [[nodiscard]] SceneVersionId version() const;
        [[nodiscard]] const VoxelRegion& region() const;
        [[nodiscard]] const InfiniTamScene& scene() const;

        [[nodiscard]] SceneReadAccess child(const VoxelRegion& region) const;

      private:
        friend class SceneAccessInternals;
        friend class SharedScene;

        SceneReadAccess(std::shared_ptr<SceneAccessLease> lease, VoxelRegion region);

        std::shared_ptr<SceneAccessLease> m_lease{};
        VoxelRegion m_region{};
    };

    class SceneWriteAccess final {
      public:
        ~SceneWriteAccess();

        SceneWriteAccess(SceneWriteAccess&& other) noexcept;
        SceneWriteAccess& operator=(SceneWriteAccess&& other) noexcept;

        SceneWriteAccess(const SceneWriteAccess&) = delete;
        SceneWriteAccess& operator=(const SceneWriteAccess&) = delete;

        [[nodiscard]] SceneVersionId version() const;
        [[nodiscard]] const VoxelRegion& region() const;
        [[nodiscard]] InfiniTamScene& scene();
        [[nodiscard]] const InfiniTamScene& scene() const;

        [[nodiscard]] SceneWriteAccess child(const VoxelRegion& region) const;

      private:
        friend class SceneAccessInternals;
        friend class SharedScene;

        SceneWriteAccess(std::shared_ptr<SceneAccessLease> lease, VoxelRegion region);

        std::shared_ptr<SceneAccessLease> m_lease{};
        VoxelRegion m_region{};
    };

    class SharedScene final {
      public:
        SharedScene();
        ~SharedScene();

        SharedScene(SharedScene&& other) noexcept;
        SharedScene& operator=(SharedScene&& other) noexcept;

        SharedScene(const SharedScene&) = delete;
        SharedScene& operator=(const SharedScene&) = delete;

        [[nodiscard]] SceneReadAccess read() const;
        [[nodiscard]] std::optional<SceneReadAccess> tryRead() const;

        [[nodiscard]] SceneWriteAccess write();
        [[nodiscard]] std::optional<SceneWriteAccess> tryWrite();

        [[nodiscard]] SceneVersionId version() const;
        [[nodiscard]] SceneVersionId publish(SceneWriteAccess& access);

      private:
        std::shared_ptr<SharedSceneState> m_state{};
    };

} // namespace edgevision::model::scene
