#pragma once

#include <k4a/k4a.h>
#include <memory.h>

#include "capture/Camera.hpp"
#include "types/Image.hpp"

class CameraInterface {
    CameraInterface();

  private:
    void capture();
    ColorImage getCaptureColorImage();


    k4a_capture_t m_lastCapture = nullptr;
    Camera m_camera;
};
