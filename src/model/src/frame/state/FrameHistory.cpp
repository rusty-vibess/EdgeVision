#include "frame/state/FrameHistory.hpp"

#include <algorithm>
#include <utility>

namespace edgevision::model::frame {

    void FrameHistory::add(Frame frame) {
        m_history.push_back(frame);
        m_framesById[frame.frameId] = frame;
        m_latestFrame = std::move(frame);
    }

    void FrameHistory::remove(FrameId frameId) {
        const auto frame =
            std::find_if(m_history.begin(), m_history.end(), [frameId](const Frame& candidate) {
                return candidate.frameId == frameId;
            });

        if (frame != m_history.end()) {
            m_history.erase(frame);
        }

        m_framesById.erase(frameId);
    }

    std::optional<Frame> FrameHistory::latest() const {
        return m_latestFrame;
    }

    std::optional<Frame> FrameHistory::get(FrameId frameId) const {
        const auto frame = m_framesById.find(frameId);
        if (frame == m_framesById.end()) {
            return std::nullopt;
        }

        return frame->second;
    }

    std::vector<Frame> FrameHistory::recent(std::size_t count) const {
        const std::size_t clampedCount = std::min(count, m_history.size());
        const std::size_t start = m_history.size() - clampedCount;

        std::vector<Frame> frames{};
        frames.reserve(clampedCount);
        for (std::size_t index = start; index < m_history.size(); ++index) {
            frames.push_back(m_history[index]);
        }

        return frames;
    }

    std::size_t FrameHistory::size() const {
        return m_history.size();
    }

    std::optional<FrameId> FrameHistory::latestFrameId() const {
        if (!m_latestFrame.has_value()) {
            return std::nullopt;
        }

        return m_latestFrame->frameId;
    }

    std::optional<FrameTimestamp> FrameHistory::latestTimestamp() const {
        if (!m_latestFrame.has_value()) {
            return std::nullopt;
        }

        return m_latestFrame->timestamp;
    }

    std::optional<FrameId> FrameHistory::findOldestEvictable(
        const std::function<bool(FrameId)>& canEvict
    ) const {
        for (const Frame& frame : m_history) {
            if (m_latestFrame.has_value() && frame.frameId == m_latestFrame->frameId) {
                continue;
            }

            if (canEvict(frame.frameId)) {
                return frame.frameId;
            }
        }

        return std::nullopt;
    }

} // namespace edgevision::model::frame
