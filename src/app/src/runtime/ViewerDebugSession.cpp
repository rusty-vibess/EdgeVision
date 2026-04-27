#include "app/runtime/ViewerDebugSession.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <utility>

namespace edgevision::app::runtime {
    namespace {
        constexpr auto kReportPeriod = std::chrono::seconds(1);

        [[nodiscard]] std::size_t clampedDelta(std::size_t end, std::size_t begin) {
            return end >= begin ? end - begin : 0;
        }

        [[nodiscard]] std::size_t uncachedOutputCount(const ViewerDebugCounters& counters) {
            return counters.publishedOutputCount >= counters.cachedOutputCount
                ? counters.publishedOutputCount - counters.cachedOutputCount
                : 0;
        }

        void writeRates(
            std::ostream& stream,
            const char* label,
            const ViewerDebugRates& rates,
            std::size_t skippedDuplicateCount
        ) {
            stream << label << " ingest=" << std::fixed << std::setprecision(1) << rates.ingestFps
                   << "fps build=" << rates.buildFps << "fps view=" << rates.viewFps
                   << "fps uncached=" << rates.uncachedViewFps
                   << "fps cached=" << rates.cachedViewFps
                   << "fps skipped_duplicates=" << skippedDuplicateCount << '\n';
        }
    } // namespace

    ViewerDebugRates computeViewerDebugRates(
        const ViewerDebugCounters& begin,
        const ViewerDebugCounters& end,
        std::chrono::duration<double> elapsed
    ) {
        const double elapsedSeconds = elapsed.count();
        if (elapsedSeconds <= 0.0) {
            return {};
        }

        const std::size_t submittedDelta =
            clampedDelta(end.submittedFrameCount, begin.submittedFrameCount);
        const std::size_t consumedDelta =
            clampedDelta(end.consumedFrameCount, begin.consumedFrameCount);
        const std::size_t publishedDelta =
            clampedDelta(end.publishedOutputCount, begin.publishedOutputCount);
        const std::size_t cachedDelta =
            clampedDelta(end.cachedOutputCount, begin.cachedOutputCount);
        const std::size_t uncachedDelta =
            publishedDelta >= cachedDelta ? publishedDelta - cachedDelta : 0;

        ViewerDebugRates rates{};
        rates.ingestFps = static_cast<double>(submittedDelta) / elapsedSeconds;
        rates.buildFps = static_cast<double>(consumedDelta) / elapsedSeconds;
        rates.viewFps = static_cast<double>(publishedDelta) / elapsedSeconds;
        rates.uncachedViewFps = static_cast<double>(uncachedDelta) / elapsedSeconds;
        rates.cachedViewFps = static_cast<double>(cachedDelta) / elapsedSeconds;
        return rates;
    }

    ViewerDebugSession::ViewerDebugSession(
        const edgevision::model::viewer::RenderOutputStore& renderOutputStore,
        const edgevision::config::ViewerDumpConfig& config,
        ViewerDebugCounterSampler counterSampler,
        std::filesystem::path outputDirectory
    )
        : m_frameDumper(renderOutputStore, config, std::move(outputDirectory)),
          m_counterSampler(std::move(counterSampler)),
          m_logPath(m_frameDumper.outputDirectory() / "viewer-debug.log"),
          m_startedAt(std::chrono::steady_clock::now()),
          m_lastReportedAt(m_startedAt),
          m_startedCounters(sampleCounters()),
          m_lastReportedCounters(m_startedCounters) {
        if (!m_frameDumper.dumpingEnabled()) {
            return;
        }

        std::error_code error{};
        std::filesystem::create_directories(m_frameDumper.outputDirectory(), error);
        if (!error) {
            m_logStream.open(m_logPath, std::ios::out | std::ios::trunc);
        }
    }

    bool ViewerDebugSession::waitForCompletion(std::chrono::milliseconds timeout) {
        const bool completed =
            m_frameDumper.waitForConfiguredOutputs(timeout, [this]() { maybeLogSample(); });
        writeFinalSummary();
        return completed;
    }

    bool ViewerDebugSession::dumpingEnabled() const {
        return m_frameDumper.dumpingEnabled();
    }

    std::size_t ViewerDebugSession::dumpedOutputCount() const {
        return m_frameDumper.dumpedOutputCount();
    }

    std::size_t ViewerDebugSession::skippedDuplicateCount() const {
        return m_frameDumper.skippedDuplicateCount();
    }

    const std::filesystem::path& ViewerDebugSession::outputDirectory() const {
        return m_frameDumper.outputDirectory();
    }

    const std::filesystem::path& ViewerDebugSession::logPath() const {
        return m_logPath;
    }

    void ViewerDebugSession::maybeLogSample() {
        if (!m_logStream.is_open() || !m_counterSampler) {
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        if (now - m_lastReportedAt < kReportPeriod) {
            return;
        }

        const ViewerDebugCounters counters = sampleCounters();
        const ViewerDebugRates rates =
            computeViewerDebugRates(m_lastReportedCounters, counters, now - m_lastReportedAt);
        writeRates(m_logStream, "metrics:", rates, m_frameDumper.skippedDuplicateCount());
        m_logStream.flush();

        m_lastReportedAt = now;
        m_lastReportedCounters = counters;
    }

    void ViewerDebugSession::writeFinalSummary() {
        if (m_summaryWritten || !m_logStream.is_open()) {
            m_summaryWritten = true;
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        const ViewerDebugCounters counters = sampleCounters();
        const ViewerDebugRates rates =
            computeViewerDebugRates(m_startedCounters, counters, now - m_startedAt);
        ViewerDebugCounters totals{};
        totals.submittedFrameCount =
            clampedDelta(counters.submittedFrameCount, m_startedCounters.submittedFrameCount);
        totals.consumedFrameCount =
            clampedDelta(counters.consumedFrameCount, m_startedCounters.consumedFrameCount);
        totals.publishedOutputCount =
            clampedDelta(counters.publishedOutputCount, m_startedCounters.publishedOutputCount);
        totals.cachedOutputCount =
            clampedDelta(counters.cachedOutputCount, m_startedCounters.cachedOutputCount);

        const double elapsedSeconds =
            std::chrono::duration_cast<std::chrono::duration<double>>(now - m_startedAt).count();
        m_logStream << "summary: elapsed=" << std::fixed << std::setprecision(1) << elapsedSeconds
                    << "s submitted=" << totals.submittedFrameCount
                    << " consumed=" << totals.consumedFrameCount
                    << " published=" << totals.publishedOutputCount
                    << " uncached=" << uncachedOutputCount(totals)
                    << " cached=" << totals.cachedOutputCount
                    << " skipped_duplicates=" << m_frameDumper.skippedDuplicateCount() << '\n';
        writeRates(m_logStream, "average:", rates, m_frameDumper.skippedDuplicateCount());
        m_logStream.flush();
        m_summaryWritten = true;
    }

    ViewerDebugCounters ViewerDebugSession::sampleCounters() const {
        return m_counterSampler ? m_counterSampler() : ViewerDebugCounters{};
    }

} // namespace edgevision::app::runtime
