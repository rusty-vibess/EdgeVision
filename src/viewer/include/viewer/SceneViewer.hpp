#pragma once

#include <memory>
#include <optional>

#include "model/viewer/types/output.hpp"
#include "model/viewer/types/pose.hpp"

namespace edgevision::model::scene {
    class SharedScene;
} // namespace edgevision::model::scene

namespace edgevision::model::viewer {
    class RenderOutputStore;
    class ViewerPoseStore;
} // namespace edgevision::model::viewer

namespace edgevision::viewer {

    class InfiniTamViewerPipeline;

    /// Renders viewer poses against the latest published shared scene.
    class SceneViewer final {
      public:
        SceneViewer(
            model::viewer::ViewerPoseStore& viewerPoseStore,
            model::scene::SharedScene& sharedScene,
            model::viewer::RenderOutputStore& renderOutputStore
        );
        ~SceneViewer();

        SceneViewer(const SceneViewer&) = delete;
        SceneViewer& operator=(const SceneViewer&) = delete;

        /// Renders the latest stored viewer pose, if one is available.
        [[nodiscard]] std::optional<model::viewer::RenderOutput> renderLatestPose();
        /// Renders `viewerPose` against the current committed shared scene.
        [[nodiscard]] std::optional<model::viewer::RenderOutput> renderPose(
            const model::viewer::ViewerPose& viewerPose
        );

      private:
        model::viewer::ViewerPoseStore& m_viewerPoseStore;
        model::scene::SharedScene& m_sharedScene;
        model::viewer::RenderOutputStore& m_renderOutputStore;
        std::unique_ptr<InfiniTamViewerPipeline> m_pipeline{};
    };

} // namespace edgevision::viewer
