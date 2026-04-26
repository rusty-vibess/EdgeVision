#include <algorithm>
#include <chrono>
#include <iostream>

#include "app/runtime/ViewerFrameDumper.hpp"
#include "app/runtime/ViewerPoseSeeder.hpp"
#include "builder/SceneBuilderRunner.hpp"
#include "capture/camera/CameraCapture.hpp"
#include "capture/config/K4aConfig.hpp"
#include "capture/frame/FrameIngestorRunner.hpp"
#include "config/CommandLineParser.hpp"
#include "model/frame/FrameStore.hpp"
#include "model/scene/SceneVersionStore.hpp"
#include "model/scene/SharedScene.hpp"
#include "model/viewer/RenderOutputStore.hpp"
#include "model/viewer/ViewerPoseStore.hpp"
#include "viewer/SceneViewerRunner.hpp"

namespace {
    using namespace std::chrono_literals;

    constexpr auto kViewerSeedTimeout = 30s;
    constexpr auto kViewerOutputTimeout = 30s;
} // namespace

/// Usage: ./EdgeVision [--port 6688] [--enable-capture]
///                      [--read-policy greedy|balanced]
///                      [--dump-viewer-frames N]
int main(int argc, char* argv[]) {
    const auto parseResult = edgevision::config::parseCommandLine(argc, argv);
    if (!parseResult.parsed()) {
        std::cerr << parseResult.error << std::endl;
        return 1;
    }

    const edgevision::config::AppConfig appConfig = parseResult.config;
    if (!appConfig.capture.enabled) {
        std::cerr << "Viewer smoke test requires --enable-capture" << std::endl;
        return 1;
    }

    // Import types
    edgevision::capture::CameraCapture camera{};
    edgevision::model::frame::FrameStore frameStore{};
    edgevision::model::scene::SharedScene sharedScene{appConfig.scene};
    edgevision::model::scene::SceneVersionStore sceneVersionStore{};
    edgevision::model::viewer::ViewerPoseStore viewerPoseStore{};
    const std::size_t renderOutputHistoryCapacity = std::max(
        appConfig.viewer.outputHistoryCapacity,
        appConfig.debug.viewerDump.enabled ? appConfig.debug.viewerDump.maxFreshOutputs
                                           : std::size_t{0}
    );
    edgevision::model::viewer::RenderOutputStore renderOutputStore{renderOutputHistoryCapacity};

    // Loop runners
    edgevision::capture::frame::FrameIngestorRunner frameIngestorRunner(
        camera, frameStore, appConfig.capture.runtime
    );
    edgevision::builder::SceneBuilderRunner sceneBuilderRunner(
        frameStore, sharedScene, sceneVersionStore, appConfig.builder
    );
    edgevision::viewer::SceneViewerRunner sceneViewerRunner(
        viewerPoseStore, sharedScene, renderOutputStore, appConfig.viewer
    );
    edgevision::app::runtime::ViewerPoseSeeder viewerPoseSeeder(
        frameStore, sceneVersionStore, viewerPoseStore
    );
    edgevision::app::runtime::ViewerFrameDumper viewerFrameDumper(
        renderOutputStore, appConfig.debug.viewerDump
    );

    // Cleanup inline util
    const auto cleanup = [&]() {
        sceneViewerRunner.requestStop();
        sceneViewerRunner.join();
        sceneBuilderRunner.requestStop();
        sceneBuilderRunner.join();

        frameIngestorRunner.requestStop();
        frameIngestorRunner.join();

        if (camera.isStarted()) {
            camera.stop();
        }

        if (camera.isOpen()) {
            camera.close();
        }
    };

    // Starts runners and sublibs with their relevant failures
    if (!camera.open(appConfig.capture.camera.deviceIndex)) {
        std::cerr << "Failed to open K4A device" << std::endl;
        return 1;
    }

    const k4a_device_configuration_t k4aConfig =
        edgevision::capture::makeK4aDeviceConfig(appConfig.capture.camera);
    if (!camera.start(k4aConfig)) {
        std::cerr << "Failed to start K4A cameras" << std::endl;
        cleanup();
        return 1;
    }

    if (!frameIngestorRunner.start()) {
        std::cerr << "Failed to start capture frame ingestor" << std::endl;
        cleanup();
        return 1;
    }

    if (!sceneBuilderRunner.start()) {
        std::cerr << "Failed to start scene builder runner" << std::endl;
        cleanup();
        return 1;
    }

    if (!sceneViewerRunner.start()) {
        std::cerr << "Failed to start scene viewer runner" << std::endl;
        cleanup();
        return 1;
    }

    // Seed viewer pose to camera origin
    if (!viewerPoseSeeder.seedOnce(kViewerSeedTimeout)) {
        std::cerr << "Timed out waiting to seed the initial viewer pose" << std::endl;
        cleanup();
        return 1;
    }

    // Debug mode: Exist until we hit timeout, dump frames
    if (!viewerFrameDumper.waitForConfiguredOutputs(kViewerOutputTimeout)) {
        std::cerr << "Timed out waiting for viewer render outputs" << std::endl;
        cleanup();
        return 1;
    }

    // Debug mode: Inform on dump
    if (viewerFrameDumper.dumpingEnabled()) {
        std::cout << "Dumped " << viewerFrameDumper.dumpedFreshOutputCount()
                  << " viewer frame(s) to " << viewerFrameDumper.outputDirectory() << std::endl;
    } else {
        std::cout << "Observed first fresh viewer render output" << std::endl;
    }

    // Die
    cleanup();

    return 0;
}
