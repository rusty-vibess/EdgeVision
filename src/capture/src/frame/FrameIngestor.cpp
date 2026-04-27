#include "frame/state/FrameIngestor.hpp"

#include <cstdint>
#include <utility>

#include "frame/resources/CaptureHandle.hpp"
#include "frame/utils/FrameIngestResult.hpp"
#include "frame/utils/MonotonicTimestamp.hpp"

namespace edgevision::capture::frame {
    using edgevision::model::frame::FrameId;
    using edgevision::model::frame::FrameSubmissionResult;
    using edgevision::model::frame::FrameTimestamp;

    FrameIngestor::FrameIngestor(
        CameraCapture& capture,
        edgevision::model::frame::FrameStore& frameStore,
        const K4aInterface& api
    )
        : m_capture(capture), m_frameStore(frameStore), m_api(api), m_builder(api) {}

    FrameIngestResult FrameIngestor::captureAndSubmit(int32_t timeoutMs) {
        k4a_capture_t rawCapture = nullptr;
        if (!m_capture.capture(rawCapture, timeoutMs) || rawCapture == nullptr) {
            return makeFrameIngestResult(
                FrameIngestCode::CaptureUnavailable, 0, "No K4A capture available"
            );
        }

        CaptureHandle captureHandle(m_api, rawCapture);

        k4a_calibration_t calibration{};
        if (!m_capture.getCalibration(calibration)) {
            return makeFrameIngestResult(
                FrameIngestCode::CalibrationUnavailable, 0, "K4A calibration is unavailable"
            );
        }

        const FrameId frameId = m_nextFrameId++;
        const FrameTimestamp timestamp = nextTimestamp();
        FrameBuildResult buildResult =
            m_builder.build(captureHandle.get(), calibration, frameId, timestamp);
        if (!buildResult.built()) {
            return makeBuildFailedResult(frameId, buildResult);
        }

        FrameSubmissionResult submissionResult =
            m_frameStore.submitFrame(std::move(*buildResult.frame));
        if (!submissionResult.accepted()) {
            return makeModelRejectedResult(submissionResult);
        }

        return makeSubmittedResult(submissionResult);
    }

    FrameTimestamp FrameIngestor::nextTimestamp() {
        return nextMonotonicTimestamp(m_lastTimestampTicks);
    }

} // namespace edgevision::capture::frame
