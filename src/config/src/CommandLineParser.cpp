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

    } // namespace

    bool CommandLineParseResult::parsed() const {
        return error.empty();
    }

    CommandLineParseResult parseCommandLine(int argc, char* argv[], const AppConfig& defaults) {
        CommandLineParseResult result{};
        result.config = defaults;

        for (int i = 1; i < argc; ++i) {
            const std::string_view arg(argv[i]);

            if (arg == "--enable-capture") {
                result.config.capture.enabled = true;
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

                result.config.render.port = port;
                ++i;
                continue;
            }

            result.error = "Unknown argument: " + std::string(arg);
            return result;
        }

        return result;
    }

} // namespace edgevision::config
