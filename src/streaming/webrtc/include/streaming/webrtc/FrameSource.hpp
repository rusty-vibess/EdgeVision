#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <vector>

namespace edgevision::model::frame { class FrameStore; }
namespace edgevision::model::scene { class SharedScene; }

namespace edgevision::streaming::webrtc {

    struct PoseUpdate;

    /// Pulls the latest captured RGB frame from FrameStore, downsamples to
    /// the target size, returns a tightly-packed BGR (24-bit) buffer.
    /// Thread-safe: invoked on the appsrc need-data thread.
    class RgbFrameSource {
      public:
        explicit RgbFrameSource(edgevision::model::frame::FrameStore& store);

        /// Fill `out` with width*height*3 bytes. Returns true on success.
        [[nodiscard]] bool pull(
            std::uint32_t width,
            std::uint32_t height,
            std::vector<std::uint8_t>& out
        );

      private:
        edgevision::model::frame::FrameStore& m_store;
    };

    /// Renders a TSDF raycast at the requested pose. Returns BGR (24-bit).
    /// Thread-safe.
    class TsdfFrameSource {
      public:
        TsdfFrameSource(
            edgevision::model::scene::SharedScene& scene,
            std::function<bool(const PoseUpdate&, std::vector<std::uint8_t>&)> renderer
        );

        /// Replace the renderer at any time (e.g. once George's raycaster is
        /// ready). Safe to call concurrently with render(). Passing nullptr
        /// disables the TSDF track (render() returns false).
        void setRenderer(
            std::function<bool(const PoseUpdate&, std::vector<std::uint8_t>&)> renderer
        );

        [[nodiscard]] bool render(
            const PoseUpdate& pose,
            std::vector<std::uint8_t>& out
        );

      private:
        edgevision::model::scene::SharedScene& m_scene;
        mutable std::mutex m_mu;
        std::function<bool(const PoseUpdate&, std::vector<std::uint8_t>&)> m_renderer;
    };

} // namespace edgevision::streaming::webrtc
