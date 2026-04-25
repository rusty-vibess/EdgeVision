#pragma once

#include <memory>
#include <string>

#include "ITMLib/Core/ITMDenseMapper.h"
#include "ITMLib/Core/ITMTrackingController.h"
#include "ITMLib/Engines/LowLevel/ITMLowLevelEngineFactory.h"
#include "ITMLib/Engines/Visualisation/ITMVisualisationEngineFactory.h"
#include "ITMLib/Objects/Misc/ITMIMUCalibrator.h"
#include "ITMLib/Objects/RenderStates/ITMRenderStateFactory.h"
#include "ITMLib/Trackers/ITMTrackerFactory.h"
#include "ITMLib/Utils/ITMLibSettings.h"
#include "builder/utils/InfiniTamViewConverter.hpp"
#include "config/types/builder.hpp"
#include "model/frame/types/camera.hpp"
#include "model/frame/types/frame.hpp"
#include "model/frame/types/image.hpp"
#include "model/scene/types/infinitam.hpp"
#include "model/scene/types/version.hpp"

namespace edgevision::builder {

    class InfiniTamPipeline final {
      public:
        explicit InfiniTamPipeline(const edgevision::config::BuilderRuntimeConfig& config = {});

        /// Converts frame buffers into the reusable InfiniTAM view.
        [[nodiscard]] ViewConversionResult prepareView(
            const edgevision::model::frame::Frame& frame
        );

        /// Recreates pipeline state when the scene or frame layout changes.
        void ensurePipelineForScene(
            const edgevision::model::scene::InfiniTamScene& scene,
            const edgevision::model::frame::Frame& frame
        );

        /// Returns whether tracking should run before the next integration.
        [[nodiscard]] bool trackingInitialised() const;
        /// Tracks the prepared view against the current scene.
        [[nodiscard]] edgevision::model::scene::TrackingStatus track();
        /// Integrates the prepared view into the current scene.
        [[nodiscard]] edgevision::model::scene::IntegrationStatus integrate(
            edgevision::model::scene::InfiniTamScene& scene
        );
        /// Prepares the pipeline state for the next tracking pass.
        void prepareForNextTracking(const edgevision::model::scene::InfiniTamScene& scene);

        /// Returns the latest camera pose estimated by the pipeline.
        [[nodiscard]] edgevision::model::scene::Pose4f cameraToWorld() const;

      private:
        [[nodiscard]] bool needsPipelineReset(const edgevision::model::frame::Frame& frame) const;
        [[nodiscard]] bool needsViewReset(const edgevision::model::frame::Frame& frame) const;
        void resetPipeline(
            const edgevision::model::scene::InfiniTamScene& scene,
            const edgevision::model::frame::Frame& frame
        );
        void resetView(const edgevision::model::frame::Frame& frame);

        edgevision::config::BuilderRuntimeConfig m_config{};
        ITMLib::ITMLibSettings m_settings{};
        std::string m_trackerConfigStorage{};

        edgevision::model::frame::ImageSize m_viewRgbSize{};
        edgevision::model::frame::ImageSize m_viewDepthSize{};
        edgevision::model::frame::ImageSize m_pipelineRgbSize{};
        edgevision::model::frame::ImageSize m_pipelineDepthSize{};
        edgevision::model::frame::CameraIntrinsics m_viewIntrinsics{};
        bool m_hasViewLayout = false;
        bool m_hasPipeline = false;
        bool m_trackingInitialised = false;

        std::unique_ptr<ITMLib::ITMLowLevelEngine> m_lowLevelEngine{};
        std::unique_ptr<ITMLib::ITMVisualisationEngine<ITMVoxel, ITMVoxelIndex>>
            m_visualisationEngine{};
        std::unique_ptr<ITMLib::ITMDenseMapper<ITMVoxel, ITMVoxelIndex>> m_denseMapper{};
        std::unique_ptr<ITMLib::ITMIMUCalibrator_iPad> m_imuCalibrator{};
        std::unique_ptr<ITMLib::ITMTracker> m_tracker{};
        std::unique_ptr<ITMLib::ITMTrackingController> m_trackingController{};
        std::unique_ptr<ITMLib::ITMRenderState> m_renderState{};
        std::unique_ptr<ITMLib::ITMTrackingState> m_trackingState{};
        std::unique_ptr<ITMLib::ITMView> m_view{};
    };

} // namespace edgevision::builder
