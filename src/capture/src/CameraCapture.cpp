#include "CameraCapture.hpp"

CameraCapture::CameraCapture() = default;

CameraCapture::~CameraCapture() {
    stop();
    close();
}

int CameraCapture::status() const {
    return m_device != nullptr ? 1 : 0;
}

bool CameraCapture::open(uint32_t deviceIndex) {
    if (m_device != nullptr) {
        return true;
    }

    if (k4a_device_open(deviceIndex, &m_device) != K4A_RESULT_SUCCEEDED) {
        m_device = nullptr;
        return false;
    }
    return true;
}

bool CameraCapture::start(const k4a_device_configuration_t& config) {
    if (m_device == nullptr) {
        return false;
    }

    if (m_started) {
        return true;
    }

    if (k4a_device_start_cameras(m_device, &config) != K4A_RESULT_SUCCEEDED) {
        return false;
    }
    m_config = config;
    m_started = true;
    return true;
}

void CameraCapture::stop() {
    if (m_device == nullptr || !m_started) {
        return;
    }

    k4a_device_stop_cameras(m_device);
    m_started = false;
}

void CameraCapture::close() {
    if (m_device == nullptr) {
        return;
    }

    k4a_device_close(m_device);
    m_device = nullptr;
    m_started = false;
}

bool CameraCapture::isOpen() const {
    return m_device != nullptr;
}

bool CameraCapture::isStarted() const {
    return m_started;
}

bool CameraCapture::capture(k4a_capture_t& outCapture, int32_t timeoutMs) const {
    if (m_device == nullptr || !m_started) {
        return false;
    }

    const k4a_wait_result_t result = k4a_device_get_capture(m_device, &outCapture, timeoutMs);
    return result == K4A_WAIT_RESULT_SUCCEEDED;
}

bool CameraCapture::getCalibration(k4a_calibration_t& outCalibration) const {
    if (m_device == nullptr) {
        return false;
    }

    return k4a_device_get_calibration(
               m_device, m_config.depth_mode, m_config.color_resolution, &outCalibration
           )
        == K4A_RESULT_SUCCEEDED;
}

k4a_device_t CameraCapture::device() const {
    return m_device;
}
