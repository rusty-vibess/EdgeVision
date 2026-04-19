#pragma once

#include <k4a/k4a.h>

#include "capture/frame/types/build_result.hpp"
#include "capture/interfaces/K4aInterface.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::capture::frame {

    class K4aFrameBuilder final {
      public:
        explicit K4aFrameBuilder(const K4aInterface& api = K4aInterface::instance());

        [[nodiscard]] FrameBuildResult build(
            k4a_capture_t capture,
            const k4a_calibration_t& calibration,
            const k4a_device_configuration_t& config,
            edgevision::model::frame::FrameId frameId,
            edgevision::model::frame::FrameTimestamp timestamp
        ) const;

      private:
        const K4aInterface& m_api;
    };

} // namespace edgevision::capture::frame
