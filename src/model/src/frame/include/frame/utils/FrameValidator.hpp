#pragma once

#include "frame/utils/frame_validation_context.hpp"
#include "model/frame/types/frame.hpp"
#include "model/frame/types/submission.hpp"

namespace edgevision::model::frame {

    class FrameValidator final {
      public:
        [[nodiscard]] static FrameSubmissionResult validate(
            const Frame& frame,
            const FrameValidationContext& context
        );
    };

} // namespace edgevision::model::frame
