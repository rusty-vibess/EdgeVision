#pragma once

#include <optional>
#include <string>

#include "model/frame/types/frame.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::capture::frame {

    enum class FrameBuildCode {
        Built,
        InvalidFrameId,
        InvalidTimestamp,
        MissingColorImage,
        MissingDepthImage,
        UnsupportedColorFormat,
        UnsupportedDepthFormat,
        InvalidColorImage,
        InvalidDepthImage,
        TransformationCreateFailed,
        TransformedDepthImageCreateFailed,
        DepthAlignmentFailed,
        InvalidIntrinsics,
        InvalidCameraConfig,
    };

    struct FrameBuildResult {
        FrameBuildCode code = FrameBuildCode::InvalidFrameId;
        edgevision::model::frame::FrameId frameId = 0;
        std::string message{};
        std::optional<edgevision::model::frame::Frame> frame{};

        [[nodiscard]] bool built() const {
            return code == FrameBuildCode::Built && frame.has_value();
        }
    };

} // namespace edgevision::capture::frame
