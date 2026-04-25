#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include "model/frame/types/frame.hpp"

namespace edgevision::builder::tests {

    inline edgevision::model::frame::FrameBuffer makeBuffer(std::vector<std::byte> bytes) {
        auto storage = std::make_shared<std::vector<std::byte>>(std::move(bytes));
        return edgevision::model::frame::FrameBuffer(storage->data(), storage->size(), storage);
    }

    inline edgevision::model::frame::Frame makeValidFrame(
        edgevision::model::frame::FrameId frameId,
        std::int64_t timestampTicks
    ) {
        using namespace edgevision::model::frame;

        constexpr int width = 16;
        constexpr int height = 12;

        std::vector<std::byte> rgbStorage(width * height * 4);
        for (std::size_t index = 0; index < rgbStorage.size(); ++index) {
            rgbStorage[index] = static_cast<std::byte>(index % 255);
        }

        std::vector<std::byte> depthStorage(width * height * 2);
        for (int index = 0; index < width * height; ++index) {
            const std::uint16_t depthMillimetres = 1000;
            std::memcpy(
                depthStorage.data() + static_cast<std::size_t>(index) * sizeof(std::uint16_t),
                &depthMillimetres,
                sizeof(depthMillimetres)
            );
        }

        Frame frame{};
        frame.frameId = frameId;
        frame.timestamp = FrameTimestamp{timestampTicks};
        frame.rgb.size = ImageSize{width, height};
        frame.rgb.strideBytes = width * 4;
        frame.rgb.buffer = makeBuffer(std::move(rgbStorage));
        frame.depth.size = ImageSize{width, height};
        frame.depth.strideBytes = width * 2;
        frame.depth.buffer = makeBuffer(std::move(depthStorage));
        frame.intrinsics = CameraIntrinsics{525.0f, 525.0f, width / 2.0f, height / 2.0f};
        return frame;
    }

    inline edgevision::model::frame::Frame makeInvalidRgbLayoutFrame(
        edgevision::model::frame::FrameId frameId,
        std::int64_t timestampTicks
    ) {
        using namespace edgevision::model::frame;

        Frame frame = makeValidFrame(frameId, timestampTicks);
        frame.rgb.size = ImageSize{2, 2};
        frame.rgb.strideBytes = 6;
        frame.rgb.buffer = makeBuffer(std::vector<std::byte>(12));
        frame.depth.size = ImageSize{2, 2};
        frame.depth.strideBytes = 4;
        frame.depth.buffer = makeBuffer(std::vector<std::byte>(8));
        frame.intrinsics = CameraIntrinsics{525.0f, 525.0f, 1.0f, 1.0f};
        return frame;
    }

} // namespace edgevision::builder::tests
