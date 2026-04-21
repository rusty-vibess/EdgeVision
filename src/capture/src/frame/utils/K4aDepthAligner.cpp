#include "frame/utils/K4aDepthAligner.hpp"

#include <utility>

#include "frame/resources/TransformationHandle.hpp"
#include "frame/utils/K4aDepthAlignmentValidator.hpp"

namespace edgevision::capture::frame {

    DepthAlignmentResult K4aDepthAligner::alignDepthToColor(
        const K4aInterface& api,
        const k4a_calibration_t& calibration,
        k4a_image_t colorImage,
        k4a_image_t depthImage,
        edgevision::model::frame::FrameId frameId
    ) {
        TransformationHandle transformation(api, api.transformationCreate(calibration));
        if (const auto failure = K4aDepthAlignmentValidator::validateTransformationCreate(
                transformation.get(), frameId
            )) {
            return makeDepthAlignmentFailure(*failure);
        }

        k4a_image_t transformedDepth = nullptr;
        const int width = api.imageGetWidthPixels(colorImage);
        const int height = api.imageGetHeightPixels(colorImage);
        const int strideBytes = width * 2;
        const k4a_result_t imageCreateResult = api.imageCreate(
            K4A_IMAGE_FORMAT_DEPTH16, width, height, strideBytes, &transformedDepth
        );
        if (const auto failure = K4aDepthAlignmentValidator::validateTransformedDepthImageCreate(
                imageCreateResult, transformedDepth, frameId
            )) {
            return makeDepthAlignmentFailure(*failure);
        }

        ImageHandle transformedDepthHandle(api, transformedDepth);
        const k4a_result_t alignmentResult = api.transformationDepthImageToColorCamera(
            transformation.get(), depthImage, transformedDepthHandle.get()
        );
        if (const auto failure =
                K4aDepthAlignmentValidator::validateDepthAlignment(alignmentResult, frameId)) {
            return makeDepthAlignmentFailure(*failure);
        }

        return makeDepthAlignmentSuccess(std::move(transformedDepthHandle));
    }

} // namespace edgevision::capture::frame
