#include "app/runtime/ViewerFrameDumper.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <source_location>
#include <string>
#include <string_view>
#include <vector>

#include "model/viewer/RenderOutputStore.hpp"

namespace {
    using edgevision::app::runtime::ViewerFrameDumper;
    using edgevision::config::ViewerDumpConfig;
    using edgevision::model::frame::FrameBuffer;
    using edgevision::model::frame::ImageSize;
    using edgevision::model::scene::SceneVersionId;
    using edgevision::model::viewer::RenderOutput;
    using edgevision::model::viewer::RenderOutputStore;
    using edgevision::model::viewer::ViewerPoseGeneration;

    int gFailures = 0;

    void recordFailure(
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        std::cerr << location.file_name() << ':' << location.line() << ": " << message << '\n';
        ++gFailures;
    }

    void expectTrue(
        bool value,
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        if (!value) {
            recordFailure(message, location);
        }
    }

    template <typename T, typename U>
    void expectEq(
        const T& lhs,
        const U& rhs,
        std::string_view message,
        const std::source_location& location = std::source_location::current()
    ) {
        if (!(lhs == rhs)) {
            recordFailure(message, location);
        }
    }

    FrameBuffer makeBuffer(std::uint8_t seed) {
        auto storage = std::make_shared<std::vector<std::byte>>(12);
        for (std::size_t index = 0; index < storage->size(); ++index) {
            (*storage)[index] = static_cast<std::byte>(seed + index);
        }

        return FrameBuffer(
            storage->data(), storage->size(), std::shared_ptr<const void>(storage, storage.get())
        );
    }

    RenderOutput makeOutput(
        ViewerPoseGeneration poseGeneration,
        SceneVersionId sceneVersionId,
        bool cached,
        std::int64_t timestampTicks
    ) {
        RenderOutput output{};
        output.rgb = makeBuffer(static_cast<std::uint8_t>(poseGeneration));
        output.imageSize = ImageSize{2, 2};
        output.poseGeneration = poseGeneration;
        output.sceneVersionId = sceneVersionId;
        output.renderTimestamp =
            std::chrono::steady_clock::time_point(std::chrono::milliseconds(timestampTicks));
        output.cached = cached;
        return output;
    }

    std::filesystem::path makeTempDirectory(const char* leafName) {
        return std::filesystem::temp_directory_path() / leafName;
    }

    std::size_t countFiles(const std::filesystem::path& directory) {
        if (!std::filesystem::exists(directory)) {
            return 0;
        }

        std::size_t count = 0;
        for ([[maybe_unused]] const auto& entry : std::filesystem::directory_iterator(directory)) {
            ++count;
        }

        return count;
    }

    void testDisabledDumperWaitsForFirstNonCachedOutputWithoutWritingFiles() {
        const std::filesystem::path outputDirectory =
            makeTempDirectory("edgevision-viewer-frame-dumper-disabled");
        std::filesystem::remove_all(outputDirectory);

        RenderOutputStore store{4};
        store.publish(makeOutput(1, 10, true, 1));
        store.publish(makeOutput(2, 11, false, 2));

        ViewerDumpConfig config{};
        config.enabled = false;
        ViewerFrameDumper dumper(store, config, outputDirectory);

        expectTrue(
            dumper.waitForConfiguredOutputs(std::chrono::milliseconds(10)),
            "disabled dumper should still observe the first non-cached output"
        );
        expectEq(
            dumper.dumpedOutputCount(),
            std::size_t{0},
            "disabled dumper should not write any frames"
        );
        expectEq(
            countFiles(outputDirectory),
            std::size_t{0},
            "disabled dumper should not create output files"
        );
    }

    void testEnabledDumperWritesFirstOutputsIncludingCachedRepeats() {
        const std::filesystem::path outputDirectory =
            makeTempDirectory("edgevision-viewer-frame-dumper-enabled");
        std::filesystem::remove_all(outputDirectory);

        RenderOutputStore store{6};
        store.publish(makeOutput(1, 10, false, 1));
        store.publish(makeOutput(1, 10, true, 2));
        store.publish(makeOutput(2, 11, false, 3));

        ViewerDumpConfig config{};
        config.enabled = true;
        config.maxFrames = 2;
        ViewerFrameDumper dumper(store, config, outputDirectory);

        expectTrue(
            dumper.waitForConfiguredOutputs(std::chrono::milliseconds(10)),
            "enabled dumper should complete once the requested number of outputs is dumped"
        );
        expectEq(
            dumper.dumpedOutputCount(),
            std::size_t{2},
            "enabled dumper should write the requested number of outputs"
        );
        expectEq(
            dumper.skippedDuplicateCount(),
            std::size_t{1},
            "enabled dumper should skip exact duplicate outputs"
        );
        expectEq(
            countFiles(outputDirectory),
            std::size_t{2},
            "enabled dumper should create one file per dumped output"
        );

        const std::filesystem::path firstOutput = outputDirectory / "viewer-output-0001.ppm";
        const std::filesystem::path secondOutput = outputDirectory / "viewer-output-0002.ppm";
        expectTrue(
            std::filesystem::exists(firstOutput),
            "enabled dumper should create sequential ppm files"
        );
        expectTrue(
            std::filesystem::exists(secondOutput),
            "enabled dumper should continue dumping once a later distinct output arrives"
        );

        std::ifstream stream(firstOutput, std::ios::binary);
        std::string header{};
        std::getline(stream, header);
        expectEq(header, std::string("P6"), "dumped files should use binary ppm encoding");
    }
} // namespace

int main() {
    testDisabledDumperWaitsForFirstNonCachedOutputWithoutWritingFiles();
    testEnabledDumperWritesFirstOutputsIncludingCachedRepeats();

    if (gFailures != 0) {
        std::cerr << "Tests failed: " << gFailures << '\n';
        return 1;
    }

    std::cout << "All tests passed\n";
    return 0;
}
