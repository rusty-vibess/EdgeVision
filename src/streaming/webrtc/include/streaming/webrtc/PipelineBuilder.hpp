#pragma once

#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>
#include <memory>
#include <string>

#include "streaming/webrtc/types/StreamConfig.hpp"

namespace edgevision::streaming::webrtc {

    struct PipelineHandles {
        GstElement* pipeline = nullptr;
        GstElement* webrtcbin = nullptr;
        GstElement* rgbAppSrc = nullptr;
        GstElement* tsdfAppSrc = nullptr;
        GstElement* rgbBranchBin = nullptr;   // for pause/resume
        GstElement* tsdfBranchBin = nullptr;
    };

    /// Constructs the gstreamer pipeline, returns owned handles. Caller
    /// must `gst_object_unref(pipeline)` on shutdown — the others are
    /// children and freed transitively.
    [[nodiscard]] PipelineHandles buildPipeline(const StreamConfig& cfg);

    /// Pause / resume one of the encode branches by name. Used on
    /// view_mode change (rgb-only mode pauses the tsdf branch, etc.).
    void setBranchPaused(GstElement* branch, bool paused);

    /// Returns the launch-style pipeline string for logging / debugging.
    [[nodiscard]] std::string pipelineString(const StreamConfig& cfg);

} // namespace edgevision::streaming::webrtc
