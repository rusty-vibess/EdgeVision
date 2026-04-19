#pragma once

#include <k4a/k4a.h>
#include <optional>

#include "capture/frame/types/build_result.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::capture::frame {

    class K4aDepthAlignmentValidator final {
      public:
        [[nodiscard]] static std::optional<FrameBuildResult> validateTransformationCreate(
            k4a_transformation_t transformation,
            edgevision::model::frame::FrameId frameId
        );

        [[nodiscard]] static std::optional<FrameBuildResult> validateTransformedDepthImageCreate(
            k4a_result_t result,
            k4a_image_t image,
            edgevision::model::frame::FrameId frameId
        );

        [[nodiscard]] static std::optional<FrameBuildResult> validateDepthAlignment(
            k4a_result_t result,
            edgevision::model::frame::FrameId frameId
        );
    };

} // namespace edgevision::capture::frame
