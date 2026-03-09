#include <cstdio>

#include "CameraCapture.hpp"

namespace {
    struct FakeK4aApi final : public k4aInterface {
        mutable int openCalls = 0;
        mutable uint32_t lastOpenIndex = 0;
        mutable k4a_result_t openResult = K4A_RESULT_SUCCEEDED;
        mutable k4a_device_t deviceToReturn = reinterpret_cast<k4a_device_t>(0x1);

        mutable int closeCalls = 0;
        mutable int startCalls = 0;
        mutable int stopCalls = 0;
        mutable int getCaptureCalls = 0;
        mutable int getCalibrationCalls = 0;

        mutable k4a_result_t startResult = K4A_RESULT_SUCCEEDED;
        mutable k4a_wait_result_t captureResult = K4A_WAIT_RESULT_SUCCEEDED;
        mutable int32_t lastCaptureTimeout = 0;

        mutable k4a_depth_mode_t lastDepthMode = K4A_DEPTH_MODE_OFF;
        mutable k4a_color_resolution_t lastColorResolution = K4A_COLOR_RESOLUTION_OFF;
        mutable k4a_result_t calibrationResult = K4A_RESULT_SUCCEEDED;

        k4a_result_t deviceOpen(uint32_t deviceIndex, k4a_device_t* device) const override {
            ++openCalls;
            lastOpenIndex = deviceIndex;
            if (openResult != K4A_RESULT_SUCCEEDED) {
                return openResult;
            }

            if (device != nullptr) {
                *device = deviceToReturn;
            }

            return openResult;
        }

        void deviceClose(k4a_device_t device) const override {
            (void)device;
            ++closeCalls;
        }

        k4a_result_t deviceStartCameras(
            k4a_device_t device,
            const k4a_device_configuration_t* config
        ) const override {
            (void)device;
            (void)config;
            ++startCalls;
            return startResult;
        }

        void deviceStopCameras(k4a_device_t device) const override {
            (void)device;
            ++stopCalls;
        }

        k4a_wait_result_t deviceGetCapture(
            k4a_device_t device,
            k4a_capture_t* capture,
            int32_t timeoutMs
        ) const override {
            (void)device;
            ++getCaptureCalls;
            lastCaptureTimeout = timeoutMs;
            if (capture != nullptr) {
                *capture = reinterpret_cast<k4a_capture_t>(0x2);
            }

            return captureResult;
        }

        k4a_result_t deviceGetCalibration(
            k4a_device_t device,
            k4a_depth_mode_t depthMode,
            k4a_color_resolution_t colorResolution,
            k4a_calibration_t* calibration
        ) const override {
            (void)device;
            (void)calibration;
            ++getCalibrationCalls;
            lastDepthMode = depthMode;
            lastColorResolution = colorResolution;
            return calibrationResult;
        }
    };

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
#define EXPECT_FALSE(expr) expectTrue(!(expr), #expr, __FILE__, __LINE__)
#define EXPECT_EQ(lhs, rhs) expectEq((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__)

    void testOpenSuccess() {
        FakeK4aApi api;
        Camera capture(api);

        EXPECT_TRUE(capture.open(2));
        EXPECT_TRUE(capture.isOpen());
        EXPECT_EQ(api.openCalls, 1);
        EXPECT_EQ(api.lastOpenIndex, 2U);
    }

    void testOpenFailure() {
        FakeK4aApi api;
        api.openResult = K4A_RESULT_FAILED;
        Camera capture(api);

        EXPECT_FALSE(capture.open(1));
        EXPECT_FALSE(capture.isOpen());
        EXPECT_EQ(api.openCalls, 1);
    }

    void testStartRequiresOpen() {
        FakeK4aApi api;
        Camera capture(api);
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

        EXPECT_FALSE(capture.start(config));
        EXPECT_EQ(api.startCalls, 0);
    }

    void testStartIdempotent() {
        FakeK4aApi api;
        Camera capture(api);
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

        EXPECT_TRUE(capture.open());
        EXPECT_TRUE(capture.start(config));
        EXPECT_TRUE(capture.isStarted());
        EXPECT_EQ(api.startCalls, 1);

        EXPECT_TRUE(capture.start(config));
        EXPECT_EQ(api.startCalls, 1);
    }

    void testStopIdempotent() {
        FakeK4aApi api;
        Camera capture(api);
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

        EXPECT_TRUE(capture.open());
        EXPECT_TRUE(capture.start(config));
        capture.stop();
        EXPECT_FALSE(capture.isStarted());
        EXPECT_EQ(api.stopCalls, 1);

        capture.stop();
        EXPECT_EQ(api.stopCalls, 1);
    }

    void testCaptureGuards() {
        FakeK4aApi api;
        Camera capture(api);
        k4a_capture_t outCapture = nullptr;

        EXPECT_FALSE(capture.capture(outCapture, 10));
        EXPECT_EQ(api.getCaptureCalls, 0);

        EXPECT_TRUE(capture.open());
        EXPECT_FALSE(capture.capture(outCapture, 10));
        EXPECT_EQ(api.getCaptureCalls, 0);

        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        EXPECT_TRUE(capture.start(config));
        api.captureResult = K4A_WAIT_RESULT_TIMEOUT;
        EXPECT_FALSE(capture.capture(outCapture, 25));
        EXPECT_EQ(api.getCaptureCalls, 1);
        EXPECT_EQ(api.lastCaptureTimeout, 25);

        api.captureResult = K4A_WAIT_RESULT_SUCCEEDED;
        EXPECT_TRUE(capture.capture(outCapture, 5));
        EXPECT_EQ(api.getCaptureCalls, 2);
    }

    void testCalibrationUsesConfig() {
        FakeK4aApi api;
        Camera capture(api);
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
        config.color_resolution = K4A_COLOR_RESOLUTION_1080P;

        EXPECT_TRUE(capture.open());
        EXPECT_TRUE(capture.start(config));

        k4a_calibration_t calibration{};
        EXPECT_TRUE(capture.getCalibration(calibration));
        EXPECT_EQ(api.getCalibrationCalls, 1);
        EXPECT_EQ(api.lastDepthMode, config.depth_mode);
        EXPECT_EQ(api.lastColorResolution, config.color_resolution);
    }

    void testCloseResetsState() {
        FakeK4aApi api;
        Camera capture(api);
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;

        EXPECT_TRUE(capture.open());
        EXPECT_TRUE(capture.start(config));
        capture.close();
        EXPECT_FALSE(capture.isOpen());
        EXPECT_FALSE(capture.isStarted());
        EXPECT_EQ(api.closeCalls, 1);

        capture.close();
        EXPECT_EQ(api.closeCalls, 1);
    }
} // namespace

int main() {
    testOpenSuccess();
    testOpenFailure();
    testStartRequiresOpen();
    testStartIdempotent();
    testStopIdempotent();
    testCaptureGuards();
    testCalibrationUsesConfig();
    testCloseResetsState();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
