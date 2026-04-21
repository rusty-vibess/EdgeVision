#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <readerwritercircularbuffer.h>
#include <unordered_set>

#include "model/frame/types/identity.hpp"
#include "model/frame/types/packet.hpp"

namespace edgevision::model::frame {

    class FramePacketQueue final {
      public:
        explicit FramePacketQueue(std::size_t capacity);

        FramePacketQueue(const FramePacketQueue&) = delete;
        FramePacketQueue& operator=(const FramePacketQueue&) = delete;

        [[nodiscard]] bool tryEnqueue(const FramePacket& packet);
        [[nodiscard]] std::optional<FramePacket> tryDequeue();
        [[nodiscard]] FramePacket waitDequeue();
        [[nodiscard]] bool markSent(FrameId frameId);
        [[nodiscard]] std::size_t sizeApprox() const;

      private:
        moodycamel::BlockingReaderWriterCircularBuffer<FramePacket> m_queue;
        mutable std::mutex m_mutex{};
        std::unordered_set<FrameId> m_pendingFrameIds{};
        std::unordered_set<FrameId> m_inFlightFrameIds{};
    };

} // namespace edgevision::model::frame
