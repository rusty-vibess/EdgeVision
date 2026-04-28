#pragma once

#include <cstddef>
#include <memory>
#include <utility>

#include "config/types/image.hpp"

namespace edgevision::model::frame {

    using ImageSize = edgevision::config::ImageSize;
    using edgevision::config::operator!=;
    using edgevision::config::operator==;

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
