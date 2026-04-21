#pragma once

#include <k4a/k4a.h>
#include <optional>

#include "capture/frame/types/build_result.hpp"
#include "capture/interfaces/K4aInterface.hpp"
#include "model/frame/types/camera.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::capture::frame {

    class K4aFrameValidator final {
      public:
        [[nodiscard]] static std::optional<FrameBuildResult> validateRequest(
            k4a_capture_t capture,
            edgevision::model::frame::FrameId frameId,
            edgevision::model::frame::FrameTimestamp timestamp
        );

        [[nodiscard]] static std::optional<FrameBuildResult> validateColorImage(
            const K4aInterface& api,
            k4a_image_t image,
            edgevision::model::frame::FrameId frameId
        );

        [[nodiscard]] static std::optional<FrameBuildResult> validateDepthImage(
            const K4aInterface& api,
            k4a_image_t image,
            edgevision::model::frame::FrameId frameId
        );

        [[nodiscard]] static std::optional<FrameBuildResult> validateIntrinsics(
            const edgevision::model::frame::CameraIntrinsics& intrinsics,
            edgevision::model::frame::FrameId frameId
        );
    };

} // namespace edgevision::capture::frame
