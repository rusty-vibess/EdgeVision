#include "frame/queue/FramePacketQueue.hpp"

#include <algorithm>
#include <mutex>

namespace edgevision::model::frame {

    FramePacketQueue::FramePacketQueue(std::size_t capacity)
        : m_queue(std::max<std::size_t>(1, capacity)) {}

    bool FramePacketQueue::tryEnqueue(const FramePacket& packet) {
        std::lock_guard lock(m_mutex);
        if (!m_queue.try_enqueue(packet)) {
            return false;
        }

        m_pendingFrameIds.insert(packet.frameId);
        return true;
    }

    std::optional<FramePacket> FramePacketQueue::tryDequeue() {
        FramePacket packet{};
        if (!m_queue.try_dequeue(packet)) {
            return std::nullopt;
        }

        std::lock_guard lock(m_mutex);
        m_pendingFrameIds.erase(packet.frameId);
        m_inFlightFrameIds.insert(packet.frameId);
        return packet;
    }

    FramePacket FramePacketQueue::waitDequeue() {
        FramePacket packet{};
        m_queue.wait_dequeue(packet);

        std::lock_guard lock(m_mutex);
        m_pendingFrameIds.erase(packet.frameId);
        m_inFlightFrameIds.insert(packet.frameId);
        return packet;
    }

    bool FramePacketQueue::markSent(FrameId frameId) {
        std::lock_guard lock(m_mutex);
        if (!m_inFlightFrameIds.contains(frameId)) {
            return false;
        }

        m_inFlightFrameIds.erase(frameId);
        return true;
    }

    std::size_t FramePacketQueue::sizeApprox() const {
        return m_queue.size_approx();
    }

} // namespace edgevision::model::frame
