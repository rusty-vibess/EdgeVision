#include "app/runtime/ViewerFrameDumper.hpp"

#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include "model/viewer/RenderOutputStore.hpp"

namespace edgevision::app::runtime {
    namespace {
        constexpr auto kPollInterval = std::chrono::milliseconds(10);

        [[nodiscard]] std::filesystem::path makeOutputPath(
            const std::filesystem::path& outputDirectory,
            std::size_t sequence
        ) {
            std::ostringstream filename;
            filename << "viewer-output-" << std::setw(4) << std::setfill('0') << sequence
                     << ".ppm";
            return outputDirectory / filename.str();
        }
    } // namespace

    std::filesystem::path defaultViewerDumpDirectory() {
        return std::filesystem::temp_directory_path() / "edgevision-viewer-dumps";
    }

    ViewerFrameDumper::ViewerFrameDumper(
        const edgevision::model::viewer::RenderOutputStore& renderOutputStore,
        const edgevision::config::ViewerDumpConfig& config,
        std::filesystem::path outputDirectory
    )
        : m_renderOutputStore(renderOutputStore),
          m_config(config),
          m_outputDirectory(std::move(outputDirectory)) {
        if (m_config.maxFreshOutputs == 0) {
            m_config.maxFreshOutputs = 1;
        }
    }

    bool ViewerFrameDumper::waitForConfiguredOutputs(std::chrono::milliseconds timeout) {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline) {
            if (!processNewOutputs()) {
                return false;
            }

            if (!m_observedFreshOutput) {
                std::this_thread::sleep_for(kPollInterval);
                continue;
            }

            if (!m_config.enabled) {
                return true;
            }

            if (m_dumpedFreshOutputCount >= m_config.maxFreshOutputs) {
                return true;
            }

            std::this_thread::sleep_for(kPollInterval);
        }

        return false;
    }

    bool ViewerFrameDumper::dumpingEnabled() const {
        return m_config.enabled;
    }

    std::size_t ViewerFrameDumper::dumpedFreshOutputCount() const {
        return m_dumpedFreshOutputCount;
    }

    const std::filesystem::path& ViewerFrameDumper::outputDirectory() const {
        return m_outputDirectory;
    }

    bool ViewerFrameDumper::processNewOutputs() {
        const std::vector<edgevision::model::viewer::RenderOutput> outputs =
            m_renderOutputStore.recent(std::numeric_limits<std::size_t>::max());
        for (const edgevision::model::viewer::RenderOutput& output : outputs) {
            if (output.renderTimestamp <= m_lastProcessedTimestamp) {
                continue;
            }

            m_lastProcessedTimestamp = output.renderTimestamp;
            if (output.stale || !output.hasRgb()) {
                continue;
            }

            if (!handleFreshOutput(output)) {
                return false;
            }
        }

        return true;
    }

    bool ViewerFrameDumper::handleFreshOutput(
        const edgevision::model::viewer::RenderOutput& output
    ) {
        m_observedFreshOutput = true;
        if (!m_config.enabled || m_dumpedFreshOutputCount >= m_config.maxFreshOutputs) {
            return true;
        }

        const std::size_t nextSequence = m_dumpedFreshOutputCount + 1;
        if (!writeOutput(output, nextSequence)) {
            return false;
        }

        ++m_dumpedFreshOutputCount;
        return true;
    }

    bool ViewerFrameDumper::writeOutput(
        const edgevision::model::viewer::RenderOutput& output,
        std::size_t sequence
    ) const {
        if (output.imageSize.width <= 0 || output.imageSize.height <= 0) {
            return false;
        }

        const std::size_t expectedByteCount = static_cast<std::size_t>(output.imageSize.width)
            * static_cast<std::size_t>(output.imageSize.height) * 3;
        if (output.rgb.byteCount < expectedByteCount) {
            return false;
        }

        std::error_code error{};
        std::filesystem::create_directories(m_outputDirectory, error);
        if (error) {
            return false;
        }

        const std::filesystem::path outputPath = makeOutputPath(m_outputDirectory, sequence);
        std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            return false;
        }

        stream << "P6\n" << output.imageSize.width << ' ' << output.imageSize.height << "\n255\n";
        stream.write(
            reinterpret_cast<const char*>(output.rgb.data),
            static_cast<std::streamsize>(expectedByteCount)
        );
        return static_cast<bool>(stream);
    }

} // namespace edgevision::app::runtime
