#pragma once

namespace edgevision::config {

    enum class SceneReadPolicy {
        Greedy,
        Balanced,
    };

    struct SceneConfig {
        SceneReadPolicy readPolicy = SceneReadPolicy::Greedy;
    };

} // namespace edgevision::config
