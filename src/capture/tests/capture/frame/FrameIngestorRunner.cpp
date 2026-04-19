#include "capture/frame/FrameIngestorRunner.hpp"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <thread>

#include "capture/camera/CameraCapture.hpp"
#include "model/frame/FrameStore.hpp"

namespace {
    using edgevision::capture::CameraCapture;
    using edgevision::capture::frame::FrameIngestCode;
    using edgevision::capture::frame::FrameIngestor;
    using edgevision::capture::frame::FrameIngestorRunner;
    using edgevision::config::CaptureRuntimeConfig;
    using edgevision::model::frame::FrameStore;

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

    CaptureRuntimeConfig makeFastRuntimeConfig() {
        CaptureRuntimeConfig config{};
        config.captureTimeoutMs = 0;
        config.failureBackoffMs = 1;
        return config;
    }

    bool waitForAttempts(const FrameIngestorRunner& runner) {
        for (int i = 0; i < 50; ++i) {
            if (runner.status().ingestAttemptCount > 0) {
                return true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return false;
    }

    void testJoinBeforeStartIsNoop() {
        CameraCapture camera{};
        FrameStore store{};
        FrameIngestor ingestor(camera, store);
        FrameIngestorRunner runner(ingestor, makeFastRuntimeConfig());

        runner.join();

        const auto status = runner.status();
        EXPECT_FALSE(runner.running());
        EXPECT_FALSE(status.running);
        EXPECT_EQ(status.ingestAttemptCount, std::size_t{0});
    }

    void testStartStopJoinRecordsCaptureFailures() {
        CameraCapture camera{};
        FrameStore store{};
        FrameIngestor ingestor(camera, store);
        FrameIngestorRunner runner(ingestor, makeFastRuntimeConfig());

        EXPECT_TRUE(runner.start());
        EXPECT_TRUE(waitForAttempts(runner));
        runner.requestStop();
        runner.join();

        const auto status = runner.status();
        EXPECT_FALSE(runner.running());
        EXPECT_FALSE(status.running);
        EXPECT_TRUE(status.stopRequested);
        EXPECT_TRUE(status.ingestAttemptCount > 0);
        EXPECT_EQ(status.submittedFrameCount, std::size_t{0});
        EXPECT_TRUE(status.failedIngestCount > 0);
        EXPECT_TRUE(status.lastCode.has_value());
        if (status.lastCode.has_value()) {
            EXPECT_EQ(status.lastCode.value(), FrameIngestCode::CaptureUnavailable);
        }
    }

    void testStartIsRejectedWhileRunning() {
        CameraCapture camera{};
        FrameStore store{};
        FrameIngestor ingestor(camera, store);
        FrameIngestorRunner runner(ingestor, makeFastRuntimeConfig());

        EXPECT_TRUE(runner.start());
        EXPECT_FALSE(runner.start());
        runner.requestStop();
        runner.join();
    }
} // namespace

int main() {
    testJoinBeforeStartIsNoop();
    testStartStopJoinRecordsCaptureFailures();
    testStartIsRejectedWhileRunning();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
