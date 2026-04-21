#include "config/CommandLineParser.hpp"

#include <cstdio>

namespace {
    using edgevision::config::AppConfig;
    using edgevision::config::parseCommandLine;

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
        EXPECT_EQ(result.config.render.port, 6688);
        EXPECT_FALSE(result.config.capture.enabled);
    }

    void testPortOverride() {
        char executable[] = "EdgeVision";
        char portFlag[] = "--port";
        char portValue[] = "7000";
        char* argv[] = {executable, portFlag, portValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.render.port, 7000);
    }

    void testEnableCaptureFlag() {
        char executable[] = "EdgeVision";
        char captureFlag[] = "--enable-capture";
        char* argv[] = {executable, captureFlag};

        const auto result = parseCommandLine(2, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_TRUE(result.config.capture.enabled);
    }

    void testCustomDefaultsArePreserved() {
        AppConfig defaults{};
        defaults.render.port = 9000;
        defaults.capture.runtime.captureTimeoutMs = 25;

        char executable[] = "EdgeVision";
        char captureFlag[] = "--enable-capture";
        char* argv[] = {executable, captureFlag};

        const auto result = parseCommandLine(2, argv, defaults);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.render.port, 9000);
        EXPECT_TRUE(result.config.capture.enabled);
        EXPECT_EQ(result.config.capture.runtime.captureTimeoutMs, 25);
    }

    void testInvalidPortFails() {
        char executable[] = "EdgeVision";
        char portFlag[] = "--port";
        char portValue[] = "99999";
        char* argv[] = {executable, portFlag, portValue};

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
    testEnableCaptureFlag();
    testCustomDefaultsArePreserved();
    testInvalidPortFails();
    testUnknownArgumentFails();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
