#include "frame/utils/DepthAlignmentResult.hpp"

#include <utility>

namespace edgevision::capture::frame {

    bool DepthAlignmentResult::aligned() const {
        return image.has_value() && !failure.has_value();
    }

    DepthAlignmentResult makeDepthAlignmentSuccess(ImageHandle image) {
        DepthAlignmentResult result{};
        result.image.emplace(std::move(image));
        return result;
    }

    DepthAlignmentResult makeDepthAlignmentFailure(FrameBuildResult result) {
        DepthAlignmentResult alignmentResult{};
        alignmentResult.failure = std::move(result);
        return alignmentResult;
    }

} // namespace edgevision::capture::frame
