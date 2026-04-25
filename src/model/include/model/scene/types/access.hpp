#pragma once

#include <cstdint>

namespace edgevision::model::scene {

    using SceneVersionId = std::uint64_t;

    enum class SceneReadPolicy {
        Greedy,
        Balanced,
    };

} // namespace edgevision::model::scene
