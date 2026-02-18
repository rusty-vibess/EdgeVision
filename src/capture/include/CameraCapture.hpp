#pragma once

#include <cstdint>

#include "interfaces/k4a.hpp"

class CameraCapture {
  public:
    explicit CameraCapture(const k4aInterface& api = k4aInterface::instance());
    ~CameraCapture();

    CameraCapture(const CameraCapture&) = delete;
    CameraCapture& operator=(const CameraCapture&) = delete;

    [[nodiscard]] int status() const;

    [[nodiscard]] bool open(uint32_t deviceIndex = 0);
    [[nodiscard]] bool start(
        const k4a_device_configuration_t& config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL
    );
    void stop();
    void close();

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] bool isStarted() const;

    [[nodiscard]] bool capture(k4a_capture_t& outCapture, int32_t timeoutMs = 0) const;
    [[nodiscard]] bool getCalibration(k4a_calibration_t& outCalibration) const;

    [[nodiscard]] k4a_device_t device() const;

  private:
    const k4aInterface& m_api;
    k4a_device_t m_device = nullptr;
    bool m_started = false;
    k4a_device_configuration_t m_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
};
