#include "model/viewer/ViewerPoseStore.hpp"

#include <atomic>
#include <chrono>
#include <iostream>
#include <optional>
#include <source_location>
#include <string_view>
#include <thread>

namespace {
    using namespace edgevision::model::viewer;

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

    ViewerPose makeViewerPose(ViewerPoseGeneration generation = 0) {
        ViewerPose viewerPose{};
        viewerPose.pose.matrix = {
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
            0.1f,
            0.0f,
            0.0f,
            1.0f,
        };
        viewerPose.intrinsics =
            edgevision::model::frame::CameraIntrinsics{525.0f, 525.0f, 8.0f, 6.0f};
        viewerPose.imageSize = edgevision::model::frame::ImageSize{16, 12};
        viewerPose.generation = generation;
        return viewerPose;
    }

    edgevision::model::scene::Pose4f makeTranslationPose(float x, float y = 0.0f, float z = 0.0f) {
        edgevision::model::scene::Pose4f pose{};
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
            x,
            y,
            z,
            1.0f,
        };
        return pose;
    }

    ViewerPose makeViewerPoseWithPose(
        const edgevision::model::scene::Pose4f& pose,
        ViewerPoseGeneration generation = 0
    ) {
        ViewerPose viewerPose = makeViewerPose(generation);
        viewerPose.pose = pose;
        return viewerPose;
    }

    void testLatestTracksMostRecentPose() {
        ViewerPoseStore store{};

        const ViewerPose firstPose = store.update(makeViewerPose());
        expectEq(
            firstPose.generation,
            ViewerPoseGeneration{1},
            "first update should assign generation 1"
        );

        const ViewerPose secondPose = store.update(makeViewerPose(7));
        expectEq(
            secondPose.generation,
            ViewerPoseGeneration{7},
            "store should preserve newer caller-provided generations"
        );

        const std::optional<ViewerPose> latestPose = store.latest();
        expectTrue(latestPose.has_value(), "latest should expose the most recent pose");
        expectEq(*latestPose, secondPose, "latest should preserve the latest pose snapshot");
    }

    void testWaitForNewerReturnsAfterUpdate() {
        ViewerPoseStore store{};
        expectEq(
            store.update(makeViewerPose(3)).generation,
            ViewerPoseGeneration{3},
            "seed update should preserve explicit generation"
        );

        std::atomic_bool waiterStarted{false};
        std::optional<ViewerPose> waitedPose{};
        std::thread waiter([&store, &waiterStarted, &waitedPose]() {
            waiterStarted.store(true);
            waitedPose =
                store.waitForNewer(ViewerPoseGeneration{3}, std::chrono::milliseconds(250));
        });

        while (!waiterStarted.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        const ViewerPose updatedPose = store.update(makeViewerPose());
        waiter.join();

        expectTrue(waitedPose.has_value(), "waitForNewer should return once a newer pose arrives");
        expectEq(
            waitedPose->generation,
            updatedPose.generation,
            "waitForNewer should return the newer stored generation"
        );
    }

    void testWaitForNewerTimesOutWhenNoUpdateArrives() {
        ViewerPoseStore store{};
        const ViewerPose latestPose = store.update(makeViewerPose());

        const std::optional<ViewerPose> waitedPose =
            store.waitForNewer(latestPose.generation, std::chrono::milliseconds(5));

        expectTrue(
            !waitedPose.has_value(), "waitForNewer should time out when no newer pose is published"
        );
    }

    void testResetRelativeBaselineRequiresCurrentPose() {
        ViewerPoseStore store{};

        expectTrue(
            !store.resetRelativeBaseline(),
            "resetRelativeBaseline should fail when no pose has been stored"
        );
        expectTrue(
            !store.applyRelativePose(makeTranslationPose(0.2f)).has_value(),
            "applyRelativePose should no-op when no baseline exists"
        );
    }

    void testApplyRelativePosePublishesPoseAgainstBaseline() {
        ViewerPoseStore store{};
        const ViewerPose seededPose =
            store.update(makeViewerPoseWithPose(makeTranslationPose(0.25f)));
        expectTrue(
            store.resetRelativeBaseline(),
            "resetRelativeBaseline should capture the current stored pose"
        );

        const std::optional<ViewerPose> updatedPose =
            store.applyRelativePose(makeTranslationPose(0.5f));

        expectTrue(updatedPose.has_value(), "applyRelativePose should publish a composed pose");
        expectEq(
            updatedPose->pose.matrix[12],
            0.75f,
            "relative translation should compose against the captured baseline"
        );
        expectEq(
            updatedPose->intrinsics.fx,
            seededPose.intrinsics.fx,
            "relative updates should preserve seeded intrinsics"
        );
        expectEq(
            updatedPose->imageSize,
            seededPose.imageSize,
            "relative updates should preserve seeded image size"
        );
        expectTrue(
            updatedPose->generation > seededPose.generation,
            "relative updates should publish a newer generation"
        );
    }

    void testApplyRelativePoseUsesFixedBaseline() {
        ViewerPoseStore store{};
        store.update(makeViewerPoseWithPose(makeTranslationPose(0.25f)));
        expectTrue(
            store.resetRelativeBaseline(),
            "resetRelativeBaseline should capture the current stored pose"
        );

        const std::optional<ViewerPose> firstUpdate =
            store.applyRelativePose(makeTranslationPose(0.5f));
        const std::optional<ViewerPose> secondUpdate =
            store.applyRelativePose(makeTranslationPose(0.5f));

        expectTrue(firstUpdate.has_value(), "first relative pose should publish an update");
        expectTrue(
            !secondUpdate.has_value(),
            "identical relative pose should not accumulate against the latest pose"
        );

        const std::optional<ViewerPose> latestPose = store.latest();
        expectTrue(latestPose.has_value(), "latest should keep the previously published pose");
        expectEq(
            latestPose->pose.matrix[12],
            0.75f,
            "latest pose should remain composed against the captured baseline only once"
        );
    }

    void testResetRelativeBaselineRebasesToCurrentPose() {
        ViewerPoseStore store{};
        store.update(makeViewerPoseWithPose(makeTranslationPose(0.25f)));
        expectTrue(
            store.resetRelativeBaseline(),
            "resetRelativeBaseline should capture the seeded pose"
        );
        expectTrue(
            store.applyRelativePose(makeTranslationPose(0.5f)).has_value(),
            "first relative pose should publish an update"
        );

        expectTrue(
            store.resetRelativeBaseline(),
            "resetRelativeBaseline should capture the current pose when called again"
        );
        const std::optional<ViewerPose> rebasedUpdate =
            store.applyRelativePose(makeTranslationPose(0.5f));

        expectTrue(rebasedUpdate.has_value(), "rebased relative pose should publish an update");
        expectEq(
            rebasedUpdate->pose.matrix[12],
            1.25f,
            "rebased relative pose should compose against the latest captured baseline"
        );
    }
} // namespace

int main() {
    testLatestTracksMostRecentPose();
    testWaitForNewerReturnsAfterUpdate();
    testWaitForNewerTimesOutWhenNoUpdateArrives();
    testResetRelativeBaselineRequiresCurrentPose();
    testApplyRelativePosePublishesPoseAgainstBaseline();
    testApplyRelativePoseUsesFixedBaseline();
    testResetRelativeBaselineRebasesToCurrentPose();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
