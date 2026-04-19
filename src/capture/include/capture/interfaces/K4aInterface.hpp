#pragma once

#include <cstdint>
#include <k4a/k4a.h>

namespace edgevision::capture {

    class K4aInterface {
      public:
        virtual ~K4aInterface() = default;

        virtual k4a_result_t deviceOpen(uint32_t deviceIndex, k4a_device_t* device) const = 0;
        virtual void deviceClose(k4a_device_t device) const = 0;
        virtual k4a_result_t deviceStartCameras(
            k4a_device_t device,
            const k4a_device_configuration_t* config
        ) const = 0;
        virtual void deviceStopCameras(k4a_device_t device) const = 0;
        virtual k4a_wait_result_t deviceGetCapture(
            k4a_device_t device,
            k4a_capture_t* capture,
            int32_t timeoutMs
        ) const = 0;
        virtual k4a_result_t deviceGetCalibration(
            k4a_device_t device,
            k4a_depth_mode_t depthMode,
            k4a_color_resolution_t colorResolution,
            k4a_calibration_t* calibration
        ) const = 0;

        static const K4aInterface& instance();
    };

} // namespace edgevision::capture
