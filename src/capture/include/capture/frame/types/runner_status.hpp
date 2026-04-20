#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "capture/frame/types/ingest_result.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::capture::frame {

    struct FrameIngestorRunnerStatus {
        bool running = false;
        bool stopRequested = false;
        std::size_t ingestAttemptCount = 0;
        std::size_t submittedFrameCount = 0;
        std::size_t failedIngestCount = 0;
        std::optional<FrameIngestCode> lastCode{};
        edgevision::model::frame::FrameId lastFrameId = 0;
        std::string lastMessage{};
    };

} // namespace edgevision::capture::frame
