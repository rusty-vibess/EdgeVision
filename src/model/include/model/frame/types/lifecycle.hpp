#pragma once

#include "model/frame/types/identity.hpp"

namespace edgevision::model::frame {

    enum class FrameLifecycleState {
        Captured,
        Stored,
        ReadyForConsumer,
        DispatchedToConsumer,
        Consumed,
        Dropped,
        Invalid,
        Failed,
    };

    struct FrameLifecycle {
        FrameId frameId = 0;
        FrameLifecycleState state = FrameLifecycleState::Captured;
        bool captured = false;
        bool stored = false;
        bool readyForConsumer = false;
        bool dispatchedToConsumer = false;
        bool consumed = false;
        bool dropped = false;
        bool invalid = false;
        bool failed = false;
    };

    [[nodiscard]] const char* frameLifecycleStateToString(FrameLifecycleState state);

} // namespace edgevision::model::frame
