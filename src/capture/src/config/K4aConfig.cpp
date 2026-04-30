#include "capture/config/K4aConfig.hpp"

namespace edgevision::capture {
    namespace {

        [[nodiscard]] k4a_image_format_t colorFormat(
            edgevision::config::CaptureColorFormat format
        ) {
            switch (format) {
                case edgevision::config::CaptureColorFormat::Bgra32:
                    return K4A_IMAGE_FORMAT_COLOR_BGRA32;
            }

            return K4A_IMAGE_FORMAT_COLOR_BGRA32;
        }

        [[nodiscard]] std::optional<k4a_color_resolution_t> colorResolution(
            const edgevision::config::ImageSize& imageSize
        ) {
            if (imageSize == edgevision::config::ImageSize{1280, 720}) {
                return K4A_COLOR_RESOLUTION_720P;
            }

            if (imageSize == edgevision::config::ImageSize{1920, 1080}) {
                return K4A_COLOR_RESOLUTION_1080P;
            }

            if (imageSize == edgevision::config::ImageSize{2560, 1440}) {
                return K4A_COLOR_RESOLUTION_1440P;
            }

            if (imageSize == edgevision::config::ImageSize{2048, 1536}) {
                return K4A_COLOR_RESOLUTION_1536P;
            }

            if (imageSize == edgevision::config::ImageSize{3840, 2160}) {
                return K4A_COLOR_RESOLUTION_2160P;
            }

            if (imageSize == edgevision::config::ImageSize{4096, 3072}) {
                return K4A_COLOR_RESOLUTION_3072P;
            }

            return std::nullopt;
        }

        [[nodiscard]] k4a_depth_mode_t depthMode(edgevision::config::CaptureDepthMode mode) {
            switch (mode) {
                case edgevision::config::CaptureDepthMode::Nfov2x2Binned:
                    return K4A_DEPTH_MODE_NFOV_2X2BINNED;
                case edgevision::config::CaptureDepthMode::NfovUnbinned:
                    return K4A_DEPTH_MODE_NFOV_UNBINNED;
                case edgevision::config::CaptureDepthMode::Wfov2x2Binned:
                    return K4A_DEPTH_MODE_WFOV_2X2BINNED;
                case edgevision::config::CaptureDepthMode::WfovUnbinned:
                    return K4A_DEPTH_MODE_WFOV_UNBINNED;
                case edgevision::config::CaptureDepthMode::PassiveIr:
                    return K4A_DEPTH_MODE_PASSIVE_IR;
            }

            return K4A_DEPTH_MODE_NFOV_UNBINNED;
        }

        [[nodiscard]] k4a_fps_t frameRate(edgevision::config::CaptureFrameRate rate) {
            switch (rate) {
                case edgevision::config::CaptureFrameRate::Fps5:
                    return K4A_FRAMES_PER_SECOND_5;
                case edgevision::config::CaptureFrameRate::Fps15:
                    return K4A_FRAMES_PER_SECOND_15;
                case edgevision::config::CaptureFrameRate::Fps30:
                    return K4A_FRAMES_PER_SECOND_30;
            }

            return K4A_FRAMES_PER_SECOND_30;
        }

    } // namespace

    std::optional<k4a_device_configuration_t> makeK4aDeviceConfig(
        const edgevision::config::CaptureCameraConfig& config,
        const edgevision::config::ImageSize& imageSize
    ) {
        const std::optional<k4a_color_resolution_t> resolvedColorResolution =
            colorResolution(imageSize);
        if (!resolvedColorResolution.has_value()) {
            return std::nullopt;
        }

        k4a_device_configuration_t deviceConfig = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        deviceConfig.color_format = colorFormat(config.colorFormat);
        deviceConfig.color_resolution = *resolvedColorResolution;
        deviceConfig.depth_mode = depthMode(config.depthMode);
        deviceConfig.camera_fps = frameRate(config.frameRate);
        deviceConfig.synchronized_images_only = config.synchronizedImagesOnly;
        return deviceConfig;
    }

} // namespace edgevision::capture
