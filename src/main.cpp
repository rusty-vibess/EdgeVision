#include <chrono>
#include <iomanip>
#include <iostream>

#include "capture/Camera.hpp"
#include "capture/Frame.hpp"

// Test run of camera stream
// int main() {
//     Camera camera;

//     if (!camera.open()) {
//         std::cerr << "Failed to open camera device\n";
//         return 1;
//     }
//     std::cout << "Camera open status: " << camera.status() << "\n";

//     k4a_device_configuration_t config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
//     config.color_format = K4A_IMAGE_FORMAT_COLOR_BGRA32;
//     config.color_resolution = K4A_COLOR_RESOLUTION_720P;
//     config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
//     config.camera_fps = K4A_FRAMES_PER_SECOND_30;

//     if (!camera.start(config)) {
//         std::cerr << "Failed to start camera\n";
//         return 1;
//     }

//     std::cout << "Camera started: " << (camera.isStarted() ? "yes" : "no") << "\n";
//     std::cout << "Pose estimate: body tracking is not enabled; using IMU motion as pose proxy\n";

//     const bool imuStarted = k4a_device_start_imu(camera.device()) == K4A_RESULT_SUCCEEDED;
//     std::cout << "IMU started: " << (imuStarted ? "yes" : "no") << "\n";

//     constexpr int kMaxFrames = 100;
//     const auto startTime = std::chrono::steady_clock::now();
//     auto nextLogTime = startTime + std::chrono::seconds(1);

//     int frameIndex = 0;
//     int timeoutCount = 0;
//     int colorFrameCount = 0;
//     int depthFrameCount = 0;
//     int lastColorWidth = 0;
//     int lastColorHeight = 0;
//     int lastDepthWidth = 0;
//     int lastDepthHeight = 0;
//     size_t lastColorBytes = 0;
//     size_t lastDepthBytes = 0;
//     uint64_t lastColorTsUs = 0;
//     uint64_t lastDepthTsUs = 0;
//     bool imuSampleValid = false;
//     float lastAccelX = 0.0f;
//     float lastAccelY = 0.0f;
//     float lastAccelZ = 0.0f;
//     float lastGyroX = 0.0f;
//     float lastGyroY = 0.0f;
//     float lastGyroZ = 0.0fypnb;
//     uint64_t lastImuTsUs = 0;

//     while (frameIndex < kMaxFrames) {
//         k4a_capture_t capture = nullptr;
//         if (camera.capture(capture, 1000)) {
//             ++frameIndex;
//             if (capture != nullptr) {
//                 k4a_image_t colorImage = k4a_capture_get_color_image(capture);
//                 if (colorImage != nullptr) {
//                     ++colorFrameCount;
//                     lastColorWidth = k4a_image_get_width_pixels(colorImage);
//                     lastColorHeight = k4a_image_get_height_pixels(colorImage);
//                     lastColorBytes = static_cast<size_t>(k4a_image_get_size(colorImage));
//                     lastColorTsUs = static_cast<uint64_t>(k4a_image_get_device_timestamp_usec(colorImage));
//                     k4a_image_release(colorImage);
//                 }

//                 k4a_image_t depthImage = k4a_capture_get_depth_image(capture);
//                 if (depthImage != nullptr) {
//                     ++depthFrameCount;
//                     lastDepthWidth = k4a_image_get_width_pixels(depthImage);
//                     lastDepthHeight = k4a_image_get_height_pixels(depthImage);
//                     lastDepthBytes = static_cast<size_t>(k4a_image_get_size(depthImage));
//                     lastDepthTsUs = static_cast<uint64_t>(k4a_image_get_device_timestamp_usec(depthImage));
//                     k4a_image_release(depthImage);
//                 }

//                 k4a_capture_release(capture);
//             }
//         } else {
//             ++timeoutCount;
//         }

//         if (imuStarted) {
//             k4a_imu_sample_t sample{};
//             if (k4a_device_get_imu_sample(camera.device(), &sample, 0) == K4A_WAIT_RESULT_SUCCEEDED) {
//                 imuSampleValid = true;
//                 lastAccelX = sample.acc_sample.xyz.x;
//                 lastAccelY = sample.acc_sample.xyz.y;
//                 lastAccelZ = sample.acc_sample.xyz.z;
//                 lastGyroX = sample.gyro_sample.xyz.x;
//                 lastGyroY = sample.gyro_sample.xyz.y;
//                 lastGyroZ = sample.gyro_sample.xyz.z;
//                 lastImuTsUs = static_cast<uint64_t>(sample.acc_timestamp_usec);
//             }
//         }

//         const auto now = std::chrono::steady_clock::now();
//         if (now >= nextLogTime) {
//             const auto elapsedMs =
//                 std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
//             const double elapsedSec = static_cast<double>(elapsedMs) / 1000.0;
//             const double fps = elapsedSec > 0.0 ? static_cast<double>(frameIndex) / elapsedSec : 0.0;

//             std::cout << std::fixed << std::setprecision(2)
//                       << "[capture] t=" << elapsedSec << "s"
//                       << " frames=" << frameIndex
//                       << " fps~" << fps
//                       << " timeouts=" << timeoutCount
//                       << " color=" << colorFrameCount
//                       << " depth=" << depthFrameCount << "\n";

//             std::cout << "          color_last=" << lastColorWidth << "x" << lastColorHeight
//                       << " bytes=" << lastColorBytes
//                       << " ts_us=" << lastColorTsUs
//                       << " depth_last=" << lastDepthWidth << "x" << lastDepthHeight
//                       << " bytes=" << lastDepthBytes
//                       << " ts_us=" << lastDepthTsUs << "\n";

//             if (imuSampleValid) {
//                 std::cout << std::fixed << std::setprecision(3)
//                           << "          imu(ts_us=" << lastImuTsUs << ")"
//                           << " acc=(" << lastAccelX << "," << lastAccelY << "," << lastAccelZ << ")"
//                           << " gyro=(" << lastGyroX << "," << lastGyroY << "," << lastGyroZ << ")\n";
//             } else if (imuStarted) {
//                 std::cout << "          imu=no-sample-yet\n";
//             } else {
//                 std::cout << "          imu=unavailable\n";
//             }

//             nextLogTime += std::chrono::seconds(1);
//         }
//     }

//     std::cout << "Capture loop complete: frames=" << frameIndex
//               << ", timeouts=" << timeoutCount
//               << ", color=" << colorFrameCount
//               << ", depth=" << depthFrameCount << "\n";

//     if (imuStarted) {
//         k4a_device_stop_imu(camera.device());
//     }
//     camera.stop();
//     camera.close();

//     return 0;
// }

#include "logger.hpp"

int main() {
    Logger::instance().set_min_level(LogLevel::debug);
    LOG(LogLevel::info, "EdgeVision", "Staring...");
}
