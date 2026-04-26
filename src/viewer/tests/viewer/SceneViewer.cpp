#include "viewer/SceneViewer.hpp"

#include <iostream>
#include <optional>
#include <source_location>
#include <string_view>
#include <vector>

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

    void testRenderLatestPosePublishesRgbOutput() {
        SharedScene sharedScene{SceneReadPolicy::Balanced};
        SceneVersionStore sceneVersionStore{};
        const std::optional<PopulatedSceneData> data =
            populateScene(sharedScene, sceneVersionStore);
        expectTrue(data.has_value(), "scene should populate before rendering");
        if (!data.has_value()) {
            return;
        }

        ViewerPoseStore viewerPoseStore{};
        RenderOutputStore renderOutputStore(4);
        SceneViewer viewer(viewerPoseStore, sharedScene, renderOutputStore);

        const ViewerPose viewerPose = viewerPoseStore.update(makeViewerPose(*data, 3));
        const std::optional<RenderOutput> renderOutput = viewer.renderLatestPose();

        expectTrue(
            renderOutput.has_value(), "renderLatestPose should produce output for a stored pose"
        );
        if (!renderOutput.has_value()) {
            return;
        }

        expectTrue(renderOutput->hasRgb(), "render output should contain RGB bytes");
        expectEq(
            renderOutput->imageSize,
            data->frame.rgb.size,
            "render output should preserve image size"
        );
        expectEq(
            renderOutput->rgb.byteCount,
            static_cast<std::size_t>(data->frame.rgb.size.width * data->frame.rgb.size.height * 3),
            "render output should be tightly packed RGB"
        );
        expectEq(
            renderOutput->poseGeneration,
            viewerPose.generation,
            "render output should preserve pose generation"
        );
        expectEq(
            renderOutput->sceneVersionId,
            sharedScene.version(),
            "render output should record the rendered scene version"
        );
        expectTrue(!renderOutput->stale, "first render for a pose and scene should not be stale");

        const std::optional<RenderOutput> latestOutput = renderOutputStore.latest();
        expectTrue(
            latestOutput.has_value(), "render output store should publish the latest render"
        );
        expectEq(
            latestOutput->poseGeneration,
            viewerPose.generation,
            "published output should preserve pose generation metadata"
        );
    }

    void testRepeatedRenderOfUnchangedInputsMarksOutputStale() {
        SharedScene sharedScene{SceneReadPolicy::Balanced};
        SceneVersionStore sceneVersionStore{};
        const std::optional<PopulatedSceneData> data =
            populateScene(sharedScene, sceneVersionStore);
        expectTrue(data.has_value(), "scene should populate before rendering");
        if (!data.has_value()) {
            return;
        }

        ViewerPoseStore viewerPoseStore{};
        RenderOutputStore renderOutputStore(4);
        SceneViewer viewer(viewerPoseStore, sharedScene, renderOutputStore);

        [[maybe_unused]] const ViewerPose viewerPose =
            viewerPoseStore.update(makeViewerPose(*data, 5));
        expectTrue(viewer.renderLatestPose().has_value(), "first render should succeed");

        const std::optional<RenderOutput> repeatedOutput = viewer.renderLatestPose();
        expectTrue(repeatedOutput.has_value(), "repeat render should still produce output");
        expectTrue(
            repeatedOutput.has_value() && repeatedOutput->stale,
            "repeat render with unchanged pose and scene should be marked stale"
        );

        const std::vector<RenderOutput> history = renderOutputStore.recent(4);
        expectEq(history.size(), std::size_t{2}, "render history should retain both outputs");
    }
} // namespace

int main() {
    testRenderLatestPosePublishesRgbOutput();
    testRepeatedRenderOfUnchangedInputsMarksOutputStale();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
