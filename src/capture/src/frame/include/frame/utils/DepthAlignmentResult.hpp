#pragma once

#include <optional>

#include "capture/frame/types/build_result.hpp"
#include "frame/resources/ImageHandle.hpp"

namespace edgevision::capture::frame {

    struct DepthAlignmentResult {
        std::optional<FrameBuildResult> failure{};
        std::optional<ImageHandle> image{};

        [[nodiscard]] bool aligned() const;
    };

    [[nodiscard]] DepthAlignmentResult makeDepthAlignmentSuccess(ImageHandle image);

    [[nodiscard]] DepthAlignmentResult makeDepthAlignmentFailure(FrameBuildResult result);

} // namespace edgevision::capture::frame
