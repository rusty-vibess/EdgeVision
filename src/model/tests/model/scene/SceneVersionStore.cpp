#include "model/scene/SceneVersionStore.hpp"

#include <array>
#include <iostream>
#include <optional>
#include <source_location>
#include <string_view>

namespace {
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

    Pose4f makePose(float diagonal, float translationX) {
        Pose4f pose{};
        pose.matrix = {
            diagonal,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            diagonal,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            diagonal,
            0.0f,
            translationX,
            0.0f,
            0.0f,
            1.0f,
        };
        return pose;
    }

    SceneVersion makeVersion(
        SceneVersionId versionId,
        edgevision::model::frame::FrameId frameId,
        std::int64_t timestampTicks
    ) {
        SceneVersion version{};
        version.versionId = versionId;
        version.timestamp = edgevision::model::frame::FrameTimestamp{timestampTicks};
        version.lastIntegratedFrameId = frameId;
        version.cameraToWorld =
            makePose(static_cast<float>(versionId), static_cast<float>(frameId));
        version.trackingStatus = TrackingStatus::Tracked;
        version.integrationStatus = IntegrationStatus::Integrated;
        return version;
    }

    void testEmptyStoreHasNoVersions() {
        SceneVersionStore store{};

        expectTrue(!store.latest().has_value(), "empty store should not expose latest version");
        expectTrue(!store.get(1).has_value(), "empty store should not expose lookup hits");
    }

    void testLatestReturnsNewestVersion() {
        SceneVersionStore store{};
        store.add(makeVersion(1, 10, 100));
        store.add(makeVersion(2, 20, 200));

        const std::optional<SceneVersion> latest = store.latest();
        expectTrue(latest.has_value(), "store should expose latest version");
        expectEq(latest->versionId, SceneVersionId{2}, "latest should be newest version");
        expectEq(
            latest->lastIntegratedFrameId,
            edgevision::model::frame::FrameId{20},
            "latest should preserve frame id"
        );
    }

    void testLookupByVersionIdReturnsStoredMetadata() {
        SceneVersionStore store{};
        const SceneVersion version = makeVersion(7, 70, 700);

        store.add(version);

        const std::optional<SceneVersion> lookup = store.get(7);
        expectTrue(lookup.has_value(), "lookup should return stored version");
        expectEq(*lookup, version, "lookup should preserve full scene version metadata");
    }

    void testBoundedRetentionEvictsOldestVersions() {
        SceneVersionStore store(2);
        store.add(makeVersion(1, 10, 100));
        store.add(makeVersion(2, 20, 200));
        store.add(makeVersion(3, 30, 300));

        expectTrue(!store.get(1).has_value(), "bounded store should evict oldest version");
        expectTrue(store.get(2).has_value(), "bounded store should retain recent version");
        expectTrue(store.get(3).has_value(), "bounded store should retain latest version");

        const std::optional<SceneVersion> latest = store.latest();
        expectTrue(latest.has_value(), "bounded store should still expose latest version");
        expectEq(latest->versionId, SceneVersionId{3}, "latest should remain the newest version");
    }
} // namespace

int main() {
    testEmptyStoreHasNoVersions();
    testLatestReturnsNewestVersion();
    testLookupByVersionIdReturnsStoredMetadata();
    testBoundedRetentionEvictsOldestVersions();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
