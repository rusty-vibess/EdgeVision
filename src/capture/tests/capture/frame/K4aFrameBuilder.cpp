#include "capture/frame/K4aFrameBuilder.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace {
    using edgevision::capture::K4aInterface;
    using edgevision::capture::frame::FrameBuildCode;
    using edgevision::capture::frame::K4aFrameBuilder;
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
        mutable int transformationCreateCalls = 0;
        mutable int transformationDestroyCalls = 0;
        mutable int imageCreateCalls = 0;
        mutable int transformDepthCalls = 0;
        mutable bool transformationCreateSucceeds = true;
        mutable k4a_result_t imageCreateResult = K4A_RESULT_SUCCEEDED;
        mutable k4a_result_t transformDepthResult = K4A_RESULT_SUCCEEDED;

        k4a_result_t deviceOpen(uint32_t, k4a_device_t*) const override {
            return K4A_RESULT_FAILED;
        }

        void deviceClose(k4a_device_t) const override {}

        k4a_result_t deviceStartCameras(
            k4a_device_t,
            const k4a_device_configuration_t*
        ) const override {
            return K4A_RESULT_FAILED;
        }

        void deviceStopCameras(k4a_device_t) const override {}

        k4a_wait_result_t deviceGetCapture(k4a_device_t, k4a_capture_t*, int32_t) const override {
            return K4A_WAIT_RESULT_FAILED;
        }

        k4a_result_t deviceGetCalibration(
            k4a_device_t,
            k4a_depth_mode_t,
            k4a_color_resolution_t,
            k4a_calibration_t*
        ) const override {
            return K4A_RESULT_FAILED;
        }

        k4a_image_t captureGetColorImage(k4a_capture_t capture) const override {
            FakeImage* image = fakeCapture(capture).color;
            return image != nullptr ? handle(*image) : nullptr;
        }

        k4a_image_t captureGetDepthImage(k4a_capture_t capture) const override {
            FakeImage* image = fakeCapture(capture).depth;
            return image != nullptr ? handle(*image) : nullptr;
        }

        void captureRelease(k4a_capture_t) const override {}

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
            ++imageCreateCalls;
            if (imageCreateResult != K4A_RESULT_SUCCEEDED) {
                return imageCreateResult;
            }

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
            ++transformationCreateCalls;
            if (!transformationCreateSucceeds) {
                return nullptr;
            }

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
            ++transformDepthCalls;
            if (transformDepthResult != K4A_RESULT_SUCCEEDED) {
                return transformDepthResult;
            }

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

    void testBuildsValidFrameWithCopiedBuffers() {
        FakeK4aApi api;
        K4aFrameBuilder builder(api);
        FakeImage color = makeColorImage();
        FakeImage depth = makeDepthImage();
        FakeCapture capture{&color, &depth};

        auto result = builder.build(handle(capture), makeCalibration(), 7, {100});

        EXPECT_TRUE(result.built());
        EXPECT_EQ(result.code, FrameBuildCode::Built);
        EXPECT_EQ(result.frame->frameId, FrameId{7});
        EXPECT_EQ(result.frame->timestamp.ticks, 100);
        const ImageSize expectedSize{2, 2};
        EXPECT_EQ(result.frame->rgb.size, expectedSize);
        EXPECT_EQ(result.frame->rgb.strideBytes, 8);
        EXPECT_EQ(result.frame->depth.size, expectedSize);
        EXPECT_EQ(result.frame->depth.strideBytes, 4);
        EXPECT_EQ(result.frame->intrinsics.fx, 525.0f);
        EXPECT_EQ(api.transformationCreateCalls, 1);
        EXPECT_EQ(api.imageCreateCalls, 1);
        EXPECT_EQ(api.transformDepthCalls, 1);
        EXPECT_EQ(api.transformationDestroyCalls, 1);

        color.bytes[0] = 99;
        EXPECT_EQ(std::to_integer<std::uint8_t>(result.frame->rgb.buffer.data[0]), 1U);
        EXPECT_EQ(color.releaseCalls, 1);
        EXPECT_EQ(depth.releaseCalls, 1);
        EXPECT_EQ(api.transformedDepth.releaseCalls, 1);
    }

    void testMissingDepthFailsAndReleasesColor() {
        FakeK4aApi api;
        K4aFrameBuilder builder(api);
        FakeImage color = makeColorImage();
        FakeCapture capture{&color, nullptr};

        auto result = builder.build(handle(capture), makeCalibration(), 1, {100});

        EXPECT_FALSE(result.built());
        EXPECT_EQ(result.code, FrameBuildCode::MissingDepthImage);
        EXPECT_EQ(color.releaseCalls, 1);
    }

    void testUnsupportedFormatsFail() {
        FakeK4aApi api;
        K4aFrameBuilder builder(api);
        FakeImage color = makeColorImage();
        FakeImage depth = makeDepthImage();
        FakeCapture capture{&color, &depth};

        color.format = K4A_IMAGE_FORMAT_COLOR_MJPG;
        auto colorResult = builder.build(handle(capture), makeCalibration(), 1, {100});
        EXPECT_EQ(colorResult.code, FrameBuildCode::UnsupportedColorFormat);

        color.format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        depth.format = K4A_IMAGE_FORMAT_CUSTOM;
        auto depthResult = builder.build(handle(capture), makeCalibration(), 2, {200});
        EXPECT_EQ(depthResult.code, FrameBuildCode::UnsupportedDepthFormat);
    }

    void testInvalidImageShapeFails() {
        FakeK4aApi api;
        K4aFrameBuilder builder(api);
        FakeImage color = makeColorImage();
        FakeImage depth = makeDepthImage();
        FakeCapture capture{&color, &depth};

        color.strideBytes = 1;
        auto result = builder.build(handle(capture), makeCalibration(), 1, {100});

        EXPECT_FALSE(result.built());
        EXPECT_EQ(result.code, FrameBuildCode::InvalidColorImage);
    }

    void testInvalidIntrinsicsFail() {
        FakeK4aApi api;
        K4aFrameBuilder builder(api);
        FakeImage color = makeColorImage();
        FakeImage depth = makeDepthImage();
        FakeCapture capture{&color, &depth};
        k4a_calibration_t calibration = makeCalibration();
        calibration.color_camera_calibration.intrinsics.parameters.param.fx = 0.0f;

        auto result = builder.build(handle(capture), calibration, 1, {100});

        EXPECT_FALSE(result.built());
        EXPECT_EQ(result.code, FrameBuildCode::InvalidIntrinsics);
    }

    void testTransformationCreateFailureFails() {
        FakeK4aApi api;
        api.transformationCreateSucceeds = false;
        K4aFrameBuilder builder(api);
        FakeImage color = makeColorImage();
        FakeImage depth = makeDepthImage();
        FakeCapture capture{&color, &depth};

        auto result = builder.build(handle(capture), makeCalibration(), 1, {100});

        EXPECT_FALSE(result.built());
        EXPECT_EQ(result.code, FrameBuildCode::TransformationCreateFailed);
    }

    void testTransformedDepthImageCreateFailureFails() {
        FakeK4aApi api;
        api.imageCreateResult = K4A_RESULT_FAILED;
        K4aFrameBuilder builder(api);
        FakeImage color = makeColorImage();
        FakeImage depth = makeDepthImage();
        FakeCapture capture{&color, &depth};

        auto result = builder.build(handle(capture), makeCalibration(), 1, {100});

        EXPECT_FALSE(result.built());
        EXPECT_EQ(result.code, FrameBuildCode::TransformedDepthImageCreateFailed);
        EXPECT_EQ(api.transformationDestroyCalls, 1);
    }

    void testDepthAlignmentFailureFails() {
        FakeK4aApi api;
        api.transformDepthResult = K4A_RESULT_FAILED;
        K4aFrameBuilder builder(api);
        FakeImage color = makeColorImage();
        FakeImage depth = makeDepthImage();
        FakeCapture capture{&color, &depth};

        auto result = builder.build(handle(capture), makeCalibration(), 1, {100});

        EXPECT_FALSE(result.built());
        EXPECT_EQ(result.code, FrameBuildCode::DepthAlignmentFailed);
        EXPECT_EQ(api.transformedDepth.releaseCalls, 1);
        EXPECT_EQ(api.transformationDestroyCalls, 1);
    }
} // namespace

int main() {
    testBuildsValidFrameWithCopiedBuffers();
    testMissingDepthFailsAndReleasesColor();
    testUnsupportedFormatsFail();
    testInvalidImageShapeFails();
    testInvalidIntrinsicsFail();
    testTransformationCreateFailureFails();
    testTransformedDepthImageCreateFailureFails();
    testDepthAlignmentFailureFails();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
