#include "capture/frame/FrameIngestorRunner.hpp"

#include <chrono>
#include <memory>
#include <thread>

#include "frame/state/FrameIngestor.hpp"

namespace edgevision::capture::frame {

    FrameIngestorRunner::FrameIngestorRunner(
        edgevision::capture::CameraCapture& capture,
        edgevision::model::frame::FrameStore& frameStore,
        const edgevision::config::CaptureRuntimeConfig& config,
        const edgevision::capture::K4aInterface& api
    )
        : m_ingestor(std::make_unique<FrameIngestor>(capture, frameStore, api)),
          m_config(config) {}

    FrameIngestorRunner::~FrameIngestorRunner() {
        requestStop();
        join();
    }

    bool FrameIngestorRunner::start() {
        if (m_thread.joinable() || m_running.load()) {
            return false;
        }

        m_stopRequested.store(false);
        setRunning(true);
        m_thread = std::thread(&FrameIngestorRunner::runLoop, this);
        return true;
    }

    void FrameIngestorRunner::requestStop() {
        m_stopRequested.store(true);
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_status.stopRequested = true;
    }

    void FrameIngestorRunner::join() {
        if (m_thread.joinable()) {
            m_thread.join();
        }
    }

    bool FrameIngestorRunner::running() const {
        return m_running.load();
    }

    FrameIngestorRunnerStatus FrameIngestorRunner::status() const {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        return m_status;
    }

    void FrameIngestorRunner::runLoop() {
        while (!m_stopRequested.load()) {
            const FrameIngestResult result =
                m_ingestor->captureAndSubmit(m_config.captureTimeoutMs);
            recordResult(result);

            if (!result.submitted() && m_config.failureBackoffMs > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(m_config.failureBackoffMs));
            }
        }

        setRunning(false);
    }

    void FrameIngestorRunner::recordResult(const FrameIngestResult& result) {
        std::lock_guard<std::mutex> lock(m_statusMutex);
        ++m_status.ingestAttemptCount;
        m_status.lastCode = result.code;
        m_status.lastFrameId = result.frameId;
        m_status.lastMessage = result.message;

        if (result.submitted()) {
            ++m_status.submittedFrameCount;
        } else {
            ++m_status.failedIngestCount;
        }
    }

    void FrameIngestorRunner::setRunning(bool running) {
        m_running.store(running);
        std::lock_guard<std::mutex> lock(m_statusMutex);
        m_status.running = running;
        m_status.stopRequested = m_stopRequested.load();
    }

} // namespace edgevision::capture::frame
