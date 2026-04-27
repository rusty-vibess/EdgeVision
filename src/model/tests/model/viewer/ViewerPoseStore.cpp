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
} // namespace

int main() {
    testLatestTracksMostRecentPose();
    testWaitForNewerReturnsAfterUpdate();
    testWaitForNewerTimesOutWhenNoUpdateArrives();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
