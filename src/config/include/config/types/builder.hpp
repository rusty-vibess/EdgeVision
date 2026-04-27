#pragma once

#include <string>

namespace edgevision::config {

    struct BuilderRuntimeConfig {
        int readyFrameTimeoutMs = (1000 / 20);
        std::string trackerConfig{};
    };

} // namespace edgevision::config
