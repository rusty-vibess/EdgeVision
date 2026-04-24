#pragma once

#include <cstddef>
#include <mutex>
#include <optional>
#include <readerwritercircularbuffer.h>
#include <unordered_set>

#include "model/frame/types/frame.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::model::frame {

    class ReadyFrameQueue final {
      public:
        explicit ReadyFrameQueue(std::size_t capacity);

        ReadyFrameQueue(const ReadyFrameQueue&) = delete;
        ReadyFrameQueue& operator=(const ReadyFrameQueue&) = delete;

        [[nodiscard]] bool tryEnqueue(const Frame& frame);
        [[nodiscard]] std::optional<Frame> tryDequeue();
        [[nodiscard]] Frame waitDequeue();
        [[nodiscard]] bool markCompleted(FrameId frameId);
        [[nodiscard]] std::size_t sizeApprox() const;

      private:
        moodycamel::BlockingReaderWriterCircularBuffer<Frame> m_queue;
        mutable std::mutex m_mutex{};
        std::unordered_set<FrameId> m_readyFrameIds{};
        std::unordered_set<FrameId> m_inFlightFrameIds{};
    };

} // namespace edgevision::model::frame
