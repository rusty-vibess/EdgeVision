#include "config/CommandLineParser.hpp"

#include <cstdio>
#include <string>

namespace {
    using edgevision::config::AppConfig;
    using edgevision::config::parseCommandLine;
    using edgevision::config::SceneReadPolicy;
    using edgevision::config::ViewerLoopPolicy;

    int gFailures = 0;

    void recordFailure(const char* message, const char* file, int line) {
        std::fprintf(stderr, "%s:%d: %s\n", file, line, message);
        ++gFailures;
    }

    void expectTrue(bool value, const char* expr, const char* file, int line) {
        if (!value) {
            recordFailure(expr, file, line);
        }
    }

    template <typename T, typename U>
    void expectEq(
        const T& lhs,
        const U& rhs,
        const char* lhsExpr,
        const char* rhsExpr,
        const char* file,
        int line
    ) {
        if (!(lhs == rhs)) {
            std::fprintf(stderr, "%s:%d: expected %s == %s\n", file, line, lhsExpr, rhsExpr);
            ++gFailures;
        }
    }

#define EXPECT_TRUE(expr) expectTrue((expr), #expr, __FILE__, __LINE__)
#define EXPECT_FALSE(expr) expectTrue(!(expr), #expr, __FILE__, __LINE__)
#define EXPECT_EQ(lhs, rhs) expectEq((lhs), (rhs), #lhs, #rhs, __FILE__, __LINE__)

    void testDefaultsAreUsed() {
        char executable[] = "EdgeVision";
        char* argv[] = {executable};

        const auto result = parseCommandLine(1, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_FALSE(result.config.streaming.tcp.enabled);
        EXPECT_EQ(result.config.streaming.tcp.port, 6688);
        EXPECT_TRUE(result.config.streaming.webrtc.enabled);
        EXPECT_EQ(result.config.streaming.webrtc.signallingPort, std::uint16_t{6689});
        EXPECT_TRUE(result.config.streaming.webrtc.stunServer.empty());
        EXPECT_EQ(result.config.imageSize.width, 1280);
        EXPECT_EQ(result.config.imageSize.height, 720);
        EXPECT_EQ(result.config.scene.readPolicy, SceneReadPolicy::Greedy);
        EXPECT_TRUE(result.config.capture.enabled);
        EXPECT_EQ(result.config.builder.readyFrameTimeoutMs, 50);
        EXPECT_TRUE(result.config.builder.trackerConfig.empty());
        EXPECT_EQ(result.config.viewer.loopPolicy, ViewerLoopPolicy::Event);
        EXPECT_EQ(result.config.viewer.loopPeriodMs, 33);
        EXPECT_EQ(result.config.viewer.outputHistoryCapacity, std::size_t{8});
        EXPECT_FALSE(result.config.debug.viewerDump.enabled);
        EXPECT_EQ(result.config.debug.viewerDump.maxFrames, std::size_t{1});
    }

    void testPortOverride() {
        char executable[] = "EdgeVision";
        char portFlag[] = "--port";
        char portValue[] = "7000";
        char* argv[] = {executable, portFlag, portValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.streaming.tcp.port, 7000);
    }

    void testDisableCaptureFlag() {
        char executable[] = "EdgeVision";
        char captureFlag[] = "--disable-capture";
        char* argv[] = {executable, captureFlag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_FALSE(result.config.capture.enabled);
    }

    void testEnableTcpStreamingFlag() {
        char executable[] = "EdgeVision";
        char tcpFlag[] = "--enable-tcp-streaming";
        char* argv[] = {executable, tcpFlag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_TRUE(result.config.streaming.tcp.enabled);
    }

    void testDisableWebRtcFlag() {
        char executable[] = "EdgeVision";
        char webrtcFlag[] = "--disable-webrtc";
        char* argv[] = {executable, webrtcFlag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_FALSE(result.config.streaming.webrtc.enabled);
    }

    void testWebRtcPortOverride() {
        char executable[] = "EdgeVision";
        char portFlag[] = "--webrtc-port";
        char portValue[] = "6690";
        char* argv[] = {executable, portFlag, portValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.streaming.webrtc.signallingPort, std::uint16_t{6690});
    }

    void testWebRtcStunOverride() {
        char executable[] = "EdgeVision";
        char stunFlag[] = "--webrtc-stun";
        char stunValue[] = "stun://example.invalid:19302";
        char* argv[] = {executable, stunFlag, stunValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.streaming.webrtc.stunServer, std::string{stunValue});
    }

    void testWebRtcStunNoneClearsServer() {
        char executable[] = "EdgeVision";
        char stunFlag[] = "--webrtc-stun";
        char stunValue[] = "none";
        char* argv[] = {executable, stunFlag, stunValue};

        AppConfig defaults{};
        defaults.streaming.webrtc.stunServer = "stun://example.invalid:19302";

        const auto result = parseCommandLine(3, argv, defaults);

        EXPECT_TRUE(result.parsed());
        EXPECT_TRUE(result.config.streaming.webrtc.stunServer.empty());
    }

    void testReadPolicyGreedyOverride() {
        char executable[] = "EdgeVision";
        char readPolicyFlag[] = "--read-policy";
        char readPolicyValue[] = "greedy";
        char* argv[] = {executable, readPolicyFlag, readPolicyValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.scene.readPolicy, SceneReadPolicy::Greedy);
    }

    void testReadPolicyBalancedOverride() {
        char executable[] = "EdgeVision";
        char readPolicyFlag[] = "--read-policy";
        char readPolicyValue[] = "balanced";
        char* argv[] = {executable, readPolicyFlag, readPolicyValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.scene.readPolicy, SceneReadPolicy::Balanced);
    }

    void testViewerPolicyEventOverride() {
        char executable[] = "EdgeVision";
        char viewerPolicyFlag[] = "--viewer-policy";
        char viewerPolicyValue[] = "event";
        char* argv[] = {executable, viewerPolicyFlag, viewerPolicyValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.viewer.loopPolicy, ViewerLoopPolicy::Event);
    }

    void testViewerPolicyHotLoopOverride() {
        char executable[] = "EdgeVision";
        char viewerPolicyFlag[] = "--viewer-policy";
        char viewerPolicyValue[] = "hot-loop";
        char* argv[] = {executable, viewerPolicyFlag, viewerPolicyValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.viewer.loopPolicy, ViewerLoopPolicy::HotLoop);
    }

    void testCustomDefaultsArePreserved() {
        AppConfig defaults{};
        defaults.streaming.tcp.enabled = true;
        defaults.streaming.tcp.port = 9000;
        defaults.streaming.webrtc.enabled = false;
        defaults.streaming.webrtc.signallingPort = 7777;
        defaults.imageSize = edgevision::config::ImageSize{2048, 1536};
        defaults.capture.enabled = false;
        defaults.scene.readPolicy = SceneReadPolicy::Balanced;
        defaults.capture.runtime.captureTimeoutMs = 25;
        defaults.builder.readyFrameTimeoutMs = 7;
        defaults.builder.trackerConfig = "type=forcefail";
        defaults.viewer.loopPolicy = ViewerLoopPolicy::HotLoop;
        defaults.viewer.loopPeriodMs = 17;
        defaults.viewer.outputHistoryCapacity = 3;
        defaults.debug.viewerDump.enabled = true;
        defaults.debug.viewerDump.maxFrames = 5;

        char executable[] = "EdgeVision";
        char* argv[] = {executable};

        const auto result = parseCommandLine(1, argv, defaults);

        EXPECT_TRUE(result.parsed());
        EXPECT_TRUE(result.config.streaming.tcp.enabled);
        EXPECT_EQ(result.config.streaming.tcp.port, 9000);
        EXPECT_FALSE(result.config.streaming.webrtc.enabled);
        EXPECT_EQ(result.config.streaming.webrtc.signallingPort, std::uint16_t{7777});
        EXPECT_EQ(result.config.imageSize, defaults.imageSize);
        EXPECT_EQ(result.config.scene.readPolicy, SceneReadPolicy::Balanced);
        EXPECT_FALSE(result.config.capture.enabled);
        EXPECT_EQ(result.config.capture.runtime.captureTimeoutMs, 25);
        EXPECT_EQ(result.config.builder.readyFrameTimeoutMs, 7);
        EXPECT_EQ(result.config.builder.trackerConfig, defaults.builder.trackerConfig);
        EXPECT_EQ(result.config.viewer.loopPolicy, ViewerLoopPolicy::HotLoop);
        EXPECT_EQ(result.config.viewer.loopPeriodMs, 17);
        EXPECT_EQ(result.config.viewer.outputHistoryCapacity, std::size_t{3});
        EXPECT_TRUE(result.config.debug.viewerDump.enabled);
        EXPECT_EQ(result.config.debug.viewerDump.maxFrames, std::size_t{5});
    }

    void testEnableDebugFlagUsesDefaultViewerDumpCount() {
        char executable[] = "EdgeVision";
        char debugFlag[] = "--enable-debug";
        char* argv[] = {executable, debugFlag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_TRUE(result.config.debug.viewerDump.enabled);
        EXPECT_EQ(result.config.debug.viewerDump.maxFrames, std::size_t{5});
    }

    void testDebugFramesOverride() {
        char executable[] = "EdgeVision";
        char dumpFlag[] = "--debug-frames";
        char dumpValue[] = "10";
        char* argv[] = {executable, dumpFlag, dumpValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_TRUE(result.config.debug.viewerDump.enabled);
        EXPECT_EQ(result.config.debug.viewerDump.maxFrames, std::size_t{10});
    }

    void testExplicitDebugFramesBeatsEnableDebug() {
        char executable[] = "EdgeVision";
        char dumpFlag[] = "--debug-frames";
        char dumpValue[] = "10";
        char debugFlag[] = "--enable-debug";
        char* argv[] = {executable, dumpFlag, dumpValue, debugFlag};

        const auto result = parseCommandLine(4, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_TRUE(result.config.debug.viewerDump.enabled);
        EXPECT_EQ(result.config.debug.viewerDump.maxFrames, std::size_t{10});
    }

    void testInvalidPortFails() {
        char executable[] = "EdgeVision";
        char portFlag[] = "--port";
        char portValue[] = "99999";
        char* argv[] = {executable, portFlag, portValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testInvalidWebRtcPortFails() {
        char executable[] = "EdgeVision";
        char portFlag[] = "--webrtc-port";
        char portValue[] = "99999";
        char* argv[] = {executable, portFlag, portValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testMissingWebRtcStunValueFails() {
        char executable[] = "EdgeVision";
        char stunFlag[] = "--webrtc-stun";
        char* argv[] = {executable, stunFlag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testMissingReadPolicyValueFails() {
        char executable[] = "EdgeVision";
        char readPolicyFlag[] = "--read-policy";
        char* argv[] = {executable, readPolicyFlag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testInvalidReadPolicyFails() {
        char executable[] = "EdgeVision";
        char readPolicyFlag[] = "--read-policy";
        char readPolicyValue[] = "unknown";
        char* argv[] = {executable, readPolicyFlag, readPolicyValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testMissingViewerPolicyValueFails() {
        char executable[] = "EdgeVision";
        char viewerPolicyFlag[] = "--viewer-policy";
        char* argv[] = {executable, viewerPolicyFlag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testInvalidViewerPolicyFails() {
        char executable[] = "EdgeVision";
        char viewerPolicyFlag[] = "--viewer-policy";
        char viewerPolicyValue[] = "live";
        char* argv[] = {executable, viewerPolicyFlag, viewerPolicyValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testMissingDebugFramesValueFails() {
        char executable[] = "EdgeVision";
        char dumpFlag[] = "--debug-frames";
        char* argv[] = {executable, dumpFlag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testInvalidDebugFramesValueFails() {
        char executable[] = "EdgeVision";
        char dumpFlag[] = "--debug-frames";
        char dumpValue[] = "0";
        char* argv[] = {executable, dumpFlag, dumpValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testLegacyDumpViewerFramesFlagFails() {
        char executable[] = "EdgeVision";
        char dumpFlag[] = "--dump-viewer-frames";
        char dumpValue[] = "10";
        char* argv[] = {executable, dumpFlag, dumpValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_FALSE(result.parsed());
    }

    void testUnknownArgumentFails() {
        char executable[] = "EdgeVision";
        char flag[] = "--capture-device";
        char* argv[] = {executable, flag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_FALSE(result.parsed());
    }
} // namespace

int main() {
    testDefaultsAreUsed();
    testPortOverride();
    testDisableCaptureFlag();
    testEnableTcpStreamingFlag();
    testDisableWebRtcFlag();
    testWebRtcPortOverride();
    testWebRtcStunOverride();
    testWebRtcStunNoneClearsServer();
    testReadPolicyGreedyOverride();
    testReadPolicyBalancedOverride();
    testViewerPolicyEventOverride();
    testViewerPolicyHotLoopOverride();
    testCustomDefaultsArePreserved();
    testEnableDebugFlagUsesDefaultViewerDumpCount();
    testDebugFramesOverride();
    testExplicitDebugFramesBeatsEnableDebug();
    testInvalidPortFails();
    testInvalidWebRtcPortFails();
    testMissingWebRtcStunValueFails();
    testMissingReadPolicyValueFails();
    testInvalidReadPolicyFails();
    testMissingViewerPolicyValueFails();
    testInvalidViewerPolicyFails();
    testMissingDebugFramesValueFails();
    testInvalidDebugFramesValueFails();
    testLegacyDumpViewerFramesFlagFails();
    testUnknownArgumentFails();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
