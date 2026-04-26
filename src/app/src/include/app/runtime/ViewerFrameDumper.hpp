#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>

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

        [[nodiscard]] bool waitForConfiguredOutputs(std::chrono::milliseconds timeout);
        [[nodiscard]] bool dumpingEnabled() const;
        [[nodiscard]] std::size_t dumpedFreshOutputCount() const;
        [[nodiscard]] const std::filesystem::path& outputDirectory() const;

      private:
        [[nodiscard]] bool processNewOutputs();
        [[nodiscard]] bool handleFreshOutput(
            const edgevision::model::viewer::RenderOutput& output
        );
        [[nodiscard]] bool writeOutput(
            const edgevision::model::viewer::RenderOutput& output,
            std::size_t sequence
        ) const;

        const model::viewer::RenderOutputStore& m_renderOutputStore;
        config::ViewerDumpConfig m_config{};
        std::filesystem::path m_outputDirectory{};
        std::chrono::steady_clock::time_point m_lastProcessedTimestamp{};
        std::size_t m_dumpedFreshOutputCount = 0;
        bool m_observedFreshOutput = false;
    };

} // namespace edgevision::app::runtime
