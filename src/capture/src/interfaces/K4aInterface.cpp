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

            k4a_image_t captureGetColorImage(k4a_capture_t capture) const override {
                return k4a_capture_get_color_image(capture);
            }

            k4a_image_t captureGetDepthImage(k4a_capture_t capture) const override {
                return k4a_capture_get_depth_image(capture);
            }

            void captureRelease(k4a_capture_t capture) const override {
                k4a_capture_release(capture);
            }

            void imageRelease(k4a_image_t image) const override {
                k4a_image_release(image);
            }

            std::uint8_t* imageGetBuffer(k4a_image_t image) const override {
                return k4a_image_get_buffer(image);
            }

            std::size_t imageGetSize(k4a_image_t image) const override {
                return k4a_image_get_size(image);
            }

            int imageGetWidthPixels(k4a_image_t image) const override {
                return k4a_image_get_width_pixels(image);
            }

            int imageGetHeightPixels(k4a_image_t image) const override {
                return k4a_image_get_height_pixels(image);
            }

            int imageGetStrideBytes(k4a_image_t image) const override {
                return k4a_image_get_stride_bytes(image);
            }

            k4a_image_format_t imageGetFormat(k4a_image_t image) const override {
                return k4a_image_get_format(image);
            }

            k4a_result_t imageCreate(
                k4a_image_format_t format,
                int widthPixels,
                int heightPixels,
                int strideBytes,
                k4a_image_t* image
            ) const override {
                return k4a_image_create(format, widthPixels, heightPixels, strideBytes, image);
            }

            k4a_transformation_t transformationCreate(
                const k4a_calibration_t& calibration
            ) const override {
                return k4a_transformation_create(&calibration);
            }

            void transformationDestroy(k4a_transformation_t transformation) const override {
                k4a_transformation_destroy(transformation);
            }

            k4a_result_t transformationDepthImageToColorCamera(
                k4a_transformation_t transformation,
                k4a_image_t depthImage,
                k4a_image_t transformedDepthImage
            ) const override {
                return k4a_transformation_depth_image_to_color_camera(
                    transformation, depthImage, transformedDepthImage
                );
            }
        };
    } // namespace

    const K4aInterface& K4aInterface::instance() {
        static const K4aInterfaceImpl api;
        return api;
    }

} // namespace edgevision::capture
