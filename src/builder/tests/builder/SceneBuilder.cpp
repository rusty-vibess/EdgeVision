#include "builder/state/SceneBuilder.hpp"

#include <cstring>
#include <cuda_runtime.h>
#include <iostream>
#include <optional>
#include <source_location>
#include <string_view>
#include <vector>

#include "ITMLib/Objects/Scene/ITMVoxelBlockHash.h"
#include "builder/utils/InfiniTamViewConverter.hpp"
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

    bool copyHashEntriesToHost(
        const InfiniTamScene& scene,
        std::vector<ITMHashEntry>& hostEntries
    ) {
        const std::size_t entryCount = static_cast<std::size_t>(scene.index.noTotalEntries);
        hostEntries.resize(entryCount);

        return cudaMemcpy(
                   hostEntries.data(),
                   scene.index.GetEntries(),
                   entryCount * sizeof(ITMHashEntry),
                   cudaMemcpyDeviceToHost
               )
            == cudaSuccess;
    }

    bool hashSceneLooksValid(const InfiniTamScene& scene) {
        std::vector<ITMHashEntry> hostEntries{};
        if (!copyHashEntriesToHost(scene, hostEntries)) {
            return false;
        }

        std::size_t allocatedEntryCount = 0;
        for (const ITMHashEntry& entry : hostEntries) {
            if (entry.ptr < 0) {
                continue;
            }

            if (entry.ptr >= SDF_LOCAL_BLOCK_NUM) {
                return false;
            }

            ++allocatedEntryCount;
            if (allocatedEntryCount > static_cast<std::size_t>(SDF_LOCAL_BLOCK_NUM)) {
                return false;
            }
        }

        return true;
    }

    edgevision::model::frame::Frame makeSwizzleTestFrame() {
        using namespace edgevision::model::frame;

        constexpr int width = 17;
        constexpr int height = 1;

        std::vector<std::byte> rgbStorage(static_cast<std::size_t>(width) * 4);
        for (int column = 0; column < width; ++column) {
            const std::size_t offset = static_cast<std::size_t>(column) * 4;
            rgbStorage[offset] = static_cast<std::byte>(10 + column);
            rgbStorage[offset + 1] = static_cast<std::byte>(40 + column);
            rgbStorage[offset + 2] = static_cast<std::byte>(70 + column);
            rgbStorage[offset + 3] = static_cast<std::byte>(column);
        }

        std::vector<std::byte> depthStorage(static_cast<std::size_t>(width) * 2);
        for (int column = 0; column < width; ++column) {
            const std::uint16_t depthMillimetres = 1000;
            std::memcpy(
                depthStorage.data() + static_cast<std::size_t>(column) * sizeof(std::uint16_t),
                &depthMillimetres,
                sizeof(depthMillimetres)
            );
        }

        Frame frame{};
        frame.frameId = 7;
        frame.timestamp = FrameTimestamp{700};
        frame.rgb.size = ImageSize{width, height};
        frame.rgb.strideBytes = width * 4;
        frame.rgb.buffer = makeBuffer(std::move(rgbStorage));
        frame.depth.size = ImageSize{width, height};
        frame.depth.strideBytes = width * 2;
        frame.depth.buffer = makeBuffer(std::move(depthStorage));
        frame.intrinsics = CameraIntrinsics{525.0f, 525.0f, width / 2.0f, 0.5f};
        return frame;
    }

    void testCopyFrameToViewSwizzlesBgraToOpaqueRgba() {
        using namespace edgevision::model::frame;

        const Frame frame = makeSwizzleTestFrame();
        ITMLib::ITMView view(
            makeRgbdCalib(frame),
            Vector2i(frame.rgb.size.width, frame.rgb.size.height),
            Vector2i(frame.depth.size.width, frame.depth.size.height),
            false
        );

        const ViewConversionResult result = copyFrameToView(view, frame, MEMORYDEVICE_CPU);
        expectTrue(result.converted(), "valid test frame should convert into an ITM view");
        if (!result.converted()) {
            return;
        }

        const Vector4u* rgbData = view.rgb->GetData(MEMORYDEVICE_CPU);
        for (int column = 0; column < frame.rgb.size.width; ++column) {
            expectEq(
                rgbData[column].r,
                static_cast<unsigned char>(70 + column),
                "converted RGB image should place red in the first channel"
            );
            expectEq(
                rgbData[column].g,
                static_cast<unsigned char>(40 + column),
                "converted RGB image should preserve green in the second channel"
            );
            expectEq(
                rgbData[column].b,
                static_cast<unsigned char>(10 + column),
                "converted RGB image should place blue in the third channel"
            );
            expectEq(
                rgbData[column].a,
                static_cast<unsigned char>(255),
                "converted RGB image should force alpha opaque"
            );
        }
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
        expectTrue(
            hashSceneLooksValid(readAccess.scene()),
            "successful build should leave a structurally valid hash scene"
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
    testCopyFrameToViewSwizzlesBgraToOpaqueRgba();
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
