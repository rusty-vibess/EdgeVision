#include "model/frame/FrameStore.hpp"

#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include "frame/queue/FramePacketQueue.hpp"
#include "frame/state/FrameHistory.hpp"
#include "frame/state/FrameLifecycleTracker.hpp"
#include "frame/utils/FrameValidator.hpp"
#include "frame/utils/frame_validation_context.hpp"

namespace edgevision::model::frame {
    namespace {
        [[nodiscard]] FrameSubmissionResult makeResult(
            FrameSubmissionCode code,
            FrameId frameId,
            std::string message
        ) {
            return FrameSubmissionResult{code, frameId, std::move(message)};
        }
    } // namespace

    FrameStore::FrameStore(const FrameStoreConfig& config)
        : m_config(config),
          m_history(std::make_unique<FrameHistory>()),
          m_lifecycleTracker(std::make_unique<FrameLifecycleTracker>()) {
        m_config.maxStoredFrames = std::max<std::size_t>(1, m_config.maxStoredFrames);
        m_config.maxPendingFramePackets =
            std::max<std::size_t>(1, m_config.maxPendingFramePackets);
        m_packetQueue = std::make_unique<FramePacketQueue>(m_config.maxPendingFramePackets);
    }

    FrameStore::~FrameStore() = default;

    FrameSubmissionResult FrameStore::submitFrame(Frame frame) {
        std::unique_lock lock(m_mutex);

        // Validate new Frame submission
        FrameValidationContext validationContext{};
        validationContext.latestFrameId = m_history->latestFrameId();
        validationContext.latestTimestamp = m_history->latestTimestamp();
        validationContext.historyFull = m_history->size() >= m_config.maxStoredFrames;
        validationContext.hasEvictableFrame =
            m_history
                ->findOldestEvictable([this](FrameId frameId) {
                    return !m_lifecycleTracker->isPendingForFpga(frameId);
                })
                .has_value();
        FrameSubmissionResult result = FrameValidator::validate(frame, validationContext);
        if (!result.accepted()) {
            recordRejectedFrameLocked(frame, result);
            return result;
        }

        // Enqueue valid Frame as FramePacket
        const FramePacket packet = frame;
        if (!m_packetQueue->tryEnqueue(packet)) {
            result = makeResult(
                FrameSubmissionCode::PacketQueueFull,
                frame.frameId,
                "Pending frame packet queue is full"
            );
            recordRejectedFrameLocked(frame, result);
            return result;
        }

        m_lifecycleTracker->recordAcceptedQueued(frame.frameId);
        m_history->add(std::move(frame));
        ++m_acceptedFrameCount;

        evictOldFramesLocked();
        recordStatusLocked(result);
        return result;
    }

    std::optional<Frame> FrameStore::getLastFrame() const {
        std::shared_lock lock(m_mutex);
        return m_history->latest();
    }

    std::optional<Frame> FrameStore::getFrame(FrameId frameId) const {
        std::shared_lock lock(m_mutex);
        return m_history->get(frameId);
    }

    std::vector<Frame> FrameStore::getRecentFrames(std::size_t count) const {
        std::shared_lock lock(m_mutex);
        return m_history->recent(count);
    }

    std::optional<FramePacket> FrameStore::getNextFramePacket() const {
        return m_packetQueue->tryDequeue();
    }

    FramePacket FrameStore::waitForNextFramePacket() const {
        return m_packetQueue->waitDequeue();
    }

    bool FrameStore::markFramePacketSent(FrameId frameId) {
        std::unique_lock lock(m_mutex);

        if (!m_packetQueue->markSent(frameId)) {
            return false;
        }

        m_lifecycleTracker->markSent(frameId);
        evictOldFramesLocked();
        return true;
    }

    FrameStoreStatus FrameStore::getStatus() const {
        std::shared_lock lock(m_mutex);

        FrameStoreStatus status{};
        status.storedFrameCount = m_history->size();
        status.maxStoredFrames = m_config.maxStoredFrames;
        status.queuedPacketCount = m_packetQueue->sizeApprox();
        status.maxPendingFramePackets = m_config.maxPendingFramePackets;
        status.acceptedFrameCount = m_acceptedFrameCount;
        status.rejectedFrameCount = m_rejectedFrameCount;
        status.droppedFrameCount = m_droppedFrameCount;
        status.evictedFrameCount = m_evictedFrameCount;
        status.latestFrameId = m_history->latestFrameId();
        status.latestTimestamp = m_history->latestTimestamp();
        status.lastSubmissionCode = m_lastSubmissionCode;
        status.lastError = m_lastError;
        status.lifecycles = m_lifecycleTracker->snapshot();
        return status;
    }

    std::optional<FrameLifecycle> FrameStore::getFrameLifecycle(FrameId frameId) const {
        std::shared_lock lock(m_mutex);
        return m_lifecycleTracker->get(frameId);
    }

    void FrameStore::evictOldFramesLocked() {
        while (m_history->size() > m_config.maxStoredFrames) {
            const std::optional<FrameId> latestFrameId = m_history->latestFrameId();
            const std::optional<FrameId> frameId =
                m_history->findOldestEvictable([this, latestFrameId](FrameId candidateFrameId) {
                    return (!latestFrameId.has_value() || candidateFrameId != *latestFrameId)
                        && !m_lifecycleTracker->isPendingForFpga(candidateFrameId);
                });

            if (!frameId.has_value()) {
                break;
            }

            m_lifecycleTracker->markDropped(*frameId);
            m_history->remove(*frameId);
            ++m_droppedFrameCount;
            ++m_evictedFrameCount;
        }
    }

    void FrameStore::recordRejectedFrameLocked(
        const Frame& frame,
        const FrameSubmissionResult& result
    ) {
        m_lifecycleTracker->recordInvalid(frame.frameId);
        ++m_rejectedFrameCount;
        recordStatusLocked(result);
    }

    void FrameStore::recordStatusLocked(const FrameSubmissionResult& result) {
        m_lastSubmissionCode = result.code;
        m_lastError = result.accepted() ? std::string{} : result.message;
    }

} // namespace edgevision::model::frame
