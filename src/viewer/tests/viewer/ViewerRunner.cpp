#include "viewer/ViewerRunner.hpp"

#include <chrono>
#include <cstddef>
#include <iostream>
#include <optional>
#include <source_location>
#include <string_view>
#include <thread>

#include "config/types/viewer.hpp"
#include "model/scene/SceneVersionStore.hpp"
#include "model/scene/SharedScene.hpp"
#include "model/viewer/RenderOutputStore.hpp"
#include "model/viewer/ViewerPoseStore.hpp"
#include "test_scene.hpp"
#include "viewer/SceneViewer.hpp"

namespace {
    using namespace edgevision::model::scene;
    using namespace edgevision::model::viewer;
    using namespace edgevision::viewer;
    using namespace edgevision::viewer::tests;

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

    struct ViewerFixture {
        SharedScene sharedScene{SceneReadPolicy::Balanced};
        SceneVersionStore sceneVersionStore{};
        ViewerPoseStore viewerPoseStore{};
        RenderOutputStore renderOutputStore{8};
        SceneViewer viewer{viewerPoseStore, sharedScene, renderOutputStore};
    };

    edgevision::config::ViewerRuntimeConfig makePoseDrivenConfig() {
        edgevision::config::ViewerRuntimeConfig config{};
        config.loopPolicy = edgevision::config::ViewerLoopPolicy::PoseDriven;
        config.loopPeriodMs = 2;
        return config;
    }

    edgevision::config::ViewerRuntimeConfig makeLiveLoopConfig() {
        edgevision::config::ViewerRuntimeConfig config{};
        config.loopPolicy = edgevision::config::ViewerLoopPolicy::LiveLoop;
        config.loopPeriodMs = 10;
        return config;
    }

    void testJoinBeforeStartIsNoop() {
        ViewerFixture fixture{};
        ViewerRunner runner(fixture.viewer, fixture.viewerPoseStore, makePoseDrivenConfig());

        runner.join();

        const ViewerRunnerStatus status = runner.status();
        expectTrue(!runner.running(), "runner should not be running before start");
        expectTrue(!status.running, "status should report not running before start");
        expectEq(status.renderAttemptCount, std::size_t{0}, "join before start should be a no-op");
    }

    void testPoseDrivenWaitsForNewerPoseBeforeRendering() {
        ViewerFixture fixture{};
        const std::optional<PopulatedSceneData> data =
            populateScene(fixture.sharedScene, fixture.sceneVersionStore);
        expectTrue(data.has_value(), "scene should populate before runner rendering");
        if (!data.has_value()) {
            return;
        }

        ViewerRunner runner(fixture.viewer, fixture.viewerPoseStore, makePoseDrivenConfig());
        expectTrue(runner.start(), "runner should start");
        expectTrue(
            waitUntil([&runner]() {
                const ViewerRunnerStatus status = runner.status();
                return status.activity == ViewerRunnerActivity::WaitingForPose
                    && status.idleIterationCount > 0;
            }),
            "pose-driven runner should wait when no pose is available"
        );
        expectEq(
            runner.status().publishedOutputCount,
            std::size_t{0},
            "pose-driven runner should not render before a pose arrives"
        );

        const ViewerPose viewerPose = fixture.viewerPoseStore.update(makeViewerPose(*data, 6));
        expectTrue(
            waitUntil([&runner]() { return runner.status().publishedOutputCount == 1; }),
            "pose-driven runner should render after a newer pose arrives"
        );

        runner.requestStop();
        runner.join();

        const ViewerRunnerStatus status = runner.status();
        expectTrue(!runner.running(), "runner should stop after join");
        expectTrue(!status.running, "status should report not running after join");
        expectTrue(status.stopRequested, "status should record stop requests");
        expectEq(
            status.lastPoseGeneration,
            std::optional<ViewerPoseGeneration>{viewerPose.generation},
            "runner status should record the rendered pose generation"
        );
        expectEq(
            status.lastSceneVersionId,
            std::optional<SceneVersionId>{fixture.sharedScene.version()},
            "runner status should record the rendered scene version"
        );
    }

    void testLiveLoopRendersRepeatedlyAndUpdatesActivity() {
        ViewerFixture fixture{};
        const std::optional<PopulatedSceneData> data =
            populateScene(fixture.sharedScene, fixture.sceneVersionStore);
        expectTrue(data.has_value(), "scene should populate before runner rendering");
        if (!data.has_value()) {
            return;
        }

        ViewerRunner runner(fixture.viewer, fixture.viewerPoseStore, makeLiveLoopConfig());
        expectTrue(runner.start(), "runner should start");
        expectTrue(
            waitUntil([&runner]() {
                const ViewerRunnerStatus status = runner.status();
                return status.activity == ViewerRunnerActivity::Idle
                    && status.idleIterationCount > 0;
            }),
            "live-loop runner should report idle activity before any pose is published"
        );

        const ViewerPose viewerPose = fixture.viewerPoseStore.update(makeViewerPose(*data, 2));
        expectTrue(
            waitUntil([&runner]() { return runner.status().publishedOutputCount >= 2; }),
            "live-loop runner should repeatedly render the latest pose"
        );

        runner.requestStop();
        runner.join();

        const ViewerRunnerStatus status = runner.status();
        expectEq(
            status.lastPoseGeneration,
            std::optional<ViewerPoseGeneration>{viewerPose.generation},
            "live-loop runner should record the latest pose generation"
        );
        expectTrue(
            status.activity == ViewerRunnerActivity::RenderedOutput,
            "live-loop runner should report rendered activity after publishing outputs"
        );
        expectTrue(
            status.staleOutputCount > 0,
            "live-loop runner should mark repeated renders of unchanged inputs as stale"
        );
        expectTrue(
            status.lastOutputWasStale,
            "live-loop runner should report stale output status after repeated rendering"
        );
    }
} // namespace

int main() {
    testJoinBeforeStartIsNoop();
    testPoseDrivenWaitsForNewerPoseBeforeRendering();
    testLiveLoopRendersRepeatedlyAndUpdatesActivity();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
