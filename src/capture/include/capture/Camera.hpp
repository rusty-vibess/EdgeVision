#pragma once

#include <cstdint>

#include "interfaces/k4a.hpp"

class Camera {
  public:
    struct CameraConfig {
        const k4a_device_configuration_t cfg = [] {
            k4a_device_configuration_t c = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
            c.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
            c.color_resolution = K4A_COLOR_RESOLUTION_720P;
            c.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
            c.camera_fps = K4A_FRAMES_PER_SECOND_30;
            return c;
        }();
        const uint32_t captureTimeout = 1000;
    };

    explicit Camera(const k4aInterface& api = k4aInterface::instance());
    ~Camera();

    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;

    [[nodiscard]] int status() const;

    [[nodiscard]] bool open(uint32_t deviceIndex = 0);
    [[nodiscard]] bool start();
    void stop();
    void close();

    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] bool isStarted() const;

    [[nodiscard]] bool capture(k4a_capture_t& outCapture, int32_t timeoutMs = 0) const;
    [[nodiscard]] bool getCalibration(k4a_calibration_t& outCalibration) const;

    [[nodiscard]] k4a_device_t device() const;

    CameraConfig m_config = CameraConfig{};

  private:
    const k4aInterface& m_api;
    k4a_device_t m_device = nullptr;
    bool m_started = false;
};
