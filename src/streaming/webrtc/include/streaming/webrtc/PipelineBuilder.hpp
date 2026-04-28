#pragma once

#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>
#include <string>

#include "config/types/image.hpp"
#include "config/types/streaming.hpp"

namespace edgevision::streaming::webrtc {

    struct PipelineHandles {
        GstElement* pipeline = nullptr;
        GstElement* webrtcbin = nullptr;
        GstElement* videoAppSrc = nullptr;
    };

    /// Constructs the gstreamer pipeline, returns owned handles. Caller
    /// must `gst_object_unref(pipeline)` on shutdown — the others are
    /// children and freed transitively.
    [[nodiscard]] PipelineHandles buildPipeline(
        const edgevision::config::WebRtcStreamingConfig& cfg,
        const edgevision::config::ImageSize& imageSize
    );

    /// Returns the launch-style pipeline string for logging / debugging.
    [[nodiscard]] std::string pipelineString(
        const edgevision::config::WebRtcStreamingConfig& cfg,
        const edgevision::config::ImageSize& imageSize
    );

} // namespace edgevision::streaming::webrtc
