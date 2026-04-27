#pragma once

#include <chrono>
#include <condition_variable>
#include <optional>
#include <shared_mutex>

#include "model/viewer/types/pose.hpp"

namespace edgevision::model::viewer {

    /// Stores the latest requested viewer pose and lets consumers wait for newer generations.
    class ViewerPoseStore final {
      public:
        ViewerPoseStore();
        ~ViewerPoseStore();

        ViewerPoseStore(const ViewerPoseStore&) = delete;
        ViewerPoseStore& operator=(const ViewerPoseStore&) = delete;

        /// Stores `viewerPose` as the latest request and normalizes its generation if needed.
        [[nodiscard]] ViewerPose update(ViewerPose viewerPose);

        /// Returns the latest stored pose request, if any.
        [[nodiscard]] std::optional<ViewerPose> latest() const;
        /// Waits up to `timeout` for a pose whose generation is newer than `generation`.
        [[nodiscard]] std::optional<ViewerPose> waitForNewer(
            ViewerPoseGeneration generation,
            std::chrono::milliseconds timeout
        ) const;

      private:
        mutable std::shared_mutex m_mutex{};
        mutable std::condition_variable_any m_condition{};
        std::optional<ViewerPose> m_latestPose{};
        ViewerPoseGeneration m_nextGeneration = 1;
    };

} // namespace edgevision::model::viewer
