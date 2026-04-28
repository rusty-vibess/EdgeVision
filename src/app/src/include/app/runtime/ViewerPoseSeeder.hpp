#pragma once

#include <chrono>
#include <optional>

#include "config/types/image.hpp"
#include "model/viewer/types/pose.hpp"

namespace edgevision::model::frame {
    class FrameStore;
} // namespace edgevision::model::frame

namespace edgevision::model::scene {
    class SceneVersionStore;
} // namespace edgevision::model::scene

namespace edgevision::model::viewer {
    class ViewerPoseStore;
} // namespace edgevision::model::viewer

namespace edgevision::app::runtime {

    class ViewerPoseSeeder final {
      public:
        ViewerPoseSeeder(
            const model::frame::FrameStore& frameStore,
            const model::scene::SceneVersionStore& sceneVersionStore,
            model::viewer::ViewerPoseStore& viewerPoseStore,
            edgevision::config::ImageSize imageSize
        );

        [[nodiscard]] bool seedOnce(std::chrono::milliseconds timeout);

      private:
        [[nodiscard]] std::optional<model::viewer::ViewerPose> makeInitialViewerPose() const;

        const model::frame::FrameStore& m_frameStore;
        const model::scene::SceneVersionStore& m_sceneVersionStore;
        model::viewer::ViewerPoseStore& m_viewerPoseStore;
        edgevision::config::ImageSize m_imageSize{};
    };

} // namespace edgevision::app::runtime
