#pragma once

#include <cstddef>
#include <memory>
#include <utility>

namespace edgevision::model::frame {

    struct ImageSize {
        int width = 0;
        int height = 0;
    };

    [[nodiscard]] constexpr bool operator==(const ImageSize& lhs, const ImageSize& rhs) {
        return lhs.width == rhs.width && lhs.height == rhs.height;
    }

    [[nodiscard]] constexpr bool operator!=(const ImageSize& lhs, const ImageSize& rhs) {
        return !(lhs == rhs);
    }

    enum class FrameColorFormat {
        Unknown,
        Rgb8,
        Bgra32,
    };

    enum class FrameDepthFormat {
        Unknown,
        Depth16Millimeters,
    };

    struct FrameBuffer {
        const std::byte* data = nullptr;
        std::size_t byteCount = 0;
        std::shared_ptr<const void> owner{};

        FrameBuffer() = default;

        FrameBuffer(
            const std::byte* dataPtr,
            std::size_t sizeBytes,
            std::shared_ptr<const void> lifetimeOwner
        )
            : data(dataPtr), byteCount(sizeBytes), owner(std::move(lifetimeOwner)) {}

        [[nodiscard]] bool empty() const {
            return data == nullptr || byteCount == 0 || owner == nullptr;
        }
    };

    struct FrameImage {
        ImageSize size{};
        int strideBytes = 0;
        FrameBuffer buffer{};
    };

} // namespace edgevision::model::frame
