#pragma once

#include <chrono>

#include "model/frame/types/image.hpp"
#include "model/scene/types/access.hpp"
#include "model/viewer/types/identity.hpp"

namespace edgevision::model::viewer {

    /// Stores the result of rendering a viewer pose against a published scene.
    struct RenderOutput {
        /// Tightly packed RGB byte buffer in row-major order.
        edgevision::model::frame::FrameBuffer rgb{};
        /// Dimensions of the RGB render.
        edgevision::model::frame::ImageSize imageSize{};
        /// Viewer pose generation used for this render.
        ViewerPoseGeneration poseGeneration = 0;
        /// Published scene version used for this render.
        edgevision::model::scene::SceneVersionId sceneVersionId = 0;
        /// Local render completion timestamp.
        std::chrono::steady_clock::time_point renderTimestamp{};
        /// Whether this output repeated the latest pose+scene combination already published.
        bool stale = false;

        /// Returns whether this output contains an RGB payload.
        [[nodiscard]] bool hasRgb() const {
            return !rgb.empty();
        }
    };

} // namespace edgevision::model::viewer
