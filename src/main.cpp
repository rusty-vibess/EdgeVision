#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "reconstruction/TorchGaussianResidualModel.hpp"
#include "streaming/RenderServer.hpp"
#include "types/HybridTypes.hpp"

/// Render-server entry point. Starts a TCP server and renders frames via
/// TorchGaussianResidualModel::renderHybrid(). Currently produces black
/// frames (the model stub returns empty output) — once the real gsplat
/// rasterisation lands behind renderHybrid(), frames start flowing with
/// zero code change here.
///
/// Usage:  ./EdgeVision [--port 6688]
int main(int argc, char* argv[]) {
    int port = 6688;
    for (int i = 1; i < argc - 1; ++i) {
        if (std::strcmp(argv[i], "--port") == 0) {
            port = std::atoi(argv[i + 1]);
        }
    }
    if (port <= 0 || port > 65535) {
        std::cerr << "Invalid port: " << port << std::endl;
        return 1;
    }

    TorchGaussianResidualModel model;
    if (!model.isBackendAvailable()) {
        std::cerr << "Warning: CUDA backend not available. "
                     "Frames will be empty." << std::endl;
    }

    auto renderThread = startRenderServer(port, [&model](
        const RenderPoseRequest& req,
        std::vector<uint8_t>& rgbOut
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

        HybridRenderOutput output = model.renderHybrid(input);

        // Convert float RGB [0,1] → uint8 sRGB.
        const auto* src = output.rgb.data();
        const auto numElements = static_cast<std::size_t>(req.width) *
                                 static_cast<std::size_t>(req.height) * 3;
        if (src != nullptr && output.rgb.size() >= numElements) {
            for (std::size_t i = 0; i < numElements; ++i) {
                float v = src[i];
                if (v < 0.0f) v = 0.0f;
                if (v > 1.0f) v = 1.0f;
                rgbOut[i] = static_cast<uint8_t>(v * 255.0f);
            }
            return true;
        }
        return false;
    });
    renderThread.join();

    return 0;
}
