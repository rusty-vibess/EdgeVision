#include "frame/queue/ReadyFrameQueue.hpp"

#include <algorithm>
#include <mutex>

namespace edgevision::model::frame {

    ReadyFrameQueue::ReadyFrameQueue(std::size_t capacity)
        : m_queue(std::max<std::size_t>(1, capacity)) {}

    bool ReadyFrameQueue::tryEnqueue(const Frame& frame) {
        std::lock_guard lock(m_mutex);
        if (!m_queue.try_enqueue(frame)) {
            return false;
        }

        m_readyFrameIds.insert(frame.frameId);
        return true;
    }

    std::optional<Frame> ReadyFrameQueue::tryDequeue() {
        Frame frame{};
        if (!m_queue.try_dequeue(frame)) {
            return std::nullopt;
        }

        std::lock_guard lock(m_mutex);
        m_readyFrameIds.erase(frame.frameId);
        m_inFlightFrameIds.insert(frame.frameId);
        return frame;
    }

    Frame ReadyFrameQueue::waitDequeue() {
        Frame frame{};
        m_queue.wait_dequeue(frame);

        std::lock_guard lock(m_mutex);
        m_readyFrameIds.erase(frame.frameId);
        m_inFlightFrameIds.insert(frame.frameId);
        return frame;
    }

    bool ReadyFrameQueue::markCompleted(FrameId frameId) {
        std::lock_guard lock(m_mutex);
        if (!m_inFlightFrameIds.contains(frameId)) {
            return false;
        }

        m_inFlightFrameIds.erase(frameId);
        return true;
    }

    std::size_t ReadyFrameQueue::sizeApprox() const {
        return m_queue.size_approx();
    }

} // namespace edgevision::model::frame
