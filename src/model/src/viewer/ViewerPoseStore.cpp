#include "model/viewer/ViewerPoseStore.hpp"

#include <cstddef>
#include <mutex>

namespace edgevision::model::viewer {
    namespace {
        [[nodiscard]] bool hasNewerGeneration(
            const std::optional<ViewerPose>& latestPose,
            ViewerPoseGeneration generation
        ) {
            return latestPose.has_value() && latestPose->generation > generation;
        }

        [[nodiscard]] edgevision::model::scene::Pose4f multiplyRowMajor(
            const edgevision::model::scene::Pose4f& lhs,
            const edgevision::model::scene::Pose4f& rhs
        ) {
            // Pose4f uses row-major camera-to-world matrices. Relative client
            // poses compose against the captured baseline as `relative * baseline`.
            edgevision::model::scene::Pose4f result{};
            for (int row = 0; row < 4; ++row) {
                for (int column = 0; column < 4; ++column) {
                    float value = 0.0f;
                    for (int index = 0; index < 4; ++index) {
                        value += lhs.matrix[static_cast<std::size_t>(row * 4 + index)]
                            * rhs.matrix[static_cast<std::size_t>(index * 4 + column)];
                    }
                    result.matrix[static_cast<std::size_t>(row * 4 + column)] = value;
                }
            }
            return result;
        }

        [[nodiscard]] bool samePoseRequest(
            const ViewerPose& lhs,
            const ViewerPose& rhs
        ) {
            return lhs.pose == rhs.pose && lhs.intrinsics.fx == rhs.intrinsics.fx
                && lhs.intrinsics.fy == rhs.intrinsics.fy && lhs.intrinsics.cx == rhs.intrinsics.cx
                && lhs.intrinsics.cy == rhs.intrinsics.cy && lhs.imageSize == rhs.imageSize;
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

    bool ViewerPoseStore::resetRelativeBaseline() {
        std::unique_lock lock(m_mutex);
        if (!m_latestPose.has_value()) {
            m_relativeBaselinePose.reset();
            return false;
        }

        m_relativeBaselinePose = m_latestPose;
        return true;
    }

    std::optional<ViewerPose> ViewerPoseStore::applyRelativePose(
        const edgevision::model::scene::Pose4f& relativePose
    ) {
        std::unique_lock lock(m_mutex);
        if (!m_relativeBaselinePose.has_value()) {
            return std::nullopt;
        }

        ViewerPose nextPose = *m_relativeBaselinePose;
        nextPose.pose = multiplyRowMajor(relativePose, m_relativeBaselinePose->pose);

        if (m_latestPose.has_value() && samePoseRequest(*m_latestPose, nextPose)) {
            return std::nullopt;
        }

        if (nextPose.generation < m_nextGeneration) {
            nextPose.generation = m_nextGeneration;
        }

        m_nextGeneration = nextPose.generation + 1;
        m_latestPose = nextPose;

        lock.unlock();
        m_condition.notify_all();
        return nextPose;
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
