#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>

#include "config/types/debug.hpp"
#include "model/viewer/types/output.hpp"

namespace edgevision::model::viewer {
    class RenderOutputStore;
} // namespace edgevision::model::viewer

namespace edgevision::app::runtime {

    [[nodiscard]] std::filesystem::path defaultViewerDumpDirectory();

    class ViewerFrameDumper final {
      public:
        ViewerFrameDumper(
            const model::viewer::RenderOutputStore& renderOutputStore,
            const config::ViewerDumpConfig& config = {},
            std::filesystem::path outputDirectory = defaultViewerDumpDirectory()
        );

        [[nodiscard]] bool waitForConfiguredOutputs(
            std::chrono::milliseconds timeout,
            const std::function<void()>& onTick = {}
        );
        [[nodiscard]] bool dumpingEnabled() const;
        [[nodiscard]] std::size_t dumpedOutputCount() const;
        [[nodiscard]] std::size_t skippedDuplicateCount() const;
        [[nodiscard]] const std::filesystem::path& outputDirectory() const;

      private:
        [[nodiscard]] bool processNewOutputs();
        [[nodiscard]] bool handleOutput(const edgevision::model::viewer::RenderOutput& output);
        [[nodiscard]] std::uint64_t hashOutput(
            const edgevision::model::viewer::RenderOutput& output
        ) const;
        [[nodiscard]] bool writeOutput(
            const edgevision::model::viewer::RenderOutput& output,
            std::size_t sequence
        ) const;

        const model::viewer::RenderOutputStore& m_renderOutputStore;
        config::ViewerDumpConfig m_config{};
        std::filesystem::path m_outputDirectory{};
        std::chrono::steady_clock::time_point m_lastProcessedTimestamp{};
        std::size_t m_dumpedOutputCount = 0;
        std::size_t m_skippedDuplicateCount = 0;
        std::uint64_t m_lastDumpedHash = 0;
        bool m_hasLastDumpedHash = false;
        bool m_observedNonCachedOutput = false;
    };

} // namespace edgevision::app::runtime
