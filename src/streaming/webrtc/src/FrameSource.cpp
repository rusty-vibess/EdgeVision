#include "streaming/webrtc/FrameSource.hpp"

#include "model/frame/FrameStore.hpp"
#include "model/frame/types/frame.hpp"
#include "model/frame/types/image.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <utility>

// `SharedScene` is referenced only via reference (`m_scene`). The full
// definition is expected to land in `model/scene/SharedScene.hpp` on a
// follow-up branch; for the streaming_webrtc translation unit a forward
// declaration in the header is sufficient because we never call into it.

namespace edgevision::streaming::webrtc {

    RgbFrameSource::RgbFrameSource(edgevision::model::frame::FrameStore& store)
        : m_store(store) {}

    bool RgbFrameSource::pull(
        std::uint32_t width, std::uint32_t height, std::vector<std::uint8_t>& out
    ) {
        out.assign(static_cast<std::size_t>(width) * height * 3, 0);

        // FrameStore on origin/main exposes the latest frame via
        // `getLastFrame() -> std::optional<Frame>`. The Frame carries
        // BGRA32 RGB data (K4A native format) in `frame.rgb.buffer`.
        const auto maybeFrame = m_store.getLastFrame();
        if (!maybeFrame.has_value()) {
            return false;
        }
        const auto& frame = *maybeFrame;
        const auto sw = static_cast<std::uint32_t>(frame.rgb.size.width);
        const auto sh = static_cast<std::uint32_t>(frame.rgb.size.height);
        if (sw == 0 || sh == 0) {
            return false;
        }
        if (frame.rgb.buffer.empty() || frame.rgb.buffer.data == nullptr) {
            return false;
        }
        const auto* src = reinterpret_cast<const std::uint8_t*>(
            frame.rgb.buffer.data
        );
        // K4A delivers BGRA32 (4 bytes per pixel). Honour `strideBytes` if
        // the producer set it; otherwise fall back to packed width * 4.
        const std::size_t srcStride = frame.rgb.strideBytes > 0
            ? static_cast<std::size_t>(frame.rgb.strideBytes)
            : static_cast<std::size_t>(sw) * 4;
        constexpr std::size_t kSrcBytesPerPixel = 4;

        // Naive nearest-neighbour downsample to target size; replace with
        // libyuv or NPP if perf becomes a bottleneck.
        for (std::uint32_t y = 0; y < height; ++y) {
            const auto srcY = std::min<std::uint32_t>(y * sh / height, sh - 1);
            const std::size_t srcRow = static_cast<std::size_t>(srcY) * srcStride;
            for (std::uint32_t x = 0; x < width; ++x) {
                const auto srcX = std::min<std::uint32_t>(x * sw / width, sw - 1);
                const std::size_t srcIdx =
                    srcRow + static_cast<std::size_t>(srcX) * kSrcBytesPerPixel;
                const std::size_t dstIdx =
                    (static_cast<std::size_t>(y) * width + x) * 3;
                out[dstIdx + 0] = src[srcIdx + 0];  // B
                out[dstIdx + 1] = src[srcIdx + 1];  // G
                out[dstIdx + 2] = src[srcIdx + 2];  // R
            }
        }
        return true;
    }

    TsdfFrameSource::TsdfFrameSource(
        edgevision::model::scene::SharedScene& scene,
        std::function<bool(const PoseUpdate&, std::vector<std::uint8_t>&)> renderer
    )
        : m_scene(scene), m_renderer(std::move(renderer)) {}

    void TsdfFrameSource::setRenderer(
        std::function<bool(const PoseUpdate&, std::vector<std::uint8_t>&)> renderer
    ) {
        std::lock_guard<std::mutex> g(m_mu);
        m_renderer = std::move(renderer);
    }

    bool TsdfFrameSource::render(
        const PoseUpdate& pose, std::vector<std::uint8_t>& out
    ) {
        // Copy under lock so setRenderer can't invalidate the callable mid-call.
        std::function<bool(const PoseUpdate&, std::vector<std::uint8_t>&)> renderer;
        {
            std::lock_guard<std::mutex> g(m_mu);
            renderer = m_renderer;
        }
        if (!renderer) {
            return false;
        }
        return renderer(pose, out);
    }

} // namespace edgevision::streaming::webrtc
