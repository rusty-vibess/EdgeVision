#include "streaming/webrtc/PipelineBuilder.hpp"

#include <gst/app/gstappsrc.h>
#include <iostream>
#include <sstream>

namespace edgevision::streaming::webrtc {

    namespace {

        std::string makeBranchString(
            const StreamConfig& cfg, const std::string& name, std::uint32_t pt
        ) {
            std::ostringstream oss;
            oss
                << "appsrc name=" << name << " is-live=true do-timestamp=true format=time"
                << " caps=\"video/x-raw,format=BGR,width=" << cfg.width
                << ",height=" << cfg.height
                << ",framerate=" << cfg.fps << "/1\""
                << " ! queue max-size-buffers=2 leaky=downstream"
                << " ! videoconvert ! video/x-raw,format=I420"
                << " ! nvvidconv ! video/x-raw(memory:NVMM),format=NV12"
                << " ! nvv4l2h264enc bitrate=" << cfg.bitrateBps
                << " maxperf-enable=true preset-level=4 profile=0"
                << " insert-sps-pps=true insert-aud=true idrinterval=" << cfg.fps
                << " ! h264parse config-interval=-1"
                << " ! rtph264pay pt=" << pt << " config-interval=-1"
                << " ! application/x-rtp,media=video,encoding-name=H264,payload=" << pt
                << " ! sendrecv.";
            return oss.str();
        }

    } // namespace

    std::string pipelineString(const StreamConfig& cfg) {
        std::ostringstream oss;
        oss << makeBranchString(cfg, "src_rgb", 96)
            << " " << makeBranchString(cfg, "src_tsdf", 97)
            << " webrtcbin name=sendrecv stun-server=" << cfg.stunServer
            << " bundle-policy=max-bundle";
        return oss.str();
    }

    PipelineHandles buildPipeline(const StreamConfig& cfg) {
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
        h.rgbAppSrc = gst_bin_get_by_name(GST_BIN(h.pipeline), "src_rgb");
        h.tsdfAppSrc = gst_bin_get_by_name(GST_BIN(h.pipeline), "src_tsdf");
        // The branch elements after `nvv4l2h264enc` aren't named; for pause
        // we control the appsrc directly (sending EOS on pause is too
        // disruptive — instead we just stop pushing frames). Set both
        // branchBin handles to nullptr; pause is implemented by the
        // FrameSource side (see WebRtcServer.cpp).
        h.rgbBranchBin = nullptr;
        h.tsdfBranchBin = nullptr;
        return h;
    }

    void setBranchPaused(GstElement* branch, bool paused) {
        if (branch == nullptr) {
            return;
        }
        gst_element_set_state(
            branch, paused ? GST_STATE_PAUSED : GST_STATE_PLAYING
        );
    }

} // namespace edgevision::streaming::webrtc
