#pragma once

#include <cstdint>

namespace edgevision::model::frame {

    using FrameId = std::uint64_t;

    struct FrameTimestamp {
        std::int64_t ticks = 0;
    };

    [[nodiscard]] constexpr bool operator==(const FrameTimestamp& lhs, const FrameTimestamp& rhs) {
        return lhs.ticks == rhs.ticks;
    }

    [[nodiscard]] constexpr bool operator!=(const FrameTimestamp& lhs, const FrameTimestamp& rhs) {
        return !(lhs == rhs);
    }

} // namespace edgevision::model::frame
