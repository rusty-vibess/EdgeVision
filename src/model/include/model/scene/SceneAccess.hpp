#pragma once

#include <memory>

#include "model/scene/types/access.hpp"
#include "model/scene/types/infinitam.hpp"

namespace edgevision::model::scene {

    class SceneAccessLease;
    class SharedScene;

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
        /// Returns a read-only view of the current scene contents.
        [[nodiscard]] const InfiniTamScene& scene() const;

      private:
        friend class SharedScene;

        explicit SceneReadAccess(std::shared_ptr<SceneAccessLease> lease);

        std::shared_ptr<SceneAccessLease> m_lease{};
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
        /// Returns mutable access to the scene contents.
        [[nodiscard]] InfiniTamScene& scene();

      private:
        friend class SharedScene;

        explicit SceneWriteAccess(std::shared_ptr<SceneAccessLease> lease);

        std::shared_ptr<SceneAccessLease> m_lease{};
    };

} // namespace edgevision::model::scene
