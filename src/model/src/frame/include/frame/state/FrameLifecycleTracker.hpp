#pragma once

#include <optional>
#include <unordered_map>

#include "model/frame/types/identity.hpp"
#include "model/frame/types/lifecycle.hpp"

namespace edgevision::model::frame {

    class FrameLifecycleTracker final {
      public:
        void recordAcceptedQueued(FrameId frameId);
        void recordInvalid(FrameId frameId);
        void markSent(FrameId frameId);
        void markDropped(FrameId frameId);

        [[nodiscard]] bool isPendingForFpga(FrameId frameId) const;
        [[nodiscard]] std::optional<FrameLifecycle> get(FrameId frameId) const;
        [[nodiscard]] std::unordered_map<FrameId, FrameLifecycle> snapshot() const;

      private:
        std::unordered_map<FrameId, FrameLifecycle> m_lifecyclesById{};
    };

} // namespace edgevision::model::frame
