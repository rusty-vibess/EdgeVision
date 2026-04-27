#pragma once

#include <memory>

#include "ITMLib/Engines/Visualisation/ITMVisualisationEngineFactory.h"
#include "ITMLib/Objects/Camera/ITMIntrinsics.h"
#include "ITMLib/Objects/RenderStates/ITMRenderStateFactory.h"
#include "ITMLib/Utils/ITMImageTypes.h"
#include "ITMLib/Utils/ITMLibSettings.h"
#include "ORUtils/SE3Pose.h"
#include "model/scene/types/infinitam.hpp"
#include "model/viewer/types/output.hpp"
#include "model/viewer/types/pose.hpp"

namespace edgevision::viewer {

    class InfiniTamViewerPipeline final {
      public:
        InfiniTamViewerPipeline();

        [[nodiscard]] edgevision::model::viewer::RenderOutput render(
            const edgevision::model::scene::InfiniTamScene& scene,
            edgevision::model::scene::SceneVersionId sceneVersionId,
            const edgevision::model::viewer::ViewerPose& viewerPose
        );

      private:
        [[nodiscard]] bool needsLayoutReset(
            const edgevision::model::viewer::ViewerPose& viewerPose
        ) const;
        void resetLayout(
            const edgevision::model::scene::InfiniTamScene& scene,
            const edgevision::model::viewer::ViewerPose& viewerPose
        );
        [[nodiscard]] ITMLib::ITMIntrinsics makeIntrinsics(
            const edgevision::model::viewer::ViewerPose& viewerPose
        ) const;
        [[nodiscard]] ORUtils::SE3Pose makePose(
            const edgevision::model::viewer::ViewerPose& viewerPose
        ) const;
        void copyRenderToHost();
        [[nodiscard]] edgevision::model::frame::FrameBuffer makeRgbBuffer() const;

        ITMLib::ITMLibSettings m_settings{};
        edgevision::model::frame::ImageSize m_renderImageSize{};
        bool m_hasLayout = false;

        std::unique_ptr<ITMLib::ITMVisualisationEngine<ITMVoxel, ITMVoxelIndex>>
            m_visualisationEngine{};
        std::unique_ptr<ITMLib::ITMRenderState> m_renderState{};
        std::unique_ptr<ITMUChar4Image> m_outputImage{};
    };

} // namespace edgevision::viewer
