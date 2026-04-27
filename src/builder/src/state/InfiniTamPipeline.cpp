#include "builder/state/InfiniTamPipeline.hpp"

#include <cstring>
#include <memory>

namespace edgevision::builder {
    namespace {
        [[nodiscard]] bool sameIntrinsics(
            const edgevision::model::frame::CameraIntrinsics& lhs,
            const edgevision::model::frame::CameraIntrinsics& rhs
        ) {
            return lhs.fx == rhs.fx && lhs.fy == rhs.fy && lhs.cx == rhs.cx && lhs.cy == rhs.cy;
        }

        [[nodiscard]] bool useGpu(const ITMLib::ITMLibSettings& settings) {
            return settings.GetMemoryType() == MEMORYDEVICE_CUDA;
        }
    } // namespace

    InfiniTamPipeline::InfiniTamPipeline(const edgevision::config::BuilderRuntimeConfig& config)
        : m_config(config) {
        if (!m_config.trackerConfig.empty()) {
            m_trackerConfigStorage = m_config.trackerConfig;
            m_settings.trackerConfig = m_trackerConfigStorage.c_str();
        }
    }

    ViewConversionResult InfiniTamPipeline::prepareView(
        const edgevision::model::frame::Frame& frame
    ) {
        if (needsViewReset(frame)) {
            resetView(frame);
        }

        return copyFrameToView(*m_view, frame, m_settings.GetMemoryType());
    }

    void InfiniTamPipeline::ensurePipelineForScene(
        edgevision::model::scene::InfiniTamScene& scene,
        const edgevision::model::frame::Frame& frame
    ) {
        if (!needsPipelineReset(frame)) {
            return;
        }

        resetPipeline(scene, frame);
    }

    bool InfiniTamPipeline::trackingInitialised() const {
        return m_trackingInitialised;
    }

    edgevision::model::scene::TrackingStatus InfiniTamPipeline::track() {
        m_trackingController->Track(m_trackingState.get(), m_view.get());
        if (m_trackingState->trackerResult == ITMLib::ITMTrackingState::TRACKING_FAILED) {
            return edgevision::model::scene::TrackingStatus::Lost;
        }

        return edgevision::model::scene::TrackingStatus::Tracked;
    }

    edgevision::model::scene::IntegrationStatus InfiniTamPipeline::integrate(
        edgevision::model::scene::InfiniTamScene& scene
    ) {
        m_denseMapper->ProcessFrame(
            m_view.get(), m_trackingState.get(), &scene, m_renderState.get()
        );
        m_trackingInitialised = true;
        return edgevision::model::scene::IntegrationStatus::Integrated;
    }

    void InfiniTamPipeline::prepareForNextTracking(
        const edgevision::model::scene::InfiniTamScene& scene
    ) {
        m_trackingController->Prepare(
            m_trackingState.get(),
            &scene,
            m_view.get(),
            m_visualisationEngine.get(),
            m_renderState.get()
        );
    }

    edgevision::model::scene::Pose4f InfiniTamPipeline::cameraToWorld() const {
        edgevision::model::scene::Pose4f pose{};
        const Matrix4f matrix = m_trackingState->pose_d->GetInvM();
        std::memcpy(pose.matrix.data(), matrix.m, sizeof(matrix.m));
        return pose;
    }

    bool InfiniTamPipeline::needsPipelineReset(
        const edgevision::model::frame::Frame& frame
    ) const {
        return !m_hasPipeline || frame.rgb.size != m_pipelineRgbSize
            || frame.depth.size != m_pipelineDepthSize;
    }

    bool InfiniTamPipeline::needsViewReset(const edgevision::model::frame::Frame& frame) const {
        return !m_hasViewLayout || frame.rgb.size != m_viewRgbSize
            || frame.depth.size != m_viewDepthSize
            || !sameIntrinsics(frame.intrinsics, m_viewIntrinsics);
    }

    void InfiniTamPipeline::resetPipeline(
        edgevision::model::scene::InfiniTamScene& scene,
        const edgevision::model::frame::Frame& frame
    ) {
        const bool shouldResetScene = !m_hasPipeline;
        const Vector2i rgbSize(frame.rgb.size.width, frame.rgb.size.height);
        const Vector2i depthSize(frame.depth.size.width, frame.depth.size.height);

        m_lowLevelEngine.reset(
            ITMLib::ITMLowLevelEngineFactory::MakeLowLevelEngine(m_settings.deviceType)
        );
        m_visualisationEngine.reset(
            ITMLib::ITMVisualisationEngineFactory::
                MakeVisualisationEngine<ITMVoxel, ITMVoxelIndex>(m_settings.deviceType)
        );
        m_denseMapper =
            std::make_unique<ITMLib::ITMDenseMapper<ITMVoxel, ITMVoxelIndex>>(&m_settings);
        if (shouldResetScene) {
            m_denseMapper->ResetScene(&scene);
        }
        m_imuCalibrator = std::make_unique<ITMLib::ITMIMUCalibrator_iPad>();
        m_tracker.reset(
            ITMLib::ITMTrackerFactory::Instance().Make(
                rgbSize,
                depthSize,
                &m_settings,
                m_lowLevelEngine.get(),
                m_imuCalibrator.get(),
                scene.sceneParams
            )
        );
        m_trackingController =
            std::make_unique<ITMLib::ITMTrackingController>(m_tracker.get(), &m_settings);

        const Vector2i trackedImageSize =
            m_trackingController->GetTrackedImageSize(rgbSize, depthSize);
        m_renderState.reset(
            ITMLib::ITMRenderStateFactory<ITMVoxelIndex>::CreateRenderState(
                trackedImageSize, scene.sceneParams, m_settings.GetMemoryType()
            )
        );
        m_trackingState = std::make_unique<ITMLib::ITMTrackingState>(
            trackedImageSize, m_settings.GetMemoryType()
        );
        m_tracker->UpdateInitialPose(m_trackingState.get());

        m_pipelineRgbSize = frame.rgb.size;
        m_pipelineDepthSize = frame.depth.size;
        m_trackingInitialised = false;
        m_hasPipeline = true;
    }

    void InfiniTamPipeline::resetView(const edgevision::model::frame::Frame& frame) {
        const ITMLib::ITMRGBDCalib calib = makeRgbdCalib(frame);
        const Vector2i rgbSize(frame.rgb.size.width, frame.rgb.size.height);
        const Vector2i depthSize(frame.depth.size.width, frame.depth.size.height);

        m_view = std::make_unique<ITMLib::ITMView>(calib, rgbSize, depthSize, useGpu(m_settings));
        m_viewRgbSize = frame.rgb.size;
        m_viewDepthSize = frame.depth.size;
        m_viewIntrinsics = frame.intrinsics;
        m_hasViewLayout = true;
    }

} // namespace edgevision::builder
