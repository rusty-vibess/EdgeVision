#include "builder/state/SceneBuilder.hpp"

#include <utility>

namespace edgevision::builder {
    using namespace edgevision::model::scene;

    SceneBuilder::SceneBuilder(
        edgevision::model::scene::SharedScene& sharedScene,
        edgevision::model::scene::SceneVersionStore& sceneVersionStore,
        const edgevision::config::BuilderRuntimeConfig& config
    )
        : m_sharedScene(sharedScene), m_sceneVersionStore(sceneVersionStore), m_pipeline(config) {}

    FrameBuildResult SceneBuilder::buildFrame(edgevision::model::frame::Frame frame) {
        const ViewConversionResult viewResult = m_pipeline.prepareView(frame);
        if (!viewResult.converted()) {
            return FrameBuildResult{
                FrameBuildStatus::FrameFailed,
                BuildFailureReason::ViewConversionFailed,
                frame.frameId,
                std::nullopt,
            };
        }

        TrackingStatus trackingStatus =
            edgevision::model::scene::TrackingStatus::NotRun;
        edgevision::model::scene::IntegrationStatus integrationStatus =
            edgevision::model::scene::IntegrationStatus::Skipped;
        edgevision::model::scene::SceneVersionId versionId = 0;
        edgevision::model::scene::Pose4f cameraToWorld{};

        {
            edgevision::model::scene::SceneWriteAccess writeAccess = m_sharedScene.write();
            m_pipeline.ensurePipelineForScene(writeAccess.scene(), frame);

            if (m_pipeline.trackingInitialised()) {
                trackingStatus = m_pipeline.track();
                if (trackingStatus == edgevision::model::scene::TrackingStatus::Lost) {
                    return FrameBuildResult{
                        FrameBuildStatus::FrameFailed,
                        BuildFailureReason::TrackingLost,
                        frame.frameId,
                        std::nullopt,
                    };
                }
            }

            integrationStatus = m_pipeline.integrate(writeAccess.scene());
            m_pipeline.prepareForNextTracking(writeAccess.scene());
            cameraToWorld = m_pipeline.cameraToWorld();
            versionId = m_sharedScene.publish(writeAccess);
        }

        edgevision::model::scene::SceneVersion sceneVersion =
            makeSceneVersion(frame, versionId, trackingStatus, integrationStatus, cameraToWorld);
        m_sceneVersionStore.add(sceneVersion);

        return FrameBuildResult{
            FrameBuildStatus::FrameConsumed,
            BuildFailureReason::None,
            frame.frameId,
            versionId,
        };
    }

    edgevision::model::scene::SceneVersion SceneBuilder::makeSceneVersion(
        const edgevision::model::frame::Frame& frame,
        edgevision::model::scene::SceneVersionId versionId,
        edgevision::model::scene::TrackingStatus trackingStatus,
        edgevision::model::scene::IntegrationStatus integrationStatus,
        const edgevision::model::scene::Pose4f& cameraToWorld
    ) const {
        edgevision::model::scene::SceneVersion version{};
        version.versionId = versionId;
        version.timestamp = frame.timestamp;
        version.lastIntegratedFrameId = frame.frameId;
        version.cameraToWorld = cameraToWorld;
        version.trackingStatus = trackingStatus;
        version.integrationStatus = integrationStatus;
        return version;
    }

} // namespace edgevision::builder
