#pragma once

#include <atomic>
#include <mutex>
#include <thread>

#include "capture/frame/FrameIngestor.hpp"
#include "capture/frame/types/runner_status.hpp"
#include "config/types/capture.hpp"

namespace edgevision::capture::frame {

    class FrameIngestorRunner final {
      public:
        explicit FrameIngestorRunner(
            FrameIngestor& ingestor,
            const edgevision::config::CaptureRuntimeConfig& config =
                edgevision::config::CaptureRuntimeConfig{}
        );
        ~FrameIngestorRunner();

        FrameIngestorRunner(const FrameIngestorRunner&) = delete;
        FrameIngestorRunner& operator=(const FrameIngestorRunner&) = delete;

        [[nodiscard]] bool start();
        void requestStop();
        void join();

        [[nodiscard]] bool running() const;
        [[nodiscard]] FrameIngestorRunnerStatus status() const;

      private:
        void runLoop();
        void recordResult(const FrameIngestResult& result);
        void setRunning(bool running);

        FrameIngestor& m_ingestor;
        edgevision::config::CaptureRuntimeConfig m_config{};
        std::atomic_bool m_stopRequested{false};
        std::atomic_bool m_running{false};
        mutable std::mutex m_statusMutex{};
        std::thread m_thread{};
        FrameIngestorRunnerStatus m_status{};
    };

} // namespace edgevision::capture::frame
