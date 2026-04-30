#include "config/CommandLineParser.hpp"

#include <charconv>
#include <string_view>
#include <system_error>

namespace edgevision::config {
    namespace {

        [[nodiscard]] bool parsePort(std::string_view value, int& port) {
            const char* begin = value.data();
            const char* end = begin + value.size();
            int parsedPort = 0;
            const std::from_chars_result result = std::from_chars(begin, end, parsedPort);
            if (result.ec != std::errc{} || result.ptr != end) {
                return false;
            }

            if (parsedPort <= 0 || parsedPort > 65535) {
                return false;
            }

            port = parsedPort;
            return true;
        }

        [[nodiscard]] bool parseReadPolicy(std::string_view value, SceneReadPolicy& readPolicy) {
            if (value == "greedy") {
                readPolicy = SceneReadPolicy::Greedy;
                return true;
            }

            if (value == "balanced") {
                readPolicy = SceneReadPolicy::Balanced;
                return true;
            }

            return false;
        }

        [[nodiscard]] bool parseViewerPolicy(
            std::string_view value,
            ViewerLoopPolicy& viewerLoopPolicy
        ) {
            if (value == "event") {
                viewerLoopPolicy = ViewerLoopPolicy::Event;
                return true;
            }

            if (value == "hot-loop") {
                viewerLoopPolicy = ViewerLoopPolicy::HotLoop;
                return true;
            }

            return false;
        }

        [[nodiscard]] bool parsePositiveCount(std::string_view value, std::size_t& count) {
            const char* begin = value.data();
            const char* end = begin + value.size();
            std::size_t parsedCount = 0;
            const std::from_chars_result result = std::from_chars(begin, end, parsedCount);
            if (result.ec != std::errc{} || result.ptr != end || parsedCount == 0) {
                return false;
            }

            count = parsedCount;
            return true;
        }

    } // namespace

    bool CommandLineParseResult::parsed() const {
        return error.empty();
    }

    CommandLineParseResult parseCommandLine(int argc, char* argv[], const AppConfig& defaults) {
        CommandLineParseResult result{};
        result.config = defaults;
        bool viewerDumpCountExplicit = false;

        for (int i = 1; i < argc; ++i) {
            const std::string_view arg(argv[i]);

            if (arg == "--disable-capture") {
                result.config.capture.enabled = false;
                continue;
            }

            if (arg == "--enable-debug") {
                result.config.debug.viewerDump.enabled = true;
                if (!viewerDumpCountExplicit) {
                    result.config.debug.viewerDump.maxFrames = 5;
                }

                continue;
            }

            if (arg == "--port") {
                if (i + 1 >= argc) {
                    result.error = "Missing value for --port";
                    return result;
                }

                int port = 0;
                if (!parsePort(argv[i + 1], port)) {
                    result.error = "Invalid port: " + std::string(argv[i + 1]);
                    return result;
                }

                result.config.streaming.tcp.port = port;
                ++i;
                continue;
            }

            if (arg == "--enable-tcp-streaming") {
                result.config.streaming.tcp.enabled = true;
                continue;
            }

            if (arg == "--disable-webrtc") {
                result.config.streaming.webrtc.enabled = false;
                continue;
            }

            if (arg == "--webrtc-port") {
                if (i + 1 >= argc) {
                    result.error = "Missing value for --webrtc-port";
                    return result;
                }

                int port = 0;
                if (!parsePort(argv[i + 1], port)) {
                    result.error = "Invalid WebRTC port: " + std::string(argv[i + 1]);
                    return result;
                }

                result.config.streaming.webrtc.signallingPort = static_cast<std::uint16_t>(port);
                ++i;
                continue;
            }

            if (arg == "--webrtc-stun") {
                if (i + 1 >= argc) {
                    result.error = "Missing value for --webrtc-stun";
                    return result;
                }

                const std::string_view stunValue(argv[i + 1]);
                result.config.streaming.webrtc.stunServer =
                    stunValue == "none" ? std::string{} : std::string{stunValue};
                ++i;
                continue;
            }

            if (arg == "--read-policy") {
                if (i + 1 >= argc) {
                    result.error = "Missing value for --read-policy";
                    return result;
                }

                SceneReadPolicy readPolicy = result.config.scene.readPolicy;
                if (!parseReadPolicy(argv[i + 1], readPolicy)) {
                    result.error = "Invalid read policy: " + std::string(argv[i + 1]);
                    return result;
                }

                result.config.scene.readPolicy = readPolicy;
                ++i;
                continue;
            }

            if (arg == "--viewer-policy") {
                if (i + 1 >= argc) {
                    result.error = "Missing value for --viewer-policy";
                    return result;
                }

                ViewerLoopPolicy viewerLoopPolicy = result.config.viewer.loopPolicy;
                if (!parseViewerPolicy(argv[i + 1], viewerLoopPolicy)) {
                    result.error = "Invalid viewer policy: " + std::string(argv[i + 1]);
                    return result;
                }

                result.config.viewer.loopPolicy = viewerLoopPolicy;
                ++i;
                continue;
            }

            if (arg == "--debug-frames") {
                if (i + 1 >= argc) {
                    result.error = "Missing value for --debug-frames";
                    return result;
                }

                std::size_t frameCount = 0;
                if (!parsePositiveCount(argv[i + 1], frameCount)) {
                    result.error = "Invalid viewer dump frame count: " + std::string(argv[i + 1]);
                    return result;
                }

                result.config.debug.viewerDump.enabled = true;
                result.config.debug.viewerDump.maxFrames = frameCount;
                viewerDumpCountExplicit = true;
                ++i;
                continue;
            }

            result.error = "Unknown argument: " + std::string(arg);
            return result;
        }

        return result;
    }

} // namespace edgevision::config
