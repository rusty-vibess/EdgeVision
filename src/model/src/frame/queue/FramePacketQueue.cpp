#include "frame/queue/FramePacketQueue.hpp"

#include <algorithm>

namespace edgevision::model::frame {

    FramePacketQueue::FramePacketQueue(std::size_t capacity)
        : m_queue(std::max<std::size_t>(1, capacity)) {}

    bool FramePacketQueue::tryEnqueue(const FramePacket& packet) {
        if (!m_queue.try_enqueue(packet)) {
            return false;
        }

        m_pendingFrameIds.insert(packet.frameId);
        m_latestQueuedFramePacket = packet;
        return true;
    }

    std::optional<FramePacket> FramePacketQueue::front() {
        FramePacket* packet = m_queue.peek();
        if (packet == nullptr) {
            return std::nullopt;
        }

        return *packet;
    }

    std::optional<FramePacket> FramePacketQueue::latest() const {
        return m_latestQueuedFramePacket;
    }

    bool FramePacketQueue::markSent(FrameId frameId) {
        FramePacket* packet = m_queue.peek();
        if (packet == nullptr || packet->frameId != frameId) {
            return false;
        }

        if (!m_queue.try_pop()) {
            return false;
        }

        m_pendingFrameIds.erase(frameId);
        if (m_pendingFrameIds.empty()) {
            m_latestQueuedFramePacket = std::nullopt;
        }

        return true;
    }

    bool FramePacketQueue::empty() const {
        return m_pendingFrameIds.empty();
    }

    std::size_t FramePacketQueue::sizeApprox() const {
        return m_queue.size_approx();
    }

    std::size_t FramePacketQueue::maxCapacity() const {
        return m_queue.max_capacity();
    }

    bool FramePacketQueue::hasPending(FrameId frameId) const {
        return m_pendingFrameIds.contains(frameId);
    }

} // namespace edgevision::model::frame
