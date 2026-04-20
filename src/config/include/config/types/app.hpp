#pragma once

#include "config/types/capture.hpp"
#include "config/types/render.hpp"

namespace edgevision::config {

    struct AppConfig {
        RenderConfig render{};
        CaptureConfig capture{};
    };

} // namespace edgevision::config
