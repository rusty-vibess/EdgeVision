#include "frame/state/FrameLifecycleTracker.hpp"

namespace edgevision::model::frame {

    const char* frameLifecycleStateToString(FrameLifecycleState state) {
        switch (state) {
            case FrameLifecycleState::Captured:
                return "captured";
            case FrameLifecycleState::Stored:
                return "stored";
            case FrameLifecycleState::QueuedForFpga:
                return "queued_for_fpga";
            case FrameLifecycleState::SentToFpga:
                return "sent_to_fpga";
            case FrameLifecycleState::FusionResultReceived:
                return "fusion_result_received";
            case FrameLifecycleState::Dropped:
                return "dropped";
            case FrameLifecycleState::Invalid:
                return "invalid";
            case FrameLifecycleState::Failed:
                return "failed";
        }

        return "unknown";
    }

    void FrameLifecycleTracker::recordAcceptedQueued(FrameId frameId) {
        FrameLifecycle lifecycle{};
        lifecycle.frameId = frameId;
        lifecycle.state = FrameLifecycleState::QueuedForFpga;
        lifecycle.captured = true;
        lifecycle.stored = true;
        lifecycle.queuedForFpga = true;
        m_lifecyclesById[frameId] = lifecycle;
    }

    void FrameLifecycleTracker::recordInvalid(FrameId frameId) {
        if (frameId == 0 || m_lifecyclesById.contains(frameId)) {
            return;
        }

        FrameLifecycle lifecycle{};
        lifecycle.frameId = frameId;
        lifecycle.state = FrameLifecycleState::Invalid;
        lifecycle.captured = true;
        lifecycle.invalid = true;
        m_lifecyclesById[frameId] = lifecycle;
    }

    void FrameLifecycleTracker::markSent(FrameId frameId) {
        auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return;
        }

        lifecycle->second.state = FrameLifecycleState::SentToFpga;
        lifecycle->second.sentToFpga = true;
    }

    void FrameLifecycleTracker::markDropped(FrameId frameId) {
        auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return;
        }

        lifecycle->second.state = FrameLifecycleState::Dropped;
        lifecycle->second.dropped = true;
    }

    bool FrameLifecycleTracker::isPendingForFpga(FrameId frameId) const {
        const auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return false;
        }

        return lifecycle->second.queuedForFpga && !lifecycle->second.sentToFpga
            && !lifecycle->second.dropped && !lifecycle->second.invalid
            && !lifecycle->second.failed;
    }

    std::optional<FrameLifecycle> FrameLifecycleTracker::get(FrameId frameId) const {
        const auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return std::nullopt;
        }

        return lifecycle->second;
    }

    std::unordered_map<FrameId, FrameLifecycle> FrameLifecycleTracker::snapshot() const {
        return m_lifecyclesById;
    }

} // namespace edgevision::model::frame
