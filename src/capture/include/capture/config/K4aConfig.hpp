#pragma once

#include <k4a/k4a.h>
#include <optional>

#include "config/types/capture.hpp"
#include "config/types/image.hpp"

namespace edgevision::capture {

    [[nodiscard]] std::optional<k4a_device_configuration_t> makeK4aDeviceConfig(
        const edgevision::config::CaptureCameraConfig& config,
        const edgevision::config::ImageSize& imageSize
    );

} // namespace edgevision::capture
