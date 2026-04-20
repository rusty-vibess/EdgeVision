#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <string>
#include <vector>

#include "model/frame/types/frame.hpp"
#include "model/frame/types/identity.hpp"
#include "model/frame/types/lifecycle.hpp"
#include "model/frame/types/packet.hpp"
#include "model/frame/types/store_status.hpp"
#include "model/frame/types/submission.hpp"

namespace edgevision::model::frame {

    class FrameHistory;
    class FrameLifecycleTracker;
    class FramePacketQueue;

    class FrameStore final {
      public:
        explicit FrameStore(const FrameStoreConfig& config = FrameStoreConfig{});
        ~FrameStore();

        FrameStore(const FrameStore&) = delete;
        FrameStore& operator=(const FrameStore&) = delete;

        [[nodiscard]] FrameSubmissionResult submitFrame(Frame frame);

        [[nodiscard]] std::optional<Frame> getLastFrame() const;
        [[nodiscard]] std::optional<Frame> getFrame(FrameId frameId) const;
        [[nodiscard]] std::vector<Frame> getRecentFrames(std::size_t count) const;

        [[nodiscard]] std::optional<FramePacket> getNextFramePacket() const;
        [[nodiscard]] FramePacket waitForNextFramePacket() const;
        [[nodiscard]] bool markFramePacketSent(FrameId frameId);

        [[nodiscard]] FrameStoreStatus getStatus() const;
        [[nodiscard]] std::optional<FrameLifecycle> getFrameLifecycle(FrameId frameId) const;

      private:
        void evictOldFramesLocked();
        void recordRejectedFrameLocked(const Frame& frame, const FrameSubmissionResult& result);
        void recordStatusLocked(const FrameSubmissionResult& result);
        [[nodiscard]] bool hasEvictableFrameLocked() const;

        FrameStoreConfig m_config{};

        mutable std::shared_mutex m_mutex{};
        std::unique_ptr<FrameHistory> m_history{};
        std::unique_ptr<FrameLifecycleTracker> m_lifecycleTracker{};
        std::unique_ptr<FramePacketQueue> m_packetQueue{};

        std::size_t m_acceptedFrameCount = 0;
        std::size_t m_rejectedFrameCount = 0;
        std::size_t m_droppedFrameCount = 0;
        std::size_t m_evictedFrameCount = 0;
        FrameSubmissionCode m_lastSubmissionCode = FrameSubmissionCode::InvalidFrameId;
        std::string m_lastError{};
    };

} // namespace edgevision::model::frame
