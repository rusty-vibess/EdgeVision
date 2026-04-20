#include "capture/frame/FrameIngestor.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <optional>
#include <vector>

namespace {
    using edgevision::capture::CameraCapture;
    using edgevision::capture::K4aInterface;
    using edgevision::capture::frame::FrameIngestCode;
    using edgevision::capture::frame::FrameIngestor;
    using namespace edgevision::model::frame;

    struct FakeImage {
        std::vector<std::uint8_t> bytes{};
        int width = 0;
        int height = 0;
        int strideBytes = 0;
        k4a_image_format_t format = K4A_IMAGE_FORMAT_CUSTOM;
        int releaseCalls = 0;
    };

    struct FakeCapture {
        FakeImage* color = nullptr;
        FakeImage* depth = nullptr;
        int releaseCalls = 0;
    };

    [[nodiscard]] k4a_image_t handle(FakeImage& image) {
        return reinterpret_cast<k4a_image_t>(&image);
    }

    [[nodiscard]] k4a_capture_t handle(FakeCapture& capture) {
        return reinterpret_cast<k4a_capture_t>(&capture);
    }

    [[nodiscard]] FakeImage& fakeImage(k4a_image_t image) {
        return *reinterpret_cast<FakeImage*>(image);
    }

    [[nodiscard]] FakeCapture& fakeCapture(k4a_capture_t capture) {
        return *reinterpret_cast<FakeCapture*>(capture);
    }

    struct FakeK4aApi final : public K4aInterface {
        mutable FakeImage transformedDepth = makeTransformedDepthImage();
        mutable int openCalls = 0;
        mutable int startCalls = 0;
        mutable int captureCalls = 0;
        mutable int calibrationCalls = 0;
        mutable int transformationDestroyCalls = 0;
        mutable k4a_wait_result_t captureResult = K4A_WAIT_RESULT_SUCCEEDED;
        mutable k4a_result_t calibrationResult = K4A_RESULT_SUCCEEDED;
        mutable k4a_calibration_t calibrationToReturn{};
        mutable FakeCapture* captureToReturn = nullptr;
        mutable k4a_device_t deviceToReturn = reinterpret_cast<k4a_device_t>(0x1);

        k4a_result_t deviceOpen(uint32_t, k4a_device_t* device) const override {
            ++openCalls;
            if (device != nullptr) {
                *device = deviceToReturn;
            }

            return K4A_RESULT_SUCCEEDED;
        }

        void deviceClose(k4a_device_t) const override {}

        k4a_result_t deviceStartCameras(
            k4a_device_t,
            const k4a_device_configuration_t*
        ) const override {
            ++startCalls;
            return K4A_RESULT_SUCCEEDED;
        }

        void deviceStopCameras(k4a_device_t) const override {}

        k4a_wait_result_t deviceGetCapture(
            k4a_device_t,
            k4a_capture_t* capture,
            int32_t
        ) const override {
            ++captureCalls;
            if (captureResult != K4A_WAIT_RESULT_SUCCEEDED) {
                return captureResult;
            }

            if (capture != nullptr && captureToReturn != nullptr) {
                *capture = handle(*captureToReturn);
            }

            return captureResult;
        }

        k4a_result_t deviceGetCalibration(
            k4a_device_t,
            k4a_depth_mode_t,
            k4a_color_resolution_t,
            k4a_calibration_t* calibration
        ) const override {
            ++calibrationCalls;
            if (calibrationResult != K4A_RESULT_SUCCEEDED) {
                return calibrationResult;
            }

            if (calibration != nullptr) {
                *calibration = calibrationToReturn;
            }

            return calibrationResult;
        }

        k4a_image_t captureGetColorImage(k4a_capture_t capture) const override {
            FakeImage* image = fakeCapture(capture).color;
            return image != nullptr ? handle(*image) : nullptr;
        }

        k4a_image_t captureGetDepthImage(k4a_capture_t capture) const override {
            FakeImage* image = fakeCapture(capture).depth;
            return image != nullptr ? handle(*image) : nullptr;
        }

        void captureRelease(k4a_capture_t capture) const override {
            ++fakeCapture(capture).releaseCalls;
        }

        void imageRelease(k4a_image_t image) const override {
            ++fakeImage(image).releaseCalls;
        }

        std::uint8_t* imageGetBuffer(k4a_image_t image) const override {
            return fakeImage(image).bytes.data();
        }

        std::size_t imageGetSize(k4a_image_t image) const override {
            return fakeImage(image).bytes.size();
        }

        int imageGetWidthPixels(k4a_image_t image) const override {
            return fakeImage(image).width;
        }

        int imageGetHeightPixels(k4a_image_t image) const override {
            return fakeImage(image).height;
        }

        int imageGetStrideBytes(k4a_image_t image) const override {
            return fakeImage(image).strideBytes;
        }

        k4a_image_format_t imageGetFormat(k4a_image_t image) const override {
            return fakeImage(image).format;
        }

        k4a_result_t imageCreate(
            k4a_image_format_t format,
            int widthPixels,
            int heightPixels,
            int strideBytes,
            k4a_image_t* image
        ) const override {
            transformedDepth.format = format;
            transformedDepth.width = widthPixels;
            transformedDepth.height = heightPixels;
            transformedDepth.strideBytes = strideBytes;
            transformedDepth.bytes.assign(
                static_cast<std::size_t>(strideBytes * heightPixels), std::uint8_t{0}
            );
            transformedDepth.releaseCalls = 0;

            if (image != nullptr) {
                *image = handle(transformedDepth);
            }

            return K4A_RESULT_SUCCEEDED;
        }

        k4a_transformation_t transformationCreate(const k4a_calibration_t&) const override {
            return reinterpret_cast<k4a_transformation_t>(0x3);
        }

        void transformationDestroy(k4a_transformation_t) const override {
            ++transformationDestroyCalls;
        }

        k4a_result_t transformationDepthImageToColorCamera(
            k4a_transformation_t,
            k4a_image_t depthImage,
            k4a_image_t transformedDepthImage
        ) const override {
            fakeImage(transformedDepthImage).bytes = fakeImage(depthImage).bytes;
            return K4A_RESULT_SUCCEEDED;
        }

      private:
        [[nodiscard]] static FakeImage makeTransformedDepthImage() {
            FakeImage image{};
            image.width = 2;
            image.height = 2;
            image.strideBytes = 4;
            image.format = K4A_IMAGE_FORMAT_DEPTH16;
            image.bytes = {0, 0, 0, 0, 0, 0, 0, 0};
            return image;
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

    [[nodiscard]] FakeImage makeColorImage() {
        FakeImage image{};
        image.width = 2;
        image.height = 2;
        image.strideBytes = 8;
        image.format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        image.bytes = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
        return image;
    }

    [[nodiscard]] FakeImage makeDepthImage() {
        FakeImage image{};
        image.width = 2;
        image.height = 2;
        image.strideBytes = 4;
        image.format = K4A_IMAGE_FORMAT_DEPTH16;
        image.bytes = {1, 0, 2, 0, 3, 0, 4, 0};
        return image;
    }

    [[nodiscard]] k4a_calibration_t makeCalibration() {
        k4a_calibration_t calibration{};
        auto& params = calibration.color_camera_calibration.intrinsics.parameters.param;
        params.fx = 525.0f;
        params.fy = 526.0f;
        params.cx = 1.0f;
        params.cy = 2.0f;
        return calibration;
    }

    [[nodiscard]] k4a_device_configuration_t makeConfig() {
        k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
        config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        config.color_resolution = K4A_COLOR_RESOLUTION_720P;
        config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
        config.camera_fps = K4A_FRAMES_PER_SECOND_30;
        config.synchronized_images_only = true;
        return config;
    }

    [[nodiscard]] Frame makeModelFrame(FrameId frameId, std::int64_t timestampTicks) {
        auto colorStorage = std::make_shared<std::vector<std::byte>>(16);
        auto depthStorage = std::make_shared<std::vector<std::byte>>(8);

        Frame frame{};
        frame.frameId = frameId;
        frame.timestamp = FrameTimestamp{timestampTicks};
        frame.rgb.size = ImageSize{2, 2};
        frame.rgb.strideBytes = 8;
        frame.rgb.buffer = FrameBuffer(colorStorage->data(), colorStorage->size(), colorStorage);
        frame.colorFormat = FrameColorFormat::Bgra32;
        frame.depth.size = ImageSize{2, 2};
        frame.depth.strideBytes = 4;
        frame.depth.buffer = FrameBuffer(depthStorage->data(), depthStorage->size(), depthStorage);
        frame.depthFormat = FrameDepthFormat::Depth16Millimeters;
        frame.intrinsics = CameraIntrinsics{525.0f, 526.0f, 1.0f, 2.0f};
        frame.cameraConfig.rgbResolution = frame.rgb.size;
        frame.cameraConfig.depthResolution = frame.depth.size;
        frame.cameraConfig.colorFormat = frame.colorFormat;
        frame.cameraConfig.depthFormat = frame.depthFormat;
        frame.cameraConfig.depthScaleToMeters = 0.001f;
        frame.cameraConfig.frameRateFps = 30;
        frame.cameraConfig.synchronizedImagesOnly = true;
        return frame;
    }

    struct IngestFixture {
        FakeImage color = makeColorImage();
        FakeImage depth = makeDepthImage();
        FakeCapture rawCapture{&color, &depth};
        FakeK4aApi api{};
        CameraCapture camera;
        FrameStore store{};
        FrameIngestor ingestor;

        IngestFixture() : camera(api), ingestor(camera, store, api) {
            api.captureToReturn = &rawCapture;
            api.calibrationToReturn = makeCalibration();
            EXPECT_TRUE(camera.open());
            EXPECT_TRUE(camera.start(makeConfig()));
        }
    };

    void testSuccessfulCaptureSubmitsFrame() {
        IngestFixture fixture;

        const auto result = fixture.ingestor.captureAndSubmit(10);

        EXPECT_TRUE(result.submitted());
        EXPECT_EQ(result.code, FrameIngestCode::Submitted);
        EXPECT_EQ(result.frameId, FrameId{1});
        EXPECT_EQ(fixture.rawCapture.releaseCalls, 1);
        const std::optional<Frame> latest = fixture.store.getLatestFrame();
        EXPECT_TRUE(latest.has_value());
        EXPECT_EQ(latest->frameId, FrameId{1});
        EXPECT_EQ(latest->colorFormat, FrameColorFormat::Bgra32);
    }

    void testCaptureFailureDoesNotSubmit() {
        IngestFixture fixture;
        fixture.api.captureResult = K4A_WAIT_RESULT_TIMEOUT;

        const auto result = fixture.ingestor.captureAndSubmit(10);

        EXPECT_EQ(result.code, FrameIngestCode::CaptureUnavailable);
        EXPECT_FALSE(fixture.store.getLatestFrame().has_value());
    }

    void testCalibrationFailureReleasesCaptureAndDoesNotSubmit() {
        IngestFixture fixture;
        fixture.api.calibrationResult = K4A_RESULT_FAILED;

        const auto result = fixture.ingestor.captureAndSubmit(10);

        EXPECT_EQ(result.code, FrameIngestCode::CalibrationUnavailable);
        EXPECT_EQ(fixture.rawCapture.releaseCalls, 1);
        EXPECT_FALSE(fixture.store.getLatestFrame().has_value());
    }

    void testBuildFailureDoesNotSubmit() {
        IngestFixture fixture;
        fixture.rawCapture.depth = nullptr;

        const auto result = fixture.ingestor.captureAndSubmit(10);

        EXPECT_EQ(result.code, FrameIngestCode::BuildFailed);
        EXPECT_EQ(fixture.rawCapture.releaseCalls, 1);
        EXPECT_FALSE(fixture.store.getLatestFrame().has_value());
    }

    void testModelRejectionIsSurfaced() {
        IngestFixture fixture;
        EXPECT_TRUE(fixture.store.submitFrame(makeModelFrame(1, 1)).accepted());

        const auto result = fixture.ingestor.captureAndSubmit(10);

        EXPECT_EQ(result.code, FrameIngestCode::ModelRejected);
        EXPECT_EQ(result.submissionCode.value(), FrameSubmissionCode::FrameIdNotNewer);
        EXPECT_EQ(fixture.store.getLatestFrame()->frameId, FrameId{1});
    }

    void testGeneratedIdsAndTimestampsAreMonotonic() {
        IngestFixture fixture;

        EXPECT_TRUE(fixture.ingestor.captureAndSubmit(10).submitted());
        EXPECT_TRUE(fixture.ingestor.captureAndSubmit(10).submitted());

        const std::vector<Frame> frames = fixture.store.getRecentFrames(2);
        EXPECT_EQ(frames.size(), std::size_t{2});
        EXPECT_EQ(frames[0].frameId, FrameId{1});
        EXPECT_EQ(frames[1].frameId, FrameId{2});
        EXPECT_TRUE(frames[1].timestamp.ticks > frames[0].timestamp.ticks);
    }
} // namespace

int main() {
    testSuccessfulCaptureSubmitsFrame();
    testCaptureFailureDoesNotSubmit();
    testCalibrationFailureReleasesCaptureAndDoesNotSubmit();
    testBuildFailureDoesNotSubmit();
    testModelRejectionIsSurfaced();
    testGeneratedIdsAndTimestampsAreMonotonic();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
