#include "frame/utils/MonotonicTimestamp.hpp"

#include <chrono>

namespace edgevision::capture::frame {

    edgevision::model::frame::FrameTimestamp nextMonotonicTimestamp(
        std::int64_t& lastTimestampTicks
    ) {
        using Clock = std::chrono::steady_clock;
        const auto now =
            std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch());

        std::int64_t ticks = now.count();
        if (ticks <= 0) {
            ticks = 1;
        }

        if (ticks <= lastTimestampTicks) {
            ticks = lastTimestampTicks + 1;
        }

        lastTimestampTicks = ticks;
        return edgevision::model::frame::FrameTimestamp{ticks};
    }

} // namespace edgevision::capture::frame
