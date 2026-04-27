#include "app/runtime/ViewerDebugSession.hpp"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include "model/viewer/RenderOutputStore.hpp"

namespace {
    using edgevision::app::runtime::computeViewerDebugRates;
    using edgevision::app::runtime::ViewerDebugCounters;
    using edgevision::app::runtime::ViewerDebugRates;
    using edgevision::app::runtime::ViewerDebugSession;
    using edgevision::config::ViewerDumpConfig;
    using edgevision::model::frame::FrameBuffer;
    using edgevision::model::frame::ImageSize;
    using edgevision::model::scene::SceneVersionId;
    using edgevision::model::viewer::RenderOutput;
    using edgevision::model::viewer::RenderOutputStore;
    using edgevision::model::viewer::ViewerPoseGeneration;

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

    void expectNear(
        double lhs,
        double rhs,
        double tolerance,
        const char* lhsExpr,
        const char* rhsExpr,
        const char* file,
        int line
    ) {
        if (std::fabs(lhs - rhs) > tolerance) {
            std::fprintf(
                stderr,
                "%s:%d: expected %s ~= %s (got %.6f vs %.6f)\n",
                file,
                line,
                lhsExpr,
                rhsExpr,
                lhs,
                rhs
            );
            ++gFailures;
        }
    }

#define EXPECT_TRUE(expr) expectTrue((expr), #expr, __FILE__, __LINE__)
#define EXPECT_NEAR(lhs, rhs, tolerance) \
    expectNear((lhs), (rhs), (tolerance), #lhs, #rhs, __FILE__, __LINE__)

    FrameBuffer makeBuffer(std::uint8_t seed) {
        auto storage = std::make_shared<std::vector<std::byte>>(12);
        for (std::size_t index = 0; index < storage->size(); ++index) {
            (*storage)[index] = static_cast<std::byte>(seed + index);
        }

        return FrameBuffer(
            storage->data(), storage->size(), std::shared_ptr<const void>(storage, storage.get())
        );
    }

    RenderOutput makeOutput(
        ViewerPoseGeneration poseGeneration,
        SceneVersionId sceneVersionId,
        bool cached,
        std::int64_t timestampTicks
    ) {
        RenderOutput output{};
        output.rgb = makeBuffer(static_cast<std::uint8_t>(poseGeneration));
        output.imageSize = ImageSize{2, 2};
        output.poseGeneration = poseGeneration;
        output.sceneVersionId = sceneVersionId;
        output.renderTimestamp =
            std::chrono::steady_clock::time_point(std::chrono::milliseconds(timestampTicks));
        output.cached = cached;
        return output;
    }

    void testComputeViewerDebugRatesUsesCounterDeltas() {
        ViewerDebugCounters begin{};
        begin.submittedFrameCount = 100;
        begin.consumedFrameCount = 50;
        begin.publishedOutputCount = 80;
        begin.cachedOutputCount = 20;

        ViewerDebugCounters end{};
        end.submittedFrameCount = 130;
        end.consumedFrameCount = 74;
        end.publishedOutputCount = 110;
        end.cachedOutputCount = 45;

        const ViewerDebugRates rates =
            computeViewerDebugRates(begin, end, std::chrono::seconds(2));

        EXPECT_NEAR(rates.ingestFps, 15.0, 0.0001);
        EXPECT_NEAR(rates.buildFps, 12.0, 0.0001);
        EXPECT_NEAR(rates.viewFps, 15.0, 0.0001);
        EXPECT_NEAR(rates.cachedViewFps, 12.5, 0.0001);
        EXPECT_NEAR(rates.uncachedViewFps, 2.5, 0.0001);
    }

    void testComputeViewerDebugRatesHandlesZeroElapsedTime() {
        const ViewerDebugRates rates =
            computeViewerDebugRates({}, {}, std::chrono::duration<double>(0.0));

        EXPECT_NEAR(rates.ingestFps, 0.0, 0.0001);
        EXPECT_NEAR(rates.buildFps, 0.0, 0.0001);
        EXPECT_NEAR(rates.viewFps, 0.0, 0.0001);
        EXPECT_NEAR(rates.cachedViewFps, 0.0, 0.0001);
        EXPECT_NEAR(rates.uncachedViewFps, 0.0, 0.0001);
    }

    void testDebugSessionWritesMetricsLogAlongsideFrames() {
        const std::filesystem::path outputDirectory =
            std::filesystem::temp_directory_path() / "edgevision-viewer-debug-session";
        std::filesystem::remove_all(outputDirectory);

        RenderOutputStore store{4};
        store.publish(makeOutput(1, 10, false, 1));
        store.publish(makeOutput(1, 10, true, 2));
        store.publish(makeOutput(2, 11, false, 3));

        ViewerDumpConfig config{};
        config.enabled = true;
        config.maxFrames = 2;

        ViewerDebugSession session(
            store,
            config,
            []() {
                ViewerDebugCounters counters{};
                counters.submittedFrameCount = 2;
                counters.consumedFrameCount = 2;
                counters.publishedOutputCount = 1;
                return counters;
            },
            outputDirectory
        );

        EXPECT_TRUE(session.waitForCompletion(std::chrono::milliseconds(10)));
        EXPECT_TRUE(std::filesystem::exists(outputDirectory / "viewer-output-0001.ppm"));
        EXPECT_TRUE(std::filesystem::exists(outputDirectory / "viewer-output-0002.ppm"));
        EXPECT_TRUE(std::filesystem::exists(session.logPath()));
        EXPECT_TRUE(std::filesystem::file_size(session.logPath()) > 0);
        EXPECT_TRUE(session.skippedDuplicateCount() == 1);

        std::ifstream stream(session.logPath());
        const std::string logContents(
            (std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>()
        );
        EXPECT_TRUE(logContents.find("skipped_duplicates=1") != std::string::npos);
    }
} // namespace

int main() {
    testComputeViewerDebugRatesUsesCounterDeltas();
    testComputeViewerDebugRatesHandlesZeroElapsedTime();
    testDebugSessionWritesMetricsLogAlongsideFrames();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
