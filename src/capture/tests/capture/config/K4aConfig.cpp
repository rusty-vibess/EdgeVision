#include "capture/config/K4aConfig.hpp"

#include <cstdio>

namespace {
    using edgevision::capture::makeK4aDeviceConfig;
    using edgevision::config::CaptureCameraConfig;
    using edgevision::config::CaptureColorResolution;
    using edgevision::config::CaptureDepthMode;
    using edgevision::config::CaptureFrameRate;

    int gFailures = 0;

    void recordFailure(const char* message, const char* file, int line) {
        std::fprintf(stderr, "%s:%d: %s\n", file, line, message);
        ++gFailures;
    }

    void expectTrue(bool value, const char* expr, const char* file, int line) {
        if (!value) {
            recordFailure(expr, file, line);
        }
    }

    template <typename T, typename U>
    void expectEq(
        const T& lhs,
        const U& rhs,
        const char* lhsExpr,
        const char* rhsExpr,
        const char* file,
        int line
    ) {
        if (!(lhs == rhs)) {
            std::fprintf(stderr, "%s:%d: expected %s == %s\n", file, line, lhsExpr, rhsExpr);
            ++gFailures;
        }
    }

#define EXPECT_TRUE(expr) expectTrue((expr), #expr, __FILE__, __LINE__)
#define EXPECT_EQ(lhs, rhs) expectEq((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__)

    void testDefaultConfigMapsToK4aDefaults() {
        const k4a_device_configuration_t config = makeK4aDeviceConfig(CaptureCameraConfig{});

        EXPECT_EQ(config.color_format, K4A_IMAGE_FORMAT_COLOR_BGRA32);
        EXPECT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_720P);
        EXPECT_EQ(config.depth_mode, K4A_DEPTH_MODE_NFOV_UNBINNED);
        EXPECT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_30);
        EXPECT_TRUE(config.synchronized_images_only);
    }

    void testOverridesMapToK4aValues() {
        CaptureCameraConfig cameraConfig{};
        cameraConfig.colorResolution = CaptureColorResolution::P1080;
        cameraConfig.depthMode = CaptureDepthMode::Wfov2x2Binned;
        cameraConfig.frameRate = CaptureFrameRate::Fps15;
        cameraConfig.synchronizedImagesOnly = false;

        const k4a_device_configuration_t config = makeK4aDeviceConfig(cameraConfig);

        EXPECT_EQ(config.color_resolution, K4A_COLOR_RESOLUTION_1080P);
        EXPECT_EQ(config.depth_mode, K4A_DEPTH_MODE_WFOV_2X2BINNED);
        EXPECT_EQ(config.camera_fps, K4A_FRAMES_PER_SECOND_15);
        EXPECT_EQ(config.synchronized_images_only, false);
    }
} // namespace

int main() {
    testDefaultConfigMapsToK4aDefaults();
    testOverridesMapToK4aValues();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
