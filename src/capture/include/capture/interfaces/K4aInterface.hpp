#pragma once

#include <cstddef>
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
        virtual k4a_image_t captureGetColorImage(k4a_capture_t capture) const = 0;
        virtual k4a_image_t captureGetDepthImage(k4a_capture_t capture) const = 0;
        virtual void captureRelease(k4a_capture_t capture) const = 0;
        virtual void imageRelease(k4a_image_t image) const = 0;
        virtual std::uint8_t* imageGetBuffer(k4a_image_t image) const = 0;
        virtual std::size_t imageGetSize(k4a_image_t image) const = 0;
        virtual int imageGetWidthPixels(k4a_image_t image) const = 0;
        virtual int imageGetHeightPixels(k4a_image_t image) const = 0;
        virtual int imageGetStrideBytes(k4a_image_t image) const = 0;
        virtual k4a_image_format_t imageGetFormat(k4a_image_t image) const = 0;
        virtual k4a_result_t imageCreate(
            k4a_image_format_t format,
            int widthPixels,
            int heightPixels,
            int strideBytes,
            k4a_image_t* image
        ) const = 0;
        virtual k4a_transformation_t transformationCreate(
            const k4a_calibration_t& calibration
        ) const = 0;
        virtual void transformationDestroy(k4a_transformation_t transformation) const = 0;
        virtual k4a_result_t transformationDepthImageToColorCamera(
            k4a_transformation_t transformation,
            k4a_image_t depthImage,
            k4a_image_t transformedDepthImage
        ) const = 0;

        static const K4aInterface& instance();
    };

} // namespace edgevision::capture
