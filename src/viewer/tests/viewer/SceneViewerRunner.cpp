#include "viewer/SceneViewerRunner.hpp"

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
        SharedScene sharedScene{edgevision::config::SceneReadPolicy::Balanced};
        SceneVersionStore sceneVersionStore{};
        ViewerPoseStore viewerPoseStore{};
        RenderOutputStore renderOutputStore{8};
    };

    edgevision::config::ViewerRuntimeConfig makeEventConfig() {
        edgevision::config::ViewerRuntimeConfig config{};
        config.loopPolicy = edgevision::config::ViewerLoopPolicy::Event;
        config.loopPeriodMs = 2;
        return config;
    }

    edgevision::config::ViewerRuntimeConfig makeHotLoopConfig() {
        edgevision::config::ViewerRuntimeConfig config{};
        config.loopPolicy = edgevision::config::ViewerLoopPolicy::HotLoop;
        config.loopPeriodMs = 10;
        return config;
    }

    void testJoinBeforeStartIsNoop() {
        ViewerFixture fixture{};
        SceneViewerRunner runner(
            fixture.viewerPoseStore,
            fixture.sharedScene,
            fixture.renderOutputStore,
            makeEventConfig()
        );

        runner.join();

        const SceneViewerRunnerStatus status = runner.status();
        expectTrue(!runner.running(), "runner should not be running before start");
        expectTrue(!status.running, "status should report not running before start");
        expectEq(status.renderAttemptCount, std::size_t{0}, "join before start should be a no-op");
    }

    void testEventWaitsForNewerPoseBeforeRendering() {
        ViewerFixture fixture{};
        const std::optional<PopulatedSceneData> data =
            populateScene(fixture.sharedScene, fixture.sceneVersionStore);
        expectTrue(data.has_value(), "scene should populate before runner rendering");
        if (!data.has_value()) {
            return;
        }

        SceneViewerRunner runner(
            fixture.viewerPoseStore,
            fixture.sharedScene,
            fixture.renderOutputStore,
            makeEventConfig()
        );
        expectTrue(runner.start(), "runner should start");
        expectTrue(
            waitUntil([&runner]() {
                const SceneViewerRunnerStatus status = runner.status();
                return status.activity == SceneViewerRunnerActivity::WaitingForPose
                    && status.idleIterationCount > 0;
            }),
            "event runner should wait when no pose is available"
        );
        expectEq(
            runner.status().publishedOutputCount,
            std::size_t{0},
            "event runner should not render before a pose arrives"
        );

        const ViewerPose viewerPose = fixture.viewerPoseStore.update(makeViewerPose(*data, 6));
        expectTrue(
            waitUntil([&runner]() { return runner.status().publishedOutputCount == 1; }),
            "event runner should render after a newer pose arrives"
        );

        runner.requestStop();
        runner.join();

        const SceneViewerRunnerStatus status = runner.status();
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

    void testHotLoopRendersRepeatedlyAndUpdatesActivity() {
        ViewerFixture fixture{};
        const std::optional<PopulatedSceneData> data =
            populateScene(fixture.sharedScene, fixture.sceneVersionStore);
        expectTrue(data.has_value(), "scene should populate before runner rendering");
        if (!data.has_value()) {
            return;
        }

        SceneViewerRunner runner(
            fixture.viewerPoseStore,
            fixture.sharedScene,
            fixture.renderOutputStore,
            makeHotLoopConfig()
        );
        expectTrue(runner.start(), "runner should start");
        expectTrue(
            waitUntil([&runner]() {
                const SceneViewerRunnerStatus status = runner.status();
                return status.activity == SceneViewerRunnerActivity::Idle
                    && status.idleIterationCount > 0;
            }),
            "hot-loop runner should report idle activity before any pose is published"
        );

        const ViewerPose viewerPose = fixture.viewerPoseStore.update(makeViewerPose(*data, 2));
        expectTrue(
            waitUntil([&runner]() { return runner.status().publishedOutputCount >= 2; }),
            "hot-loop runner should repeatedly render the latest pose"
        );

        runner.requestStop();
        runner.join();

        const SceneViewerRunnerStatus status = runner.status();
        expectEq(
            status.lastPoseGeneration,
            std::optional<ViewerPoseGeneration>{viewerPose.generation},
            "hot-loop runner should record the latest pose generation"
        );
        expectTrue(
            status.activity == SceneViewerRunnerActivity::RenderedOutput,
            "hot-loop runner should report rendered activity after publishing outputs"
        );
        expectTrue(
            status.cachedOutputCount > 0,
            "hot-loop runner should mark repeated renders of unchanged inputs as cached"
        );
        expectTrue(
            status.lastOutputWasCached,
            "hot-loop runner should report cached output status after repeated rendering"
        );
    }
} // namespace

int main() {
    testJoinBeforeStartIsNoop();
    testEventWaitsForNewerPoseBeforeRendering();
    testHotLoopRendersRepeatedlyAndUpdatesActivity();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
