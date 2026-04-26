#include "app/runtime/ViewerPoseSeeder.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <optional>
#include <source_location>
#include <string_view>
#include <vector>

#include "model/frame/FrameStore.hpp"
#include "model/scene/SceneVersionStore.hpp"
#include "model/viewer/ViewerPoseStore.hpp"

namespace {
    using edgevision::app::runtime::ViewerPoseSeeder;
    using edgevision::model::frame::CameraIntrinsics;
    using edgevision::model::frame::Frame;
    using edgevision::model::frame::FrameBuffer;
    using edgevision::model::frame::FrameId;
    using edgevision::model::frame::FrameStore;
    using edgevision::model::frame::FrameSubmissionCode;
    using edgevision::model::frame::FrameTimestamp;
    using edgevision::model::frame::ImageSize;
    using edgevision::model::scene::Pose4f;
    using edgevision::model::scene::SceneVersion;
    using edgevision::model::scene::SceneVersionId;
    using edgevision::model::scene::SceneVersionStore;
    using edgevision::model::viewer::ViewerPose;
    using edgevision::model::viewer::ViewerPoseStore;

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

    FrameBuffer makeBuffer(std::size_t byteCount) {
        auto storage = std::make_shared<std::vector<std::byte>>(byteCount);
        return FrameBuffer(
            storage->data(), storage->size(), std::shared_ptr<const void>(storage, storage.get())
        );
    }

    Frame makeFrame(FrameId frameId) {
        Frame frame{};
        frame.frameId = frameId;
        frame.timestamp = FrameTimestamp{static_cast<std::int64_t>(frameId) * 100};
        frame.rgb.size = ImageSize{4, 3};
        frame.rgb.strideBytes = 16;
        frame.rgb.buffer =
            makeBuffer(static_cast<std::size_t>(frame.rgb.strideBytes) * frame.rgb.size.height);
        frame.depth.size = ImageSize{4, 3};
        frame.depth.strideBytes = 8;
        frame.depth.buffer = makeBuffer(
            static_cast<std::size_t>(frame.depth.strideBytes) * frame.depth.size.height
        );
        frame.intrinsics = CameraIntrinsics{525.0f, 525.0f, 2.0f, 1.5f};
        return frame;
    }

    Pose4f makePose(float translationX) {
        Pose4f pose{};
        pose.matrix = {
            1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            translationX,
            0.0f,
            0.0f,
            1.0f,
        };
        return pose;
    }

    SceneVersion makeSceneVersion(SceneVersionId versionId, FrameId frameId, float translationX) {
        SceneVersion version{};
        version.versionId = versionId;
        version.lastIntegratedFrameId = frameId;
        version.cameraToWorld = makePose(translationX);
        return version;
    }

    void testSeedOnceTimesOutWithoutSceneData() {
        FrameStore frameStore{};
        SceneVersionStore sceneVersionStore{};
        ViewerPoseStore viewerPoseStore{};
        ViewerPoseSeeder seeder(frameStore, sceneVersionStore, viewerPoseStore);

        expectTrue(
            !seeder.seedOnce(std::chrono::milliseconds(5)),
            "seedOnce should time out when no scene version is available"
        );
        expectTrue(
            !viewerPoseStore.latest().has_value(),
            "failed seeding should not publish a viewer pose"
        );
    }

    void testSeedOncePublishesPoseFromStoredSceneAndFrame() {
        FrameStore frameStore{};
        const Frame frame = makeFrame(7);
        const auto submission = frameStore.submitFrame(frame);
        expectEq(
            submission.code,
            FrameSubmissionCode::Accepted,
            "test setup should accept the source frame"
        );

        SceneVersionStore sceneVersionStore{};
        const SceneVersion sceneVersion = makeSceneVersion(3, frame.frameId, 4.0f);
        sceneVersionStore.add(sceneVersion);

        ViewerPoseStore viewerPoseStore{};
        ViewerPoseSeeder seeder(frameStore, sceneVersionStore, viewerPoseStore);

        expectTrue(
            seeder.seedOnce(std::chrono::milliseconds(10)),
            "seedOnce should publish the initial pose when frame and scene data exist"
        );

        const std::optional<ViewerPose> latestPose = viewerPoseStore.latest();
        expectTrue(latestPose.has_value(), "seedOnce should publish a viewer pose");
        expectEq(
            latestPose->pose, sceneVersion.cameraToWorld, "seed pose should copy cameraToWorld"
        );
        expectEq(
            latestPose->intrinsics.fx, frame.intrinsics.fx, "seed pose should copy intrinsics"
        );
        expectEq(latestPose->imageSize, frame.rgb.size, "seed pose should copy rgb image size");
    }

    void testSeedOnceDoesNotOverwriteExistingViewerPose() {
        FrameStore frameStore{};
        const Frame frame = makeFrame(9);
        const auto submission = frameStore.submitFrame(frame);
        expectEq(
            submission.code,
            FrameSubmissionCode::Accepted,
            "test setup should accept the source frame"
        );

        SceneVersionStore sceneVersionStore{};
        sceneVersionStore.add(makeSceneVersion(5, frame.frameId, 8.0f));

        ViewerPoseStore viewerPoseStore{};
        ViewerPose existingPose{};
        existingPose.pose = makePose(1.0f);
        existingPose.intrinsics = CameraIntrinsics{600.0f, 600.0f, 3.0f, 2.0f};
        existingPose.imageSize = ImageSize{8, 6};
        existingPose.generation = 7;
        const ViewerPose storedPose = viewerPoseStore.update(existingPose);

        ViewerPoseSeeder seeder(frameStore, sceneVersionStore, viewerPoseStore);
        expectTrue(
            seeder.seedOnce(std::chrono::milliseconds(10)),
            "seedOnce should succeed immediately when a viewer pose already exists"
        );

        const std::optional<ViewerPose> latestPose = viewerPoseStore.latest();
        expectTrue(latestPose.has_value(), "existing viewer pose should remain available");
        expectEq(*latestPose, storedPose, "seedOnce should not overwrite an existing viewer pose");
    }
} // namespace

int main() {
    testSeedOnceTimesOutWithoutSceneData();
    testSeedOncePublishesPoseFromStoredSceneAndFrame();
    testSeedOnceDoesNotOverwriteExistingViewerPose();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
