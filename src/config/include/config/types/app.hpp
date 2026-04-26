#pragma once

#include "config/types/builder.hpp"
#include "config/types/capture.hpp"
#include "config/types/debug.hpp"
#include "config/types/scene.hpp"
#include "config/types/streaming.hpp"
#include "config/types/viewer.hpp"

namespace edgevision::config {

    struct AppConfig {
        StreamingConfig streaming{};
        CaptureConfig capture{};
        BuilderRuntimeConfig builder{};
        SceneConfig scene{};
        ViewerRuntimeConfig viewer{};
        AppDebugConfig debug{};
    };

} // namespace edgevision::config
