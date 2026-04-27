#include "model/viewer/ViewerPoseStore.hpp"

#include <mutex>

namespace edgevision::model::viewer {
    namespace {
        [[nodiscard]] bool hasNewerGeneration(
            const std::optional<ViewerPose>& latestPose,
            ViewerPoseGeneration generation
        ) {
            return latestPose.has_value() && latestPose->generation > generation;
        }
    } // namespace

    ViewerPoseStore::ViewerPoseStore() = default;

    ViewerPoseStore::~ViewerPoseStore() = default;

    ViewerPose ViewerPoseStore::update(ViewerPose viewerPose) {
        std::unique_lock lock(m_mutex);

        if (viewerPose.generation < m_nextGeneration) {
            viewerPose.generation = m_nextGeneration;
        }

        m_nextGeneration = viewerPose.generation + 1;
        m_latestPose = viewerPose;

        lock.unlock();
        m_condition.notify_all();
        return viewerPose;
    }

    std::optional<ViewerPose> ViewerPoseStore::latest() const {
        std::shared_lock lock(m_mutex);
        return m_latestPose;
    }

    std::optional<ViewerPose> ViewerPoseStore::waitForNewer(
        ViewerPoseGeneration generation,
        std::chrono::milliseconds timeout
    ) const {
        std::unique_lock lock(m_mutex);

        if (hasNewerGeneration(m_latestPose, generation)) {
            return m_latestPose;
        }

        if (timeout.count() <= 0) {
            return std::nullopt;
        }

        const bool updated = m_condition.wait_for(lock, timeout, [this, generation]() {
            return hasNewerGeneration(m_latestPose, generation);
        });
        if (!updated) {
            return std::nullopt;
        }

        return m_latestPose;
    }

} // namespace edgevision::model::viewer
