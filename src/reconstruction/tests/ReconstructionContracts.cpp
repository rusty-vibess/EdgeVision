#include <cstddef>
#include <cstdio>

#include "interfaces/GaussianResidualModel.hpp"
#include "interfaces/TsdfRaycaster.hpp"

namespace {
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

    struct FakeTsdfRaycaster final : public ITsdfRaycaster {
        TsdfRaycastBuffers result{};

        [[nodiscard]] TsdfRaycastBuffers raycast(
            const RgbdFrameView& frame,
            bool useCameraDepth
        ) const override {
            (void)frame;
            (void)useCameraDepth;
            return result;
        }
    };

    struct FakeGaussianResidualModel final : public IGaussianResidualModel {
        int seedCalls = 0;
        float optimizeResult = 0.0f;
        HybridRenderOutput renderResult{};

        void seedFromResidual(
            const RgbdFrameView& frame,
            const TsdfRaycastBuffers& raycast,
            const ResidualSeedConfig& config
        ) override {
            (void)frame;
            (void)raycast;
            (void)config;
            ++seedCalls;
        }

        [[nodiscard]] HybridRenderOutput renderHybrid(const HybridRenderInput& input) override {
            (void)input;
            return renderResult;
        }

        [[nodiscard]] float optimizeLocalWindow(
            BufferView<RgbdFrameView> frames,
            BufferView<TsdfRaycastBuffers> raycasts
        ) override {
            (void)frames;
            (void)raycasts;
            return optimizeResult;
        }
    };

    void testBufferView() {
        const int values[] = {4, 5, 6};
        BufferView<int> view(values, 3);

        EXPECT_FALSE(view.empty());
        EXPECT_EQ(view.size(), static_cast<std::size_t>(3));
        EXPECT_EQ(view.data(), values);
        EXPECT_EQ(view[1], 5);
        EXPECT_EQ(*view.begin(), 4);
        EXPECT_EQ(*(view.end() - 1), 6);
    }

    void testPlainContracts() {
        const float rgbValues[] = {0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f};
        const float depthValues[] = {1.0f, 2.0f};
        const float vertexValues[] = {0.0f, 0.1f, 0.2f, 1.0f, 1.1f, 1.2f};
        const float confidenceValues[] = {0.8f, 0.9f};
        const int radiiValues[] = {3, 7};

        ImageSize size{640, 480};
        CameraIntrinsics intrinsics{525.0f, 525.0f, 319.5f, 239.5f};
        Pose4f pose{};
        pose.matrix[0] = 1.0f;
        pose.matrix[5] = 1.0f;
        pose.matrix[10] = 1.0f;
        pose.matrix[15] = 1.0f;

        RgbdFrameView frame{};
        frame.rgb = RgbFrameView{size, BufferView<float>(rgbValues, 6)};
        frame.depth = DepthFrameView{size, BufferView<float>(depthValues, 2)};
        frame.intrinsics = intrinsics;
        frame.cameraToWorld = pose;
        frame.cameraToWorldSlam = pose;
        frame.hasDepth = true;

        TsdfRaycastBuffers raycast{};
        raycast.size = size;
        raycast.colorMap = BufferView<float>(rgbValues, 6);
        raycast.vertexMap = BufferView<float>(vertexValues, 6);
        raycast.confidenceMap = BufferView<float>(confidenceValues, 2);
        raycast.depthMap = BufferView<float>(depthValues, 2);
        raycast.normalMap = BufferView<float>(vertexValues, 6);

        HybridRenderOutput renderOutput{};
        renderOutput.size = size;
        renderOutput.rgb = BufferView<float>(rgbValues, 6);
        renderOutput.depth = BufferView<float>(depthValues, 2);
        renderOutput.alpha = BufferView<float>(confidenceValues, 2);
        renderOutput.radii = BufferView<int>(radiiValues, 2);
        renderOutput.means2d = BufferView<float>(vertexValues, 6);

        EXPECT_EQ(frame.rgb.data.size(), static_cast<std::size_t>(6));
        EXPECT_EQ(frame.depth.data.size(), static_cast<std::size_t>(2));
        EXPECT_EQ(frame.intrinsics.fx, 525.0f);
        EXPECT_TRUE(frame.hasDepth);
        EXPECT_EQ(raycast.vertexMap.size(), static_cast<std::size_t>(6));
        EXPECT_EQ(renderOutput.radii[1], 7);
    }

    void testInterfaces() {
        const float values[] = {0.0f, 1.0f, 2.0f};
        const int radiiValues[] = {1};

        RgbdFrameView frames[] = {RgbdFrameView{}};
        TsdfRaycastBuffers raycasts[] = {TsdfRaycastBuffers{}};

        frames[0].rgb.data = BufferView<float>(values, 3);
        raycasts[0].colorMap = BufferView<float>(values, 3);

        FakeTsdfRaycaster raycaster;
        raycaster.result = raycasts[0];
        EXPECT_EQ(raycaster.raycast(frames[0], true).colorMap.size(), static_cast<std::size_t>(3));

        FakeGaussianResidualModel model;
        model.optimizeResult = 1.25f;
        model.renderResult.radii = BufferView<int>(radiiValues, 1);

        ResidualSeedConfig config{};
        config.colorErrorThreshold = 0.05f;
        model.seedFromResidual(frames[0], raycasts[0], config);

        HybridRenderInput input{};
        input.frame = frames[0];
        input.raycast = raycasts[0];
        input.deltaDepth = 0.1f;

        EXPECT_EQ(model.seedCalls, 1);
        EXPECT_EQ(model.renderHybrid(input).radii[0], 1);
        EXPECT_EQ(
            model.optimizeLocalWindow(
                BufferView<RgbdFrameView>(frames, 1), BufferView<TsdfRaycastBuffers>(raycasts, 1)
            ),
            1.25f
        );
    }
} // namespace

int main() {
    testBufferView();
    testPlainContracts();
    testInterfaces();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
