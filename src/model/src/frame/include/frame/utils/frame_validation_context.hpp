#pragma once

#include <optional>

#include "model/frame/types/identity.hpp"

namespace edgevision::model::frame {

    struct FrameValidationContext {
        std::optional<FrameId> latestFrameId{};
        std::optional<FrameTimestamp> latestTimestamp{};
        bool historyFull = false;
        bool hasEvictableFrame = false;
    };

} // namespace edgevision::model::frame
