#include "capture/interfaces/K4aInterface.hpp"

namespace edgevision::capture {
    namespace {

        /// Provides the default K4A interface implementation that forwards to the SDK.
        class K4aInterfaceImpl final : public K4aInterface {
          public:
            k4a_result_t deviceOpen(uint32_t deviceIndex, k4a_device_t* device) const override {
                return k4a_device_open(deviceIndex, device);
            }

            void deviceClose(k4a_device_t device) const override {
                k4a_device_close(device);
            }

            k4a_result_t deviceStartCameras(
                k4a_device_t device,
                const k4a_device_configuration_t* config
            ) const override {
                return k4a_device_start_cameras(device, config);
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

    const K4aInterface& K4aInterface::instance() {
        static const K4aInterfaceImpl api;
        return api;
    }

} // namespace edgevision::capture
