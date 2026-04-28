#pragma once

#include <optional>
#include <string_view>

#include "streaming/webrtc/types/PoseUpdate.hpp"

namespace edgevision::streaming::webrtc::utils {

    [[nodiscard]] std::optional<PoseUpdate> parsePoseUpdateMessage(std::string_view payload);

} // namespace edgevision::streaming::webrtc::utils
