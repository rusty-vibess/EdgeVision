#pragma once

#include "config/types/builder.hpp"
#include "config/types/capture.hpp"
#include "config/types/render.hpp"
#include "config/types/viewer.hpp"

namespace edgevision::config {

    struct AppConfig {
        RenderConfig render{};
        CaptureConfig capture{};
        BuilderRuntimeConfig builder{};
        ViewerRuntimeConfig viewer{};
    };

} // namespace edgevision::config
