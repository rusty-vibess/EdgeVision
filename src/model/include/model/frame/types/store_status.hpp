#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>

#include "model/frame/types/identity.hpp"
#include "model/frame/types/lifecycle.hpp"
#include "model/frame/types/submission.hpp"

namespace edgevision::model::frame {

    struct FrameStoreConfig {
        std::size_t maxStoredFrames = 120;
        std::size_t maxReadyFrames = 120;
    };

    struct FrameStoreStatus {
        std::size_t storedFrameCount = 0;
        std::size_t maxStoredFrames = 0;
        std::size_t readyFrameCount = 0;
        std::size_t maxReadyFrames = 0;
        std::size_t acceptedFrameCount = 0;
        std::size_t rejectedFrameCount = 0;
        std::size_t droppedFrameCount = 0;
        std::size_t evictedFrameCount = 0;
        std::optional<FrameId> latestFrameId{};
        std::optional<FrameTimestamp> latestTimestamp{};
        FrameSubmissionCode lastSubmissionCode = FrameSubmissionCode::InvalidFrameId;
        std::string lastError{};
        std::unordered_map<FrameId, FrameLifecycle> lifecycles{};
    };

} // namespace edgevision::model::frame
