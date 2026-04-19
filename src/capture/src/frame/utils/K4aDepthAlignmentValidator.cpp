#include "frame/utils/K4aDepthAlignmentValidator.hpp"

#include "frame/utils/FrameBuildResult.hpp"

namespace edgevision::capture::frame {

    std::optional<FrameBuildResult> K4aDepthAlignmentValidator::validateTransformationCreate(
        k4a_transformation_t transformation,
        edgevision::model::frame::FrameId frameId
    ) {
        if (transformation == nullptr) {
            return makeFrameBuildFailure(
                FrameBuildCode::TransformationCreateFailed,
                frameId,
                "K4A depth-to-color transformation could not be created"
            );
        }

        return std::nullopt;
    }

    std::optional<FrameBuildResult> K4aDepthAlignmentValidator::
        validateTransformedDepthImageCreate(
            k4a_result_t result,
            k4a_image_t image,
            edgevision::model::frame::FrameId frameId
        ) {
        if (result != K4A_RESULT_SUCCEEDED || image == nullptr) {
            return makeFrameBuildFailure(
                FrameBuildCode::TransformedDepthImageCreateFailed,
                frameId,
                "K4A transformed depth image could not be created"
            );
        }

        return std::nullopt;
    }

    std::optional<FrameBuildResult> K4aDepthAlignmentValidator::validateDepthAlignment(
        k4a_result_t result,
        edgevision::model::frame::FrameId frameId
    ) {
        if (result != K4A_RESULT_SUCCEEDED) {
            return makeFrameBuildFailure(
                FrameBuildCode::DepthAlignmentFailed,
                frameId,
                "K4A depth image could not be transformed into color camera geometry"
            );
        }

        return std::nullopt;
    }

} // namespace edgevision::capture::frame
