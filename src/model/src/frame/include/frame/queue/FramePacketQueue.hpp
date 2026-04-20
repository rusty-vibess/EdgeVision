#pragma once

#include <cstddef>
#include <optional>
#include <unordered_set>

#include "model/frame/types/identity.hpp"
#include "model/frame/types/packet.hpp"
#include <readerwritercircularbuffer.h>

namespace edgevision::model::frame {

    class FramePacketQueue final {
      public:
        explicit FramePacketQueue(std::size_t capacity);

        FramePacketQueue(const FramePacketQueue&) = delete;
        FramePacketQueue& operator=(const FramePacketQueue&) = delete;

        [[nodiscard]] bool tryEnqueue(const FramePacket& packet);
        [[nodiscard]] std::optional<FramePacket> front();
        [[nodiscard]] std::optional<FramePacket> latest() const;
        [[nodiscard]] bool markSent(FrameId frameId);
        [[nodiscard]] bool empty() const;
        [[nodiscard]] std::size_t sizeApprox() const;
        [[nodiscard]] std::size_t maxCapacity() const;
        [[nodiscard]] bool hasPending(FrameId frameId) const;

      private:
        moodycamel::BlockingReaderWriterCircularBuffer<FramePacket> m_queue;
        std::unordered_set<FrameId> m_pendingFrameIds{};
        std::optional<FramePacket> m_latestQueuedFramePacket{};
    };

} // namespace edgevision::model::frame
