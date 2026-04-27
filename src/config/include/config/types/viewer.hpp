#pragma once

#include <cstddef>

namespace edgevision::config {

    enum class ViewerLoopPolicy {
        Event,
        HotLoop,
    };

    struct ViewerRuntimeConfig {
        ViewerLoopPolicy loopPolicy = ViewerLoopPolicy::Event;
        int loopPeriodMs = (1000 / 30);
        std::size_t outputHistoryCapacity = 8;
    };

} // namespace edgevision::config
