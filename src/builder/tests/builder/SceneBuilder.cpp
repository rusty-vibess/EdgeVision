#include "builder/state/SceneBuilder.hpp"

#include <iostream>
#include <optional>
#include <source_location>
#include <string_view>

#include "config/types/builder.hpp"
#include "test_frames.hpp"

namespace {
    using namespace edgevision::builder;
    using namespace edgevision::builder::tests;
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

    int usedBlockCount(const InfiniTamScene& scene) {
        auto& index = const_cast<decltype(scene.index)&>(scene.index);
        return index.getNumAllocatedVoxelBlocks() - scene.localVBA.lastFreeBlockId - 1;
    }

    void testBootstrapFrameIntegratesPublishesAndStoresSceneVersion() {
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilder builder(sharedScene, sceneVersionStore);

        const FrameBuildResult result = builder.buildFrame(makeValidFrame(1, 100));
        expectEq(
            result.status,
            FrameBuildStatus::FrameConsumed,
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
            edgevision::model::frame::FrameId{1},
            "scene version should record integrated frame id"
        );

        const SceneReadAccess readAccess = sharedScene.read();
        expectTrue(
            usedBlockCount(readAccess.scene()) > 0,
            "successful build should allocate TSDF scene blocks"
        );
    }

    void testInvalidFrameLayoutFailsBeforePublish() {
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilder builder(sharedScene, sceneVersionStore);

        const FrameBuildResult result = builder.buildFrame(makeInvalidRgbLayoutFrame(1, 100));
        expectEq(result.status, FrameBuildStatus::FrameFailed, "invalid layout should fail");
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
    }

    void testTrackingLossFailsWithoutPublishOrVersionWrite() {
        SharedScene sharedScene{};
        SceneVersionStore sceneVersionStore{};
        SceneBuilder builder(
            sharedScene,
            sceneVersionStore,
            edgevision::config::BuilderRuntimeConfig{.trackerConfig = "type=forcefail"}
        );

        expectEq(
            builder.buildFrame(makeValidFrame(1, 100)).status,
            FrameBuildStatus::FrameConsumed,
            "bootstrap frame should still integrate"
        );

        const FrameBuildResult result = builder.buildFrame(makeValidFrame(2, 200));
        expectEq(result.status, FrameBuildStatus::FrameFailed, "forced tracking loss should fail");
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
    }
} // namespace

int main() {
    testBootstrapFrameIntegratesPublishesAndStoresSceneVersion();
    testInvalidFrameLayoutFailsBeforePublish();
    testTrackingLossFailsWithoutPublishOrVersionWrite();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
