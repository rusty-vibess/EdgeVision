#include <algorithm>
#include <chrono>
#include <iostream>

#include "app/runtime/ViewerDebugSession.hpp"
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

/// Usage: ./EdgeVision [--port 6688] [--disable-capture]
///                      [--read-policy greedy|balanced]
///                      [--viewer-policy event|hot-loop]
///                      [--enable-debug]
///                      [--debug-frames N]
int main(int argc, char* argv[]) {
    const auto parseResult = edgevision::config::parseCommandLine(argc, argv);
    if (!parseResult.parsed()) {
        std::cerr << parseResult.error << std::endl;
        return 1;
    }

    const edgevision::config::AppConfig appConfig = parseResult.config;
    if (!appConfig.capture.enabled) {
        std::cerr << "Viewer smoke test requires capture to remain enabled" << std::endl;
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
        appConfig.debug.viewerDump.enabled ? appConfig.debug.viewerDump.maxFrames : std::size_t{0}
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
    edgevision::app::runtime::ViewerDebugSession viewerDebugSession(
        renderOutputStore, appConfig.debug.viewerDump, [&]() {
            const auto ingestStatus = frameIngestorRunner.status();
            const auto buildStatus = sceneBuilderRunner.status();
            const auto viewStatus = sceneViewerRunner.status();

            edgevision::app::runtime::ViewerDebugCounters counters{};
            counters.submittedFrameCount = ingestStatus.submittedFrameCount;
            counters.consumedFrameCount = buildStatus.consumedFrameCount;
            counters.publishedOutputCount = viewStatus.publishedOutputCount;
            counters.cachedOutputCount = viewStatus.cachedOutputCount;
            return counters;
        }
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
    std::cout << "Opening K4A device..." << std::endl;
    if (!camera.open(appConfig.capture.camera.deviceIndex)) {
        std::cerr << "Failed to open K4A device" << std::endl;
        cleanup();
        return 1;
    }
    std::cout << "K4A device opened" << std::endl;

    const k4a_device_configuration_t k4aConfig =
        edgevision::capture::makeK4aDeviceConfig(appConfig.capture.camera);
    std::cout << "Starting K4A cameras..." << std::endl;
    if (!camera.start(k4aConfig)) {
        std::cerr << "Failed to start K4A cameras" << std::endl;
        cleanup();
        return 1;
    }
    std::cout << "K4A cameras started" << std::endl;

    std::cout << "Starting frame ingestor runner..." << std::endl;
    if (!frameIngestorRunner.start()) {
        std::cerr << "Failed to start capture frame ingestor" << std::endl;
        cleanup();
        return 1;
    }
    std::cout << "Frame ingestor runner started" << std::endl;

    std::cout << "Starting scene builder runner..." << std::endl;
    if (!sceneBuilderRunner.start()) {
        std::cerr << "Failed to start scene builder runner" << std::endl;
        cleanup();
        return 1;
    }
    std::cout << "Scene builder runner started" << std::endl;

    std::cout << "Starting scene viewer runner..." << std::endl;
    if (!sceneViewerRunner.start()) {
        std::cerr << "Failed to start scene viewer runner" << std::endl;
        cleanup();
        return 1;
    }
    std::cout << "Scene viewer runner started" << std::endl;

    // Seed the initial viewer pose from the first built scene.
    std::cout << "Seeding initial viewer pose..." << std::endl;
    if (!viewerPoseSeeder.seedOnce(kViewerSeedTimeout)) {
        std::cerr << "Timed out waiting to seed the initial viewer pose" << std::endl;
        cleanup();
        return 1;
    }
    const auto seededPose = viewerPoseStore.latest();
    const auto seededSceneVersion = sceneVersionStore.latest();
    if (seededPose.has_value() && seededSceneVersion.has_value()) {
        std::cout << "Initial viewer pose seeded from frame "
                  << seededSceneVersion->lastIntegratedFrameId << " at scene version "
                  << seededSceneVersion->versionId << " with image size "
                  << seededPose->imageSize.width << 'x' << seededPose->imageSize.height
                  << std::endl;
    } else {
        std::cout << "Initial viewer pose seeded" << std::endl;
    }

    if (viewerDebugSession.dumpingEnabled()) {
        std::cout << "Waiting to dump " << appConfig.debug.viewerDump.maxFrames
                  << " viewer frame(s) to " << viewerDebugSession.outputDirectory()
                  << " with metrics in " << viewerDebugSession.logPath() << "..." << std::endl;
    } else {
        std::cout << "Waiting for first non-cached viewer render output..." << std::endl;
    }
    if (!viewerDebugSession.waitForCompletion(kViewerOutputTimeout)) {
        std::cerr << "Timed out waiting for viewer render outputs" << std::endl;
        cleanup();
        return 1;
    }

    if (viewerDebugSession.dumpingEnabled()) {
        std::cout << "Dumped " << viewerDebugSession.dumpedOutputCount() << " viewer frame(s) to "
                  << viewerDebugSession.outputDirectory() << " and wrote metrics to "
                  << viewerDebugSession.logPath() << std::endl;
    } else {
        std::cout << "Observed first non-cached viewer render output" << std::endl;
    }

    cleanup();
    return 0;
}
