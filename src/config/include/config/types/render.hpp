#pragma once

namespace edgevision::config {

    enum class ReadPolicy {
        Greedy,
        Balanced,
    };

    struct RenderConfig {
        int port = 6688;
        ReadPolicy readPolicy = ReadPolicy::Greedy;
    };

} // namespace edgevision::config
