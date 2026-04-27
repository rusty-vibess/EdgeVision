#include "streaming/webrtc/PipelineBuilder.hpp"

#include <gst/app/gstappsrc.h>
#include <iostream>
#include <sstream>

namespace edgevision::streaming::webrtc {

    namespace {

        std::string makeVideoBranchString(const edgevision::config::WebRtcStreamingConfig& cfg) {
            // Software x264 encode: Orin Nano has no NVENC. Pin to cores 4-5
            // (WebRtcStreamingConfig.pumpCoreMask) and keep `threads=2` to fit
            // the budget. x264enc bitrate is in kbps, not bps.
            const std::uint32_t kbps = cfg.bitrateBps / 1000u;
            std::ostringstream oss;
            oss << "appsrc name=src_video is-live=true do-timestamp=true format=time"
                << " caps=\"video/x-raw,format=RGB,width=" << cfg.width << ",height=" << cfg.height
                << ",framerate=" << cfg.fps << "/1\""
                << " ! queue max-size-buffers=2 leaky=downstream"
                << " ! videoconvert ! video/x-raw,format=I420"
                << " ! x264enc tune=zerolatency speed-preset=ultrafast"
                << " bitrate=" << kbps << " threads=2"
                << " key-int-max=" << cfg.fps << " ! video/x-h264,profile=baseline"
                << " ! h264parse config-interval=-1"
                << " ! rtph264pay pt=96 config-interval=-1"
                << " ! application/x-rtp,media=video,encoding-name=H264,payload=96"
                << " ! sendrecv.";
            return oss.str();
        }

    } // namespace

    std::string pipelineString(const edgevision::config::WebRtcStreamingConfig& cfg) {
        std::ostringstream oss;
        oss << makeVideoBranchString(cfg)
            << " webrtcbin name=sendrecv stun-server=" << cfg.stunServer
            << " bundle-policy=max-bundle";
        return oss.str();
    }

    PipelineHandles buildPipeline(const edgevision::config::WebRtcStreamingConfig& cfg) {
        PipelineHandles h{};
        const auto desc = pipelineString(cfg);

        GError* err = nullptr;
        h.pipeline = gst_parse_launch(desc.c_str(), &err);
        if (err != nullptr) {
            std::cerr << "gst_parse_launch error: " << err->message << std::endl;
            g_error_free(err);
            if (h.pipeline != nullptr) {
                gst_object_unref(h.pipeline);
                h.pipeline = nullptr;
            }
            return h;
        }

        h.webrtcbin = gst_bin_get_by_name(GST_BIN(h.pipeline), "sendrecv");
        h.videoAppSrc = gst_bin_get_by_name(GST_BIN(h.pipeline), "src_video");
        return h;
    }

} // namespace edgevision::streaming::webrtc
