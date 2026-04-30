#pragma once

namespace edgevision::config {

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

} // namespace edgevision::config
