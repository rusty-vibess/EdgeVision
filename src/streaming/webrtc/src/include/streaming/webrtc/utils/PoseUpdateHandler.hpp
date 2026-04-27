#pragma once

#include <optional>
#include <string_view>

#include "model/viewer/types/pose.hpp"
#include "streaming/webrtc/types/PoseUpdate.hpp"

namespace edgevision::streaming::webrtc::utils {

    [[nodiscard]] std::optional<PoseUpdate> parsePoseUpdateMessage(std::string_view payload);

    [[nodiscard]] std::optional<edgevision::model::viewer::ViewerPose> makeUpdatedViewerPose(
        const std::optional<edgevision::model::viewer::ViewerPose>& currentPose,
        const PoseUpdate& poseUpdate
    );

} // namespace edgevision::streaming::webrtc::utils
