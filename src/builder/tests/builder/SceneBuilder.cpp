#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <source_location>
#include <string_view>
#include <utility>
#include <vector>

#include "builder/state/SceneBuilder.hpp"

namespace {
    using namespace edgevision::builder;
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

    FrameBuffer makeBuffer(std::vector<std::byte> bytes) {
        auto storage = std::make_shared<std::vector<std::byte>>(std::move(bytes));
        return FrameBuffer(storage->data(), storage->size(), storage);
    }

    Frame makeValidFrame(FrameId frameId, std::int64_t timestampTicks) {
        constexpr int width = 16;
        constexpr int height = 12;

        std::vector<std::byte> rgbStorage(width * height * 4);
        for (std::size_t index = 0; index < rgbStorage.size(); ++index) {
            rgbStorage[index] = static_cast<std::byte>(index % 255);
        }

        std::vector<std::byte> depthStorage(width * height * 2);
        for (int index = 0; index < width * height; ++index) {
            const std::uint16_t depthMillimetres = 1000;
            std::memcpy(
                depthStorage.data() + static_cast<std::size_t>(index) * sizeof(std::uint16_t),
                &depthMillimetres,
                sizeof(depthMillimetres)
            );
        }

        Frame frame{};
        frame.frameId = frameId;
        frame.timestamp = FrameTimestamp{timestampTicks};
        frame.rgb.size = ImageSize{width, height};
        frame.rgb.strideBytes = width * 4;
        frame.rgb.buffer = makeBuffer(std::move(rgbStorage));
        frame.depth.size = ImageSize{width, height};
        frame.depth.strideBytes = width * 2;
        frame.depth.buffer = makeBuffer(std::move(depthStorage));
        frame.intrinsics = CameraIntrinsics{525.0f, 525.0f, width / 2.0f, height / 2.0f};
        return frame;
    }

    Frame makeInvalidRgbLayoutFrame(FrameId frameId, std::int64_t timestampTicks) {
        Frame frame = makeValidFrame(frameId, timestampTicks);
        frame.rgb.size = ImageSize{2, 2};
        frame.rgb.strideBytes = 6;
        frame.rgb.buffer = makeBuffer(std::vector<std::byte>(12));
        frame.depth.size = ImageSize{2, 2};
        frame.depth.strideBytes = 4;
        frame.depth.buffer = makeBuffer(std::vector<std::byte>(8));
        frame.intrinsics = CameraIntrinsics{525.0f, 525.0f, 1.0f, 1.0f};
        return frame;
    }

    void testNoPendingFrameReturnsNoOp() {
        FrameStore frameStore{};
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilder builder(frameStore, sharedScene, sceneVersionStore);

        const BuildStepResult result = builder.tryProcessNextFrame();
        expectEq(result.status, BuildStepStatus::NoFrameAvailable, "empty store should no-op");
        expectTrue(!result.sceneVersionId.has_value(), "no-op should not publish a version");
    }

    void testBootstrapSuccessPublishesSceneVersionAndConsumesFrame() {
        FrameStore frameStore{};
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilder builder(frameStore, sharedScene, sceneVersionStore);

        expectTrue(
            frameStore.submitFrame(makeValidFrame(1, 100)).accepted(), "frame should submit"
        );

        const BuildStepResult result = builder.tryProcessNextFrame();
        expectEq(
            result.status,
            BuildStepStatus::FrameConsumed,
            "bootstrap frame should be integrated and consumed"
        );
        expectTrue(result.sceneVersionId.has_value(), "success should publish a scene version");
        expectEq(sharedScene.version(), SceneVersionId{1}, "scene publish should advance version");

        const std::optional<SceneVersion> latestVersion = sceneVersionStore.latest();
        expectTrue(latestVersion.has_value(), "success should write scene version metadata");
        expectEq(
            latestVersion->trackingStatus,
            TrackingStatus::NotRun,
            "bootstrap integration should record tracking not run"
        );
        expectEq(
            latestVersion->integrationStatus,
            IntegrationStatus::Integrated,
            "bootstrap integration should record integrated status"
        );
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

    void testConversionFailureMarksFrameFailedWithoutPublishingVersion() {
        FrameStore frameStore{};
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilder builder(frameStore, sharedScene, sceneVersionStore);

        expectTrue(
            frameStore.submitFrame(makeInvalidRgbLayoutFrame(1, 100)).accepted(),
            "frame should submit despite builder-invalid layout"
        );

        const BuildStepResult result = builder.tryProcessNextFrame();
        expectEq(result.status, BuildStepStatus::FrameFailed, "invalid layout should fail frame");
        expectEq(
            result.failureReason,
            BuildFailureReason::ViewConversionFailed,
            "invalid layout should fail during view conversion"
        );
        expectEq(sharedScene.version(), SceneVersionId{0}, "failed conversion should not publish");
        expectTrue(
            !sceneVersionStore.latest().has_value(),
            "failed conversion should not write scene version metadata"
        );

        const std::optional<FrameLifecycle> lifecycle = frameStore.getFrameLifecycle(1);
        expectTrue(lifecycle.has_value(), "failed frame should retain lifecycle");
        expectEq(lifecycle->state, FrameLifecycleState::Failed, "frame should be marked failed");
    }

    void testTrackingFailureMarksFrameFailedAndDoesNotPublishVersion() {
        FrameStore frameStore{};
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilder builder(
            frameStore,
            sharedScene,
            sceneVersionStore,
            SceneBuilderConfig{.trackerConfig = "type=forcefail"}
        );

        expectTrue(
            frameStore.submitFrame(makeValidFrame(1, 100)).accepted(), "frame 1 should submit"
        );
        expectEq(
            builder.tryProcessNextFrame().status,
            BuildStepStatus::FrameConsumed,
            "bootstrap frame should still integrate"
        );

        expectTrue(
            frameStore.submitFrame(makeValidFrame(2, 200)).accepted(), "frame 2 should submit"
        );
        const BuildStepResult result = builder.tryProcessNextFrame();
        expectEq(result.status, BuildStepStatus::FrameFailed, "forced tracking loss should fail");
        expectEq(
            result.failureReason,
            BuildFailureReason::TrackingLost,
            "forcefail tracker should report tracking loss"
        );
        expectEq(
            sharedScene.version(),
            SceneVersionId{1},
            "tracking failure should not publish a new scene version"
        );

        const std::optional<SceneVersion> latestVersion = sceneVersionStore.latest();
        expectTrue(latestVersion.has_value(), "bootstrap scene version should remain available");
        expectEq(
            latestVersion->versionId,
            SceneVersionId{1},
            "tracking failure should not replace latest scene version"
        );

        const std::optional<FrameLifecycle> lifecycle = frameStore.getFrameLifecycle(2);
        expectTrue(lifecycle.has_value(), "failed tracked frame should retain lifecycle");
        expectEq(
            lifecycle->state, FrameLifecycleState::Failed, "tracked failure should mark failed"
        );
    }
} // namespace

int main() {
    testNoPendingFrameReturnsNoOp();
    testBootstrapSuccessPublishesSceneVersionAndConsumesFrame();
    testConversionFailureMarksFrameFailedWithoutPublishingVersion();
    testTrackingFailureMarksFrameFailedAndDoesNotPublishVersion();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
