#include "interfaces/k4a.hpp"

#include <iostream>

#include "logger.hpp"

namespace {
    /// Provides the default k4a interface implementation that forwards to the SDK.
    class k4aInterfaceImpl final : public k4aInterface {
      public:
        k4a_result_t deviceOpen(uint32_t deviceIndex, k4a_device_t* device) const override {
            return k4a_device_open(deviceIndex, device);
        }

        void deviceClose(k4a_device_t device) const override {
            k4a_device_close(device);
        }

        bool deviceStartCameras(
            k4a_device_t device,
            const k4a_device_configuration_t* config
        ) const override {
            const k4a_result_t startResult = k4a_device_start_cameras(device, config);
            if (startResult != K4A_RESULT_SUCCEEDED) {
                LOG(LogLevel::error, "k4a", "Failed to start camera");
                return false;
            };

            const k4a_result_t imuResult = k4a_device_start_imu(device);
            if (imuResult != K4A_RESULT_SUCCEEDED) {
                LOG(LogLevel::error, "k4a", "Failed to start IMU");
                return false;
            };

            return true;
        }

        void deviceStopCameras(k4a_device_t device) const override {
            k4a_device_stop_cameras(device);
        }

        k4a_wait_result_t deviceGetCapture(
            k4a_device_t device,
            k4a_capture_t* capture,
            int32_t timeoutMs
        ) const override {
            return k4a_device_get_capture(device, capture, timeoutMs);
        }

        k4a_result_t deviceGetCalibration(
            k4a_device_t device,
            k4a_depth_mode_t depthMode,
            k4a_color_resolution_t colorResolution,
            k4a_calibration_t* calibration
        ) const override {
            return k4a_device_get_calibration(device, depthMode, colorResolution, calibration);
        }
    };
} // namespace

/// Returns the process-wide singleton used by non-test code.
const k4aInterface& k4aInterface::instance() {
    static const k4aInterfaceImpl k4aInterface;
    return k4aInterface;
}
