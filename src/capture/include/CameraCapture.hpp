#pragma once

#include <cstdint>
#include <k4a/k4a.h>

class CameraCapture {
  public:
    CameraCapture();
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
    k4a_device_t m_device = nullptr;
    bool m_started = false;
    k4a_device_configuration_t m_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
};
