#pragma once

#include <k4a/k4a.h>

#include "capture/interfaces/K4aInterface.hpp"
#include "model/frame/types/camera.hpp"
#include "model/frame/types/frame.hpp"
#include "model/frame/types/identity.hpp"
#include "model/frame/types/image.hpp"

namespace edgevision::capture::frame {

    class K4aFrameAssembler final {
      public:
        [[nodiscard]] static edgevision::model::frame::CameraIntrinsics makeIntrinsics(
            const k4a_calibration_t& calibration
        );

        [[nodiscard]] static edgevision::model::frame::Frame assemble(
            const K4aInterface& api,
            k4a_image_t colorImage,
            k4a_image_t depthImage,
            edgevision::model::frame::CameraIntrinsics intrinsics,
            const k4a_device_configuration_t& config,
            edgevision::model::frame::FrameId frameId,
            edgevision::model::frame::FrameTimestamp timestamp
        );

      private:
        [[nodiscard]] static edgevision::model::frame::FrameImage makeFrameImage(
            const K4aInterface& api,
            k4a_image_t image
        );

        [[nodiscard]] static edgevision::model::frame::FrameBuffer copyImageBuffer(
            const K4aInterface& api,
            k4a_image_t image
        );

        [[nodiscard]] static edgevision::model::frame::CameraConfig makeCameraConfig(
            const edgevision::model::frame::Frame& frame,
            const k4a_device_configuration_t& config
        );
    };

} // namespace edgevision::capture::frame
