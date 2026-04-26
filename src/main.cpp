#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>

#include "app/runtime/ViewerFrameDumper.hpp"
#include "app/runtime/ViewerPoseSeeder.hpp"
#include "builder/SceneBuilderRunner.hpp"
#include "capture/camera/CameraCapture.hpp"
#include "capture/config/K4aConfig.hpp"
#include "capture/frame/FrameIngestor.hpp"
#include "capture/frame/FrameIngestorRunner.hpp"
#include "config/CommandLineParser.hpp"
#include "model/frame/FrameStore.hpp"
#include "model/scene/SceneVersionStore.hpp"
#include "model/scene/SharedScene.hpp"
#include "model/viewer/RenderOutputStore.hpp"
#include "model/viewer/ViewerPoseStore.hpp"
#include "viewer/SceneViewer.hpp"
#include "viewer/ViewerRunner.hpp"

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

    edgevision::capture::CameraCapture camera{};
    std::unique_ptr<edgevision::capture::frame::FrameIngestor> frameIngestor{};
    std::unique_ptr<edgevision::capture::frame::FrameIngestorRunner> frameIngestorRunner{};

    edgevision::builder::SceneBuilderRunner sceneBuilderRunner(
        frameStore, sharedScene, sceneVersionStore, appConfig.builder
    );
    edgevision::viewer::SceneViewer sceneViewer(viewerPoseStore, sharedScene, renderOutputStore);
    edgevision::viewer::ViewerRunner viewerRunner(sceneViewer, viewerPoseStore, appConfig.viewer);
    edgevision::app::runtime::ViewerPoseSeeder viewerPoseSeeder(
        frameStore, sceneVersionStore, viewerPoseStore
    );
    edgevision::app::runtime::ViewerFrameDumper viewerFrameDumper(
        renderOutputStore, appConfig.debug.viewerDump
    );

    const auto cleanup = [&]() {
        viewerRunner.requestStop();
        viewerRunner.join();
        sceneBuilderRunner.requestStop();
        sceneBuilderRunner.join();

        if (frameIngestorRunner) {
            frameIngestorRunner->requestStop();
            frameIngestorRunner->join();
        }

        if (camera.isStarted()) {
            camera.stop();
        }

        if (camera.isOpen()) {
            camera.close();
        }
    };

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

    frameIngestor =
        std::make_unique<edgevision::capture::frame::FrameIngestor>(camera, frameStore);
    frameIngestorRunner = std::make_unique<edgevision::capture::frame::FrameIngestorRunner>(
        *frameIngestor, appConfig.capture.runtime
    );
    if (!frameIngestorRunner->start()) {
        std::cerr << "Failed to start capture frame ingestor" << std::endl;
        cleanup();
        return 1;
    }

    if (!sceneBuilderRunner.start()) {
        std::cerr << "Failed to start scene builder runner" << std::endl;
        cleanup();
        return 1;
    }

    if (!viewerRunner.start()) {
        std::cerr << "Failed to start viewer runner" << std::endl;
        cleanup();
        return 1;
    }

    if (!viewerPoseSeeder.seedOnce(kViewerSeedTimeout)) {
        std::cerr << "Timed out waiting to seed the initial viewer pose" << std::endl;
        cleanup();
        return 1;
    }

    if (!viewerFrameDumper.waitForConfiguredOutputs(kViewerOutputTimeout)) {
        std::cerr << "Timed out waiting for viewer render outputs" << std::endl;
        cleanup();
        return 1;
    }

    if (viewerFrameDumper.dumpingEnabled()) {
        std::cout << "Dumped " << viewerFrameDumper.dumpedFreshOutputCount()
                  << " viewer frame(s) to " << viewerFrameDumper.outputDirectory() << std::endl;
    } else {
        std::cout << "Observed first fresh viewer render output" << std::endl;
    }

    cleanup();

    return 0;
}
