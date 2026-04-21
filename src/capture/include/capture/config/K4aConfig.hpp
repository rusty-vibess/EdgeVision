#pragma once

#include <k4a/k4a.h>

#include "config/types/capture.hpp"

namespace edgevision::capture {

    [[nodiscard]] k4a_device_configuration_t makeK4aDeviceConfig(
        const edgevision::config::CaptureCameraConfig& config
    );

} // namespace edgevision::capture
