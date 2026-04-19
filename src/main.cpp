#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>

#include "capture/camera/CameraCapture.hpp"
#include "capture/config/K4aConfig.hpp"
#include "capture/frame/FrameIngestor.hpp"
#include "capture/frame/FrameIngestorRunner.hpp"
#include "config/CommandLineParser.hpp"
#include "model/frame/FrameStore.hpp"
#include "reconstruction/TorchGaussianResidualModel.hpp"
#include "streaming/RenderServer.hpp"
#include "types/HybridTypes.hpp"

/// Usage:  ./EdgeVision [--port 6688] [--enable-capture]
int main(int argc, char* argv[]) {
    const auto parseResult = edgevision::config::parseCommandLine(argc, argv);
    if (!parseResult.parsed()) {
        std::cerr << parseResult.error << std::endl;
        return 1;
    }

    const edgevision::config::AppConfig appConfig = parseResult.config;

    edgevision::model::frame::FrameStore frameStore{};
    edgevision::capture::CameraCapture camera{};
    std::unique_ptr<edgevision::capture::frame::FrameIngestor> frameIngestor{};
    std::unique_ptr<edgevision::capture::frame::FrameIngestorRunner> frameIngestorRunner{};

    if (appConfig.capture.enabled) {
        if (!camera.open(appConfig.capture.camera.deviceIndex)) {
            std::cerr << "Failed to open K4A device" << std::endl;
            return 1;
        }

        const k4a_device_configuration_t k4aConfig =
            edgevision::capture::makeK4aDeviceConfig(appConfig.capture.camera);
        if (!camera.start(k4aConfig)) {
            std::cerr << "Failed to start K4A cameras" << std::endl;
            return 1;
        }

        frameIngestor =
            std::make_unique<edgevision::capture::frame::FrameIngestor>(camera, frameStore);
        frameIngestorRunner = std::make_unique<edgevision::capture::frame::FrameIngestorRunner>(
            *frameIngestor, appConfig.capture.runtime
        );
        if (!frameIngestorRunner->start()) {
            std::cerr << "Failed to start capture frame ingestor" << std::endl;
            return 1;
        }
    }

    TorchGaussianResidualModel reconstructionModel;
    if (!reconstructionModel.isBackendAvailable()) {
        std::cerr << "Warning: CUDA backend not available. "
                     "Frames will be empty."
                  << std::endl;
    }

    auto renderThread = startRenderServer(
        appConfig.render.port,
        [&reconstructionModel](
            const RenderPoseRequest& req, std::vector<uint8_t>& rgbOut
        ) -> bool {
            ImageSize imgSize{req.width, req.height};
            CameraIntrinsics intrinsics{req.fx, req.fy, req.cx, req.cy};

            Pose4f pose{};
            std::memcpy(pose.matrix.data(), req.pose, 16 * sizeof(float));

            RgbdFrameView frame{};
            frame.rgb.size = imgSize;
            frame.depth.size = imgSize;
            frame.intrinsics = intrinsics;
            frame.cameraToWorld = pose;
            frame.cameraToWorldSlam = pose;
            frame.hasDepth = false;

            ReconstructionObservation obs{};
            obs.frame = frame;

            HybridRenderInput input{};
            input.observation = obs;

            HybridRenderOutput output = reconstructionModel.renderHybrid(input);

            // Convert float RGB [0,1] → uint8 sRGB.
            const auto* src = output.rgb.data();
            const auto numElements =
                static_cast<std::size_t>(req.width) * static_cast<std::size_t>(req.height) * 3;
            if (src != nullptr && output.rgb.size() >= numElements) {
                for (std::size_t i = 0; i < numElements; ++i) {
                    float v = src[i];
                    if (v < 0.0f) {
                        v = 0.0f;
                    }

                    if (v > 1.0f) {
                        v = 1.0f;
                    }

                    rgbOut[i] = static_cast<uint8_t>(v * 255.0f);
                }
                return true;
            }
            return false;
        }
    );
    renderThread.join();

    if (frameIngestorRunner) {
        frameIngestorRunner->requestStop();
        frameIngestorRunner->join();
    }

    return 0;
}
