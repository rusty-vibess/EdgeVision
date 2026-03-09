#include "capture/Camera.hpp"

Camera::Camera(const k4aInterface& api) : m_api(api) {}

Camera::~Camera() {
    stop();
    close();
}

int Camera::status() const {
    return m_device != nullptr ? 1 : 0;
}

bool Camera::open(uint32_t deviceIndex) {
    if (m_device != nullptr) {
        return true;
    }

    if (m_api.deviceOpen(deviceIndex, &m_device) != K4A_RESULT_SUCCEEDED) {
        m_device = nullptr;
        return false;
    }
    return true;
}

bool Camera::start() {
    if (m_device == nullptr) {
        return false;
    }

    if (m_started) {
        return true;
    }

    if (!m_api.deviceStartCameras(m_device, &m_config.cfg)) {
        return false;
    }

    m_started = true;
    return true;
}

void Camera::stop() {
    if (m_device == nullptr || !m_started) {
        return;
    }

    m_api.deviceStopCameras(m_device);
    m_started = false;
}

void Camera::close() {
    if (m_device == nullptr) {
        return;
    }

    m_api.deviceClose(m_device);
    m_device = nullptr;
    m_started = false;
}

bool Camera::isOpen() const {
    return m_device != nullptr;
}

bool Camera::isStarted() const {
    return m_started;
}

bool Camera::capture(k4a_capture_t& outCapture, int32_t timeoutMs) const {
    if (m_device == nullptr || !m_started) {
        return false;
    }

    const k4a_wait_result_t result = m_api.deviceGetCapture(m_device, &outCapture, timeoutMs);
    return result == K4A_WAIT_RESULT_SUCCEEDED;
}

bool Camera::getCalibration(k4a_calibration_t& outCalibration) const {
    if (m_device == nullptr) {
        return false;
    }

    return m_api.deviceGetCalibration(
               m_device, m_config.cfg.depth_mode, m_config.cfg.color_resolution, &outCalibration
           )
        == K4A_RESULT_SUCCEEDED;
}

k4a_device_t Camera::device() const {
    return m_device;
}
