#include "frame/state/FrameLifecycleTracker.hpp"

namespace edgevision::model::frame {

    const char* frameLifecycleStateToString(FrameLifecycleState state) {
        switch (state) {
            case FrameLifecycleState::Captured:
                return "captured";
            case FrameLifecycleState::Stored:
                return "stored";
            case FrameLifecycleState::ReadyForConsumer:
                return "ready_for_consumer";
            case FrameLifecycleState::DispatchedToConsumer:
                return "dispatched_to_consumer";
            case FrameLifecycleState::Consumed:
                return "consumed";
            case FrameLifecycleState::Dropped:
                return "dropped";
            case FrameLifecycleState::Invalid:
                return "invalid";
            case FrameLifecycleState::Failed:
                return "failed";
        }

        return "unknown";
    }

    void FrameLifecycleTracker::recordAcceptedReady(FrameId frameId) {
        FrameLifecycle lifecycle{};
        lifecycle.frameId = frameId;
        lifecycle.state = FrameLifecycleState::ReadyForConsumer;
        lifecycle.captured = true;
        lifecycle.stored = true;
        lifecycle.readyForConsumer = true;
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

    void FrameLifecycleTracker::markDispatched(FrameId frameId) {
        auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return;
        }

        lifecycle->second.state = FrameLifecycleState::DispatchedToConsumer;
        lifecycle->second.dispatchedToConsumer = true;
    }

    void FrameLifecycleTracker::markConsumed(FrameId frameId) {
        auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return;
        }

        lifecycle->second.state = FrameLifecycleState::Consumed;
        lifecycle->second.dispatchedToConsumer = true;
        lifecycle->second.consumed = true;
    }

    void FrameLifecycleTracker::markFailed(FrameId frameId) {
        auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return;
        }

        lifecycle->second.state = FrameLifecycleState::Failed;
        lifecycle->second.dispatchedToConsumer = true;
        lifecycle->second.failed = true;
    }

    void FrameLifecycleTracker::markDropped(FrameId frameId) {
        auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return;
        }

        lifecycle->second.state = FrameLifecycleState::Dropped;
        lifecycle->second.dropped = true;
    }

    bool FrameLifecycleTracker::isReadyForConsumer(FrameId frameId) const {
        const auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return false;
        }

        return lifecycle->second.state == FrameLifecycleState::ReadyForConsumer;
    }

    bool FrameLifecycleTracker::isProtectedFromEviction(FrameId frameId) const {
        const auto lifecycle = m_lifecyclesById.find(frameId);
        if (lifecycle == m_lifecyclesById.end()) {
            return false;
        }

        return lifecycle->second.state == FrameLifecycleState::ReadyForConsumer
            || lifecycle->second.state == FrameLifecycleState::DispatchedToConsumer;
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
