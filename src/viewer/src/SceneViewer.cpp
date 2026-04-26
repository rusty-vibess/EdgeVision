#include "viewer/SceneViewer.hpp"

#include <cmath>
#include <memory>

#include "model/scene/SharedScene.hpp"
#include "model/viewer/RenderOutputStore.hpp"
#include "model/viewer/ViewerPoseStore.hpp"
#include "viewer/state/InfiniTamViewerPipeline.hpp"

namespace edgevision::viewer {
    namespace {
        constexpr int kMaxSaneImageDimension = 16384;

        [[nodiscard]] bool hasValidImageSize(
            const edgevision::model::frame::ImageSize& imageSize
        ) {
            return imageSize.width > 0 && imageSize.height > 0
                && imageSize.width <= kMaxSaneImageDimension
                && imageSize.height <= kMaxSaneImageDimension;
        }

        [[nodiscard]] bool hasValidIntrinsics(
            const edgevision::model::frame::CameraIntrinsics& intrinsics
        ) {
            return std::isfinite(intrinsics.fx) && std::isfinite(intrinsics.fy)
                && std::isfinite(intrinsics.cx) && std::isfinite(intrinsics.cy)
                && intrinsics.fx > 0.0f && intrinsics.fy > 0.0f && intrinsics.cx >= 0.0f
                && intrinsics.cy >= 0.0f;
        }
    } // namespace

    SceneViewer::SceneViewer(
        edgevision::model::viewer::ViewerPoseStore& viewerPoseStore,
        edgevision::model::scene::SharedScene& sharedScene,
        edgevision::model::viewer::RenderOutputStore& renderOutputStore
    )
        : m_viewerPoseStore(viewerPoseStore),
          m_sharedScene(sharedScene),
          m_renderOutputStore(renderOutputStore),
          m_pipeline(std::make_unique<InfiniTamViewerPipeline>()) {}

    SceneViewer::~SceneViewer() = default;

    std::optional<edgevision::model::viewer::RenderOutput> SceneViewer::renderLatestPose() {
        const std::optional<edgevision::model::viewer::ViewerPose> viewerPose =
            m_viewerPoseStore.latest();
        if (!viewerPose.has_value()) {
            return std::nullopt;
        }

        return renderPose(*viewerPose);
    }

    std::optional<edgevision::model::viewer::RenderOutput> SceneViewer::renderPose(
        const edgevision::model::viewer::ViewerPose& viewerPose
    ) {
        if (!hasValidImageSize(viewerPose.imageSize)
            || !hasValidIntrinsics(viewerPose.intrinsics)) {
            return std::nullopt;
        }

        edgevision::model::scene::SceneReadAccess readAccess = m_sharedScene.read();
        edgevision::model::viewer::RenderOutput output =
            m_pipeline->render(readAccess.scene(), readAccess.version(), viewerPose);

        const std::optional<edgevision::model::viewer::RenderOutput> latestOutput =
            m_renderOutputStore.latest();
        output.stale = latestOutput.has_value()
            && latestOutput->poseGeneration == output.poseGeneration
            && latestOutput->sceneVersionId == output.sceneVersionId;

        m_renderOutputStore.publish(output);
        return output;
    }

} // namespace edgevision::viewer
