#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <thread>

#include "capture/frame/types/runner_status.hpp"
#include "capture/interfaces/K4aInterface.hpp"
#include "config/types/capture.hpp"

namespace edgevision::capture {
    class CameraCapture;
} // namespace edgevision::capture

namespace edgevision::model::frame {
    class FrameStore;
} // namespace edgevision::model::frame

namespace edgevision::capture::frame {

    struct FrameIngestResult;
    class FrameIngestor;

    /// Runs the background frame capture and ingest loop.
    class FrameIngestorRunner final {
      public:
        FrameIngestorRunner(
            edgevision::capture::CameraCapture& capture,
            edgevision::model::frame::FrameStore& frameStore,
            const edgevision::config::CaptureRuntimeConfig& config =
                edgevision::config::CaptureRuntimeConfig{},
            const edgevision::capture::K4aInterface& api =
                edgevision::capture::K4aInterface::instance()
        );
        ~FrameIngestorRunner();

        FrameIngestorRunner(const FrameIngestorRunner&) = delete;
        FrameIngestorRunner& operator=(const FrameIngestorRunner&) = delete;
        FrameIngestorRunner(FrameIngestorRunner&& other) noexcept = delete;
        FrameIngestorRunner& operator=(FrameIngestorRunner&& other) noexcept = delete;

        /// Starts the background ingest loop.
        [[nodiscard]] bool start();
        /// Requests that the background ingest loop stop.
        void requestStop();
        /// Waits for the background ingest loop to exit.
        void join();

        /// Returns whether the background ingest loop is currently running.
        [[nodiscard]] bool running() const;
        /// Returns the latest frame ingestor runner status snapshot.
        [[nodiscard]] FrameIngestorRunnerStatus status() const;

      private:
        void runLoop();
        void recordResult(const FrameIngestResult& result);
        void setRunning(bool running);

        std::unique_ptr<FrameIngestor> m_ingestor{};
        edgevision::config::CaptureRuntimeConfig m_config{};
        std::atomic_bool m_stopRequested{false};
        std::atomic_bool m_running{false};
        mutable std::mutex m_statusMutex{};
        std::thread m_thread{};
        FrameIngestorRunnerStatus m_status{};
    };

} // namespace edgevision::capture::frame
