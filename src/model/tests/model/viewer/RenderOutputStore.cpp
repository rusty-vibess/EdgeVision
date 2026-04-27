#include "model/viewer/RenderOutputStore.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <source_location>
#include <string_view>
#include <vector>

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

    edgevision::model::frame::FrameBuffer makeBuffer(std::uint8_t seed) {
        auto storage = std::make_shared<std::vector<std::byte>>(6);
        for (std::size_t index = 0; index < storage->size(); ++index) {
            (*storage)[index] = static_cast<std::byte>(seed + index);
        }

        const std::shared_ptr<const void> owner(storage, storage.get());
        return edgevision::model::frame::FrameBuffer(storage->data(), storage->size(), owner);
    }

    RenderOutput makeOutput(
        ViewerPoseGeneration poseGeneration,
        edgevision::model::scene::SceneVersionId sceneVersionId
    ) {
        RenderOutput output{};
        output.rgb = makeBuffer(static_cast<std::uint8_t>(poseGeneration));
        output.imageSize = edgevision::model::frame::ImageSize{2, 1};
        output.poseGeneration = poseGeneration;
        output.sceneVersionId = sceneVersionId;
        output.renderTimestamp = std::chrono::steady_clock::now();
        output.cached = false;
        return output;
    }

    void testEmptyStoreHasNoOutputs() {
        RenderOutputStore store{};

        expectTrue(!store.latest().has_value(), "empty store should not expose latest output");
        expectTrue(store.recent(3).empty(), "empty store should not expose output history");
    }

    void testPublishUpdatesLatestOutput() {
        RenderOutputStore store{};
        store.publish(makeOutput(2, 5));

        const std::optional<RenderOutput> latestOutput = store.latest();
        expectTrue(latestOutput.has_value(), "latest should expose the most recent render output");
        expectEq(
            latestOutput->poseGeneration,
            ViewerPoseGeneration{2},
            "latest should preserve pose generation metadata"
        );
        expectEq(
            latestOutput->sceneVersionId,
            edgevision::model::scene::SceneVersionId{5},
            "latest should preserve scene version metadata"
        );
        expectTrue(latestOutput->hasRgb(), "latest should preserve RGB payloads");
    }

    void testBoundedHistoryRetainsRecentOutputs() {
        RenderOutputStore store(2);
        store.publish(makeOutput(1, 10));
        store.publish(makeOutput(2, 20));
        store.publish(makeOutput(3, 30));

        const std::vector<RenderOutput> recentOutputs = store.recent(3);
        expectEq(
            recentOutputs.size(),
            std::size_t{2},
            "bounded history should only retain the configured capacity"
        );
        expectEq(
            recentOutputs[0].poseGeneration,
            ViewerPoseGeneration{2},
            "history should evict the oldest output first"
        );
        expectEq(
            recentOutputs[1].poseGeneration,
            ViewerPoseGeneration{3},
            "history should retain the newest output"
        );
    }
} // namespace

int main() {
    testEmptyStoreHasNoOutputs();
    testPublishUpdatesLatestOutput();
    testBoundedHistoryRetainsRecentOutputs();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
