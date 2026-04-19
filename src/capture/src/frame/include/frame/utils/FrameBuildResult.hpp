#pragma once

#include <string>

#include "capture/frame/types/build_result.hpp"
#include "model/frame/types/frame.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::capture::frame {

    [[nodiscard]] FrameBuildResult makeFrameBuildFailure(
        FrameBuildCode code,
        edgevision::model::frame::FrameId frameId,
        std::string message
    );

    [[nodiscard]] FrameBuildResult makeFrameBuildSuccess(edgevision::model::frame::Frame frame);

} // namespace edgevision::capture::frame
