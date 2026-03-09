#include "interfaces/CameraInterface.hpp"

#include <iostream>
#include <optional>

#include "logger.hpp"

CameraInterface::CameraInterface() {
    if (!m_camera.start()) {
        throw std::runtime_error("CameraInterface: m_camera.start() failed");
    }
}

void CameraInterface::capture() {
    if (!m_camera.capture(m_lastCapture, m_camera.m_config.captureTimeout)) {
        LOG(LogLevel::debug, "capture", "CameraInterface::capture did not succeed");
    }
}

ColorImage CameraInterface::getCaptureColorImage() {
    if (m_lastCapture == nullptr) {
        LOG(LogLevel::debug,
            "capture",
            "CameraInterface: getCaputureColorImage() failed, m_lastCapture == nullptr");
    }
}
