#pragma once

#include <cstddef>
#include <deque>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

#include "model/frame/types/frame.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::model::frame {

    class FrameHistory final {
      public:
        void add(Frame frame);
        void remove(FrameId frameId);

        [[nodiscard]] std::optional<Frame> latest() const;
        [[nodiscard]] std::optional<Frame> get(FrameId frameId) const;
        [[nodiscard]] std::vector<Frame> recent(std::size_t count) const;
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] std::optional<FrameId> latestFrameId() const;
        [[nodiscard]] std::optional<FrameTimestamp> latestTimestamp() const;
        [[nodiscard]] std::optional<FrameId> findOldestEvictable(
            const std::function<bool(FrameId)>& canEvict
        ) const;

      private:
        std::optional<Frame> m_latestFrame{};
        std::deque<Frame> m_history{};
        std::unordered_map<FrameId, Frame> m_framesById{};
    };

} // namespace edgevision::model::frame
