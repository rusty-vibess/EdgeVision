#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <functional>

#include "app/runtime/ViewerFrameDumper.hpp"
#include "config/types/debug.hpp"

namespace edgevision::model::viewer {
    class RenderOutputStore;
} // namespace edgevision::model::viewer

namespace edgevision::app::runtime {

    struct ViewerDebugCounters {
        std::size_t submittedFrameCount = 0;
        std::size_t consumedFrameCount = 0;
        std::size_t publishedOutputCount = 0;
        std::size_t cachedOutputCount = 0;
    };

    struct ViewerDebugRates {
        double ingestFps = 0.0;
        double buildFps = 0.0;
        double viewFps = 0.0;
        double uncachedViewFps = 0.0;
        double cachedViewFps = 0.0;
    };

    using ViewerDebugCounterSampler = std::function<ViewerDebugCounters()>;

    [[nodiscard]] ViewerDebugRates computeViewerDebugRates(
        const ViewerDebugCounters& begin,
        const ViewerDebugCounters& end,
        std::chrono::duration<double> elapsed
    );

    class ViewerDebugSession final {
      public:
        ViewerDebugSession(
            const model::viewer::RenderOutputStore& renderOutputStore,
            const config::ViewerDumpConfig& config,
            ViewerDebugCounterSampler counterSampler = {},
            std::filesystem::path outputDirectory = defaultViewerDumpDirectory()
        );

        [[nodiscard]] bool waitForCompletion(std::chrono::milliseconds timeout);
        [[nodiscard]] bool dumpingEnabled() const;
        [[nodiscard]] std::size_t dumpedOutputCount() const;
        [[nodiscard]] std::size_t skippedDuplicateCount() const;
        [[nodiscard]] const std::filesystem::path& outputDirectory() const;
        [[nodiscard]] const std::filesystem::path& logPath() const;

      private:
        void maybeLogSample();
        void writeFinalSummary();
        [[nodiscard]] ViewerDebugCounters sampleCounters() const;

        ViewerFrameDumper m_frameDumper;
        ViewerDebugCounterSampler m_counterSampler{};
        std::filesystem::path m_logPath{};
        std::chrono::steady_clock::time_point m_startedAt{};
        std::chrono::steady_clock::time_point m_lastReportedAt{};
        ViewerDebugCounters m_startedCounters{};
        ViewerDebugCounters m_lastReportedCounters{};
        std::ofstream m_logStream{};
        bool m_summaryWritten = false;
    };

} // namespace edgevision::app::runtime
