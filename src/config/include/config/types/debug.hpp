#pragma once

#include <cstddef>

namespace edgevision::config {

    struct ViewerDumpConfig {
        bool enabled = false;
        std::size_t maxFreshOutputs = 1;
    };

    struct AppDebugConfig {
        ViewerDumpConfig viewerDump{};
    };

} // namespace edgevision::config
