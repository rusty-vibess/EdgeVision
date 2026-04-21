#include "capture/frame/K4aFrameBuilder.hpp"

#include <utility>

#include "frame/resources/ImageHandle.hpp"
#include "frame/utils/FrameBuildResult.hpp"
#include "frame/utils/K4aDepthAligner.hpp"
#include "frame/utils/K4aFrameAssembler.hpp"
#include "frame/utils/K4aFrameValidator.hpp"

namespace edgevision::capture::frame {
    using edgevision::model::frame::CameraIntrinsics;
    using edgevision::model::frame::Frame;
    using edgevision::model::frame::FrameId;
    using edgevision::model::frame::FrameTimestamp;

    K4aFrameBuilder::K4aFrameBuilder(const K4aInterface& api) : m_api(api) {}

    FrameBuildResult K4aFrameBuilder::build(
        k4a_capture_t capture,
        const k4a_calibration_t& calibration,
        FrameId frameId,
        FrameTimestamp timestamp
    ) const {
        if (const auto failure = K4aFrameValidator::validateRequest(capture, frameId, timestamp)) {
            return *failure;
        }

        ImageHandle colorImage(m_api, m_api.captureGetColorImage(capture));
        if (const auto failure =
                K4aFrameValidator::validateColorImage(m_api, colorImage.get(), frameId)) {
            return *failure;
        }

        ImageHandle depthImage(m_api, m_api.captureGetDepthImage(capture));
        if (const auto failure =
                K4aFrameValidator::validateDepthImage(m_api, depthImage.get(), frameId)) {
            return *failure;
        }

        DepthAlignmentResult alignedDepth = K4aDepthAligner::alignDepthToColor(
            m_api, calibration, colorImage.get(), depthImage.get(), frameId
        );
        if (!alignedDepth.aligned()) {
            return *alignedDepth.failure;
        }
        if (const auto failure =
                K4aFrameValidator::validateDepthImage(m_api, alignedDepth.image->get(), frameId)) {
            return *failure;
        }

        const CameraIntrinsics intrinsics = K4aFrameAssembler::makeIntrinsics(calibration);
        if (const auto failure = K4aFrameValidator::validateIntrinsics(intrinsics, frameId)) {
            return *failure;
        }

        Frame frame = K4aFrameAssembler::assemble(
            m_api, colorImage.get(), alignedDepth.image->get(), intrinsics, frameId, timestamp
        );

        return makeFrameBuildSuccess(std::move(frame));
    }

} // namespace edgevision::capture::frame
