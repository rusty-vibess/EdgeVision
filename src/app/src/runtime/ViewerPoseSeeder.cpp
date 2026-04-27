#include "app/runtime/ViewerPoseSeeder.hpp"

#include <thread>

#include "model/frame/FrameStore.hpp"
#include "model/scene/SceneVersionStore.hpp"
#include "model/viewer/ViewerPoseStore.hpp"

namespace edgevision::app::runtime {
    namespace {
        constexpr auto kPollInterval = std::chrono::milliseconds(10);
    } // namespace

    ViewerPoseSeeder::ViewerPoseSeeder(
        const edgevision::model::frame::FrameStore& frameStore,
        const edgevision::model::scene::SceneVersionStore& sceneVersionStore,
        edgevision::model::viewer::ViewerPoseStore& viewerPoseStore
    )
        : m_frameStore(frameStore),
          m_sceneVersionStore(sceneVersionStore),
          m_viewerPoseStore(viewerPoseStore) {}

    bool ViewerPoseSeeder::seedOnce(std::chrono::milliseconds timeout) {
        if (m_viewerPoseStore.latest().has_value()) {
            return true;
        }

        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            const std::optional<edgevision::model::viewer::ViewerPose> viewerPose =
                makeInitialViewerPose();
            if (viewerPose.has_value()) {
                static_cast<void>(m_viewerPoseStore.update(*viewerPose));
                return true;
            }

            std::this_thread::sleep_for(kPollInterval);
        }

        return false;
    }

    std::optional<edgevision::model::viewer::ViewerPose> ViewerPoseSeeder::
        makeInitialViewerPose() const {
        const std::optional<edgevision::model::scene::SceneVersion> sceneVersion =
            m_sceneVersionStore.latest();
        if (!sceneVersion.has_value()) {
            return std::nullopt;
        }

        const std::optional<edgevision::model::frame::Frame> frame =
            m_frameStore.getFrame(sceneVersion->lastIntegratedFrameId);
        if (!frame.has_value()) {
            return std::nullopt;
        }

        edgevision::model::viewer::ViewerPose viewerPose{};
        viewerPose.pose = sceneVersion->cameraToWorld;
        viewerPose.intrinsics = frame->intrinsics;
        viewerPose.imageSize = frame->rgb.size;
        return viewerPose;
    }

} // namespace edgevision::app::runtime
