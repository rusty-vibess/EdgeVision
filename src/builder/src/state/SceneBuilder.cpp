#include "builder/state/SceneBuilder.hpp"

#include <optional>
#include <utility>

namespace edgevision::builder {

    SceneBuilder::SceneBuilder(
        edgevision::model::frame::FrameStore& frameStore,
        edgevision::model::scene::SharedScene& sharedScene,
        edgevision::model::scene::SceneVersionStore& sceneVersionStore,
        const SceneBuilderConfig& config
    )
        : m_frameStore(frameStore),
          m_sharedScene(sharedScene),
          m_sceneVersionStore(sceneVersionStore),
          m_context(config) {}

    void SceneBuilder::run() {
        while (!m_stopRequested.load()) {
            edgevision::model::frame::Frame frame = m_frameStore.waitDequeueReadyFrame();
            [[maybe_unused]] const BuildStepResult result = processFrame(std::move(frame));
        }
    }

    void SceneBuilder::requestStop() {
        m_stopRequested.store(true);
    }

    BuildStepResult SceneBuilder::tryProcessNextFrame() {
        const std::optional<edgevision::model::frame::Frame> frame =
            m_frameStore.tryDequeueReadyFrame();
        if (!frame.has_value()) {
            return BuildStepResult{};
        }

        return processFrame(*frame);
    }

    BuildStepResult SceneBuilder::processFrame(edgevision::model::frame::Frame frame) {
        const ViewConversionResult viewResult = m_context.prepareView(frame);
        if (!viewResult.converted()) {
            [[maybe_unused]] const bool acknowledged = m_frameStore.markFrameFailed(frame.frameId);
            return BuildStepResult{
                BuildStepStatus::FrameFailed,
                BuildFailureReason::ViewConversionFailed,
                frame.frameId,
                std::nullopt,
            };
        }

        edgevision::model::scene::TrackingStatus trackingStatus =
            edgevision::model::scene::TrackingStatus::NotRun;
        edgevision::model::scene::IntegrationStatus integrationStatus =
            edgevision::model::scene::IntegrationStatus::Skipped;
        edgevision::model::scene::SceneVersionId versionId = 0;
        edgevision::model::scene::Pose4f cameraToWorld{};

        {
            edgevision::model::scene::SceneWriteAccess writeAccess = m_sharedScene.write();
            m_context.ensurePipelineForScene(writeAccess.scene(), frame);

            if (m_context.trackingInitialised()) {
                trackingStatus = m_context.track();
                if (trackingStatus == edgevision::model::scene::TrackingStatus::Lost) {
                    [[maybe_unused]] const bool acknowledged =
                        m_frameStore.markFrameFailed(frame.frameId);
                    return BuildStepResult{
                        BuildStepStatus::FrameFailed,
                        BuildFailureReason::TrackingLost,
                        frame.frameId,
                        std::nullopt,
                    };
                }
            }

            integrationStatus = m_context.integrate(writeAccess.scene());
            m_context.prepareForNextTracking(writeAccess.scene());
            cameraToWorld = m_context.cameraToWorld();
            versionId = m_sharedScene.publish(writeAccess);
        }

        edgevision::model::scene::SceneVersion sceneVersion =
            makeSceneVersion(frame, versionId, trackingStatus, integrationStatus);
        sceneVersion.cameraToWorld = cameraToWorld;
        m_sceneVersionStore.add(sceneVersion);
        [[maybe_unused]] const bool acknowledged = m_frameStore.markFrameConsumed(frame.frameId);

        return BuildStepResult{
            BuildStepStatus::FrameConsumed,
            BuildFailureReason::None,
            frame.frameId,
            versionId,
        };
    }

    edgevision::model::scene::SceneVersion SceneBuilder::makeSceneVersion(
        const edgevision::model::frame::Frame& frame,
        edgevision::model::scene::SceneVersionId versionId,
        edgevision::model::scene::TrackingStatus trackingStatus,
        edgevision::model::scene::IntegrationStatus integrationStatus
    ) const {
        edgevision::model::scene::SceneVersion version{};
        version.versionId = versionId;
        version.timestamp = frame.timestamp;
        version.lastIntegratedFrameId = frame.frameId;
        version.cameraToWorld = m_context.cameraToWorld();
        version.trackingStatus = trackingStatus;
        version.integrationStatus = integrationStatus;
        return version;
    }

} // namespace edgevision::builder
