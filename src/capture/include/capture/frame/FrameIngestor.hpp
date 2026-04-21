#pragma once

#include <cstdint>

#include "capture/camera/CameraCapture.hpp"
#include "capture/frame/K4aFrameBuilder.hpp"
#include "capture/frame/types/ingest_result.hpp"
#include "capture/interfaces/K4aInterface.hpp"
#include "model/frame/FrameStore.hpp"
#include "model/frame/types/identity.hpp"

namespace edgevision::capture::frame {

    class FrameIngestor final {
      public:
        FrameIngestor(
            CameraCapture& capture,
            edgevision::model::frame::FrameStore& frameStore,
            const K4aInterface& api = K4aInterface::instance()
        );

        [[nodiscard]] FrameIngestResult captureAndSubmit(int32_t timeoutMs = 0);

      private:
        [[nodiscard]] edgevision::model::frame::FrameTimestamp nextTimestamp();

        CameraCapture& m_capture;
        edgevision::model::frame::FrameStore& m_frameStore;
        const K4aInterface& m_api;
        K4aFrameBuilder m_builder;
        edgevision::model::frame::FrameId m_nextFrameId = 1;
        std::int64_t m_lastTimestampTicks = 0;
    };

} // namespace edgevision::capture::frame
