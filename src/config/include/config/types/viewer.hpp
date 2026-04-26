#pragma once

#include <cstddef>

namespace edgevision::config {

    enum class ViewerLoopPolicy {
        PoseDriven,
        LiveLoop,
    };

    struct ViewerRuntimeConfig {
        ViewerLoopPolicy loopPolicy = ViewerLoopPolicy::PoseDriven;
        int loopPeriodMs = (1000 / 30);
        std::size_t outputHistoryCapacity = 8;
    };

} // namespace edgevision::config
