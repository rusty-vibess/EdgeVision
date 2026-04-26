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
} // namespace edgevision::model::viewer

namespace edgevision::viewer {

    class InfiniTamViewerPipeline;

    class SceneViewer final {
      public:
        SceneViewer(
            model::scene::SharedScene& sharedScene,
            model::viewer::RenderOutputStore& renderOutputStore
        );
        ~SceneViewer();

        SceneViewer(const SceneViewer&) = delete;
        SceneViewer& operator=(const SceneViewer&) = delete;

        /// Renders one supplied viewer pose against the latest shared scene state.
        [[nodiscard]] std::optional<model::viewer::RenderOutput> renderPose(
            const model::viewer::ViewerPose& viewerPose
        );

      private:
        model::scene::SharedScene& m_sharedScene;
        model::viewer::RenderOutputStore& m_renderOutputStore;
        std::unique_ptr<InfiniTamViewerPipeline> m_pipeline{};
    };

} // namespace edgevision::viewer
