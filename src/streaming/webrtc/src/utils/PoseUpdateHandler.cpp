#include "streaming/webrtc/utils/PoseUpdateHandler.hpp"

#include <nlohmann/json.hpp>

namespace edgevision::streaming::webrtc::utils {

    std::optional<PoseUpdate> parsePoseUpdateMessage(std::string_view payload) {
        try {
            const auto message = nlohmann::json::parse(payload.begin(), payload.end());
            if (message.value("type", "") != "pose") {
                return std::nullopt;
            }

            const nlohmann::json* matrix = nullptr;
            if (message.contains("matrix")) {
                matrix = &message.at("matrix");
            } else if (message.contains("pose")) {
                matrix = &message.at("pose");
            }
            if (matrix == nullptr || !matrix->is_array() || matrix->size() != 16) {
                return std::nullopt;
            }

            PoseUpdate poseUpdate{};
            for (std::size_t i = 0; i < 16; ++i) {
                poseUpdate.matrix[i] = (*matrix)[i].get<float>();
            }
            return poseUpdate;
        } catch (...) {
            return std::nullopt;
        }
    }

} // namespace edgevision::streaming::webrtc::utils
