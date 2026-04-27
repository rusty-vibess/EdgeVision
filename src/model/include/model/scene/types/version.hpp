#pragma once

#include <array>

#include "model/frame/types/identity.hpp"
#include "model/scene/types/access.hpp"

namespace edgevision::model::scene {

    struct Pose4f {
        std::array<float, 16> matrix{};
    };

    [[nodiscard]] inline bool operator==(const Pose4f& lhs, const Pose4f& rhs) {
        return lhs.matrix == rhs.matrix;
    }

    [[nodiscard]] inline bool operator!=(const Pose4f& lhs, const Pose4f& rhs) {
        return !(lhs == rhs);
    }

    enum class TrackingStatus {
        NotRun,
        Tracked,
        Lost,
    };

    enum class IntegrationStatus {
        Skipped,
        Integrated,
        Failed,
    };

    struct SceneVersion {
        SceneVersionId versionId = 0;
        edgevision::model::frame::FrameTimestamp timestamp{};
        edgevision::model::frame::FrameId lastIntegratedFrameId = 0;
        Pose4f cameraToWorld{};
        TrackingStatus trackingStatus = TrackingStatus::NotRun;
        IntegrationStatus integrationStatus = IntegrationStatus::Skipped;
    };

    [[nodiscard]] inline bool operator==(const SceneVersion& lhs, const SceneVersion& rhs) {
        return lhs.versionId == rhs.versionId && lhs.timestamp == rhs.timestamp
            && lhs.lastIntegratedFrameId == rhs.lastIntegratedFrameId
            && lhs.cameraToWorld == rhs.cameraToWorld && lhs.trackingStatus == rhs.trackingStatus
            && lhs.integrationStatus == rhs.integrationStatus;
    }

    [[nodiscard]] inline bool operator!=(const SceneVersion& lhs, const SceneVersion& rhs) {
        return !(lhs == rhs);
    }

} // namespace edgevision::model::scene
