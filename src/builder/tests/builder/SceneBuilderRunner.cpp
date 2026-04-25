#include "builder/SceneBuilderRunner.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <optional>
#include <source_location>
#include <string_view>
#include <thread>

#include "config/types/builder.hpp"
#include "model/frame/FrameStore.hpp"
#include "model/scene/SceneVersionStore.hpp"
#include "model/scene/SharedScene.hpp"
#include "test_frames.hpp"

namespace {
    using namespace edgevision::builder;
    using namespace edgevision::builder::tests;
    using namespace edgevision::model::frame;
    using namespace edgevision::model::scene;

    int gFailures = 0;

    void recordFailure(
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        std::cerr << location.file_name() << ':' << location.line() << ": " << message << '\n';
        ++gFailures;
    }

    void expectTrue(
        bool value,
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        if (!value) {
            recordFailure(message, location);
        }
    }

    template <typename T, typename U>
    void expectEq(
        const T& lhs,
        const U& rhs,
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        if (!(lhs == rhs)) {
            recordFailure(message, location);
        }
    }

    edgevision::config::BuilderRuntimeConfig makeFastRuntimeConfig() {
        edgevision::config::BuilderRuntimeConfig config{};
        config.readyFrameTimeoutMs = 1;
        return config;
    }

    template <typename Predicate>
    bool waitUntil(
        Predicate predicate,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(500)
    ) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (predicate()) {
                return true;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        return predicate();
    }

    void testJoinBeforeStartIsNoop() {
        FrameStore frameStore{};
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilderRunner runner(
            frameStore, sharedScene, sceneVersionStore, makeFastRuntimeConfig()
        );

        runner.join();

        const SceneBuilderRunnerStatus status = runner.status();
        expectTrue(!runner.running(), "runner should not be running before start");
        expectTrue(!status.running, "status should report not running");
        expectEq(status.buildAttemptCount, std::size_t{0}, "join before start should be a no-op");
    }

    void testStartStopJoinWithoutFramesDoesNotRecordBuilds() {
        FrameStore frameStore{};
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilderRunner runner(
            frameStore, sharedScene, sceneVersionStore, makeFastRuntimeConfig()
        );

        expectTrue(runner.start(), "runner should start");
        expectTrue(runner.running(), "runner should report running after start");
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        runner.requestStop();
        runner.join();

        const SceneBuilderRunnerStatus status = runner.status();
        expectTrue(!runner.running(), "runner should stop after join");
        expectTrue(!status.running, "status should report not running after join");
        expectTrue(status.stopRequested, "status should record stop requested");
        expectEq(
            status.buildAttemptCount, std::size_t{0}, "idle timeouts should not count as builds"
        );
        expectEq(
            status.consumedFrameCount, std::size_t{0}, "idle runner should not consume frames"
        );
        expectEq(status.failedFrameCount, std::size_t{0}, "idle runner should not fail frames");
    }

    void testStartIsRejectedWhileRunning() {
        FrameStore frameStore{};
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilderRunner runner(
            frameStore, sharedScene, sceneVersionStore, makeFastRuntimeConfig()
        );

        expectTrue(runner.start(), "runner should start");
        expectTrue(!runner.start(), "runner should reject a second start while running");
        runner.requestStop();
        runner.join();
    }

    void testSuccessMarksFrameConsumedAndUpdatesStatus() {
        FrameStore frameStore{};
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilderRunner runner(
            frameStore, sharedScene, sceneVersionStore, makeFastRuntimeConfig()
        );

        expectTrue(runner.start(), "runner should start");
        expectTrue(
            frameStore.submitFrame(makeValidFrame(1, 100)).accepted(), "frame should submit"
        );
        expectTrue(
            waitUntil([&runner]() { return runner.status().consumedFrameCount == 1; }),
            "runner should consume the submitted frame"
        );
        runner.requestStop();
        runner.join();

        const SceneBuilderRunnerStatus status = runner.status();
        expectEq(
            status.buildAttemptCount, std::size_t{1}, "successful build should count one attempt"
        );
        expectEq(
            status.consumedFrameCount, std::size_t{1}, "successful build should count one consume"
        );
        expectEq(
            status.failedFrameCount, std::size_t{0}, "successful build should not count failures"
        );
        expectEq(status.lastFrameId, FrameId{1}, "status should record the last frame id");
        expectTrue(
            status.lastSceneVersionId.has_value(),
            "status should record the published scene version"
        );
        if (status.lastSceneVersionId.has_value()) {
            expectEq(
                *status.lastSceneVersionId,
                SceneVersionId{1},
                "status should record the published scene version id"
            );
        }
        expectTrue(
            !status.lastFailureReason.has_value(), "successful build should clear failure reason"
        );
        expectEq(sharedScene.version(), SceneVersionId{1}, "scene publish should advance version");

        const std::optional<SceneVersion> latestVersion = sceneVersionStore.latest();
        expectTrue(latestVersion.has_value(), "success should write scene version metadata");
        expectEq(
            latestVersion->lastIntegratedFrameId,
            FrameId{1},
            "scene version should record integrated frame id"
        );

        const std::optional<FrameLifecycle> lifecycle = frameStore.getFrameLifecycle(1);
        expectTrue(lifecycle.has_value(), "consumed frame should retain lifecycle");
        expectEq(
            lifecycle->state, FrameLifecycleState::Consumed, "frame should be marked consumed"
        );
    }

    void testFailureMarksFrameFailedAndUpdatesStatus() {
        FrameStore frameStore{};
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilderRunner runner(
            frameStore, sharedScene, sceneVersionStore, makeFastRuntimeConfig()
        );

        expectTrue(runner.start(), "runner should start");
        expectTrue(
            frameStore.submitFrame(makeInvalidRgbLayoutFrame(1, 100)).accepted(),
            "frame should submit despite builder-invalid layout"
        );
        expectTrue(
            waitUntil([&runner]() { return runner.status().failedFrameCount == 1; }),
            "runner should fail the submitted frame"
        );
        runner.requestStop();
        runner.join();

        const SceneBuilderRunnerStatus status = runner.status();
        expectEq(
            status.buildAttemptCount, std::size_t{1}, "failed build should count one attempt"
        );
        expectEq(
            status.consumedFrameCount, std::size_t{0}, "failed build should not count consumes"
        );
        expectEq(status.failedFrameCount, std::size_t{1}, "failed build should count one failure");
        expectEq(status.lastFrameId, FrameId{1}, "status should record the last frame id");
        expectTrue(
            !status.lastSceneVersionId.has_value(),
            "failed build should not record a scene version"
        );
        expectTrue(
            status.lastFailureReason.has_value(), "failed build should record a failure reason"
        );
        if (status.lastFailureReason.has_value()) {
            expectEq(
                *status.lastFailureReason,
                BuildFailureReason::ViewConversionFailed,
                "status should record the failure reason"
            );
        }
        expectEq(sharedScene.version(), SceneVersionId{0}, "failed conversion should not publish");
        expectTrue(
            !sceneVersionStore.latest().has_value(),
            "failed conversion should not write scene version metadata"
        );

        const std::optional<FrameLifecycle> lifecycle = frameStore.getFrameLifecycle(1);
        expectTrue(lifecycle.has_value(), "failed frame should retain lifecycle");
        expectEq(lifecycle->state, FrameLifecycleState::Failed, "frame should be marked failed");
    }
} // namespace

int main() {
    testJoinBeforeStartIsNoop();
    testStartStopJoinWithoutFramesDoesNotRecordBuilds();
    testStartIsRejectedWhileRunning();
    testSuccessMarksFrameConsumedAndUpdatesStatus();
    testFailureMarksFrameFailedAndUpdatesStatus();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
