#pragma once

#include <k4a/k4a.h>

#include "capture/interfaces/K4aInterface.hpp"
#include "frame/utils/DepthAlignmentResult.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::capture::frame {

    class K4aDepthAligner final {
      public:
        [[nodiscard]] static DepthAlignmentResult alignDepthToColor(
            const K4aInterface& api,
            const k4a_calibration_t& calibration,
            k4a_image_t colorImage,
            k4a_image_t depthImage,
            edgevision::model::frame::FrameId frameId
        );
    };

} // namespace edgevision::capture::frame
