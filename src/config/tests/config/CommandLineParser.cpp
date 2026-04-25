#include "config/CommandLineParser.hpp"

#include <cstdio>

namespace {
    using edgevision::config::AppConfig;
    using edgevision::config::parseCommandLine;
    using edgevision::config::ReadPolicy;

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
        EXPECT_EQ(result.config.render.readPolicy, ReadPolicy::Greedy);
        EXPECT_FALSE(result.config.capture.enabled);
        EXPECT_EQ(result.config.builder.readyFrameTimeoutMs, 50);
        EXPECT_TRUE(result.config.builder.trackerConfig.empty());
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

    void testReadPolicyGreedyOverride() {
        char executable[] = "EdgeVision";
        char readPolicyFlag[] = "--read-policy";
        char readPolicyValue[] = "greedy";
        char* argv[] = {executable, readPolicyFlag, readPolicyValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.render.readPolicy, ReadPolicy::Greedy);
    }

    void testReadPolicyBalancedOverride() {
        char executable[] = "EdgeVision";
        char readPolicyFlag[] = "--read-policy";
        char readPolicyValue[] = "balanced";
        char* argv[] = {executable, readPolicyFlag, readPolicyValue};

        const auto result = parseCommandLine(3, argv);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.render.readPolicy, ReadPolicy::Balanced);
    }

    void testCustomDefaultsArePreserved() {
        AppConfig defaults{};
        defaults.render.port = 9000;
        defaults.render.readPolicy = ReadPolicy::Balanced;
        defaults.capture.runtime.captureTimeoutMs = 25;
        defaults.builder.readyFrameTimeoutMs = 7;
        defaults.builder.trackerConfig = "type=forcefail";

        char executable[] = "EdgeVision";
        char captureFlag[] = "--enable-capture";
        char* argv[] = {executable, captureFlag};

        const auto result = parseCommandLine(2, argv, defaults);

        EXPECT_TRUE(result.parsed());
        EXPECT_EQ(result.config.render.port, 9000);
        EXPECT_EQ(result.config.render.readPolicy, ReadPolicy::Balanced);
        EXPECT_TRUE(result.config.capture.enabled);
        EXPECT_EQ(result.config.capture.runtime.captureTimeoutMs, 25);
        EXPECT_EQ(result.config.builder.readyFrameTimeoutMs, 7);
        EXPECT_EQ(result.config.builder.trackerConfig, defaults.builder.trackerConfig);
    }

    void testInvalidPortFails() {
        char executable[] = "EdgeVision";
        char portFlag[] = "--port";
        char portValue[] = "99999";
        char* argv[] = {executable, portFlag, portValue};

        const auto result = parseCommandLine(3, argv);

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
    testReadPolicyGreedyOverride();
    testReadPolicyBalancedOverride();
    testCustomDefaultsArePreserved();
    testInvalidPortFails();
    testMissingReadPolicyValueFails();
    testInvalidReadPolicyFails();
    testUnknownArgumentFails();

    if (gFailures != 0) {
        std::fprintf(stderr, "Tests failed: %d\n", gFailures);
        return 1;
    }

    std::fprintf(stdout, "All tests passed\n");
    return 0;
}
