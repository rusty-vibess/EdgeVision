#pragma once

#include <cstdint>

#include "model/frame/types/identity.hpp"

namespace edgevision::capture::frame {

    [[nodiscard]] edgevision::model::frame::FrameTimestamp nextMonotonicTimestamp(
        std::int64_t& lastTimestampTicks
    );

} // namespace edgevision::capture::frame
