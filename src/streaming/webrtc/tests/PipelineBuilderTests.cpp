#include <gtest/gtest.h>

#include "streaming/webrtc/PipelineBuilder.hpp"

using edgevision::config::WebRtcStreamingConfig;
using edgevision::streaming::webrtc::pipelineString;

TEST(PipelineBuilder, ContainsViewerOutputBranchAndWebrtcbin) {
    WebRtcStreamingConfig cfg;
    cfg.width = 854;
    cfg.height = 480;
    cfg.fps = 30;
    cfg.bitrateBps = 1500000;
    const auto s = pipelineString(cfg);
    EXPECT_NE(s.find("appsrc name=src_video"), std::string::npos);
    EXPECT_NE(s.find("webrtcbin name=sendrecv"), std::string::npos);
    EXPECT_NE(s.find("format=RGB"), std::string::npos);
    EXPECT_NE(s.find("x264enc"), std::string::npos);
    EXPECT_NE(s.find("bitrate=1500"), std::string::npos);
    EXPECT_NE(s.find("tune=zerolatency"), std::string::npos);
    EXPECT_NE(s.find("threads=2"), std::string::npos);
}

TEST(PipelineBuilder, OmitsLegacyBranchNames) {
    WebRtcStreamingConfig cfg;
    const auto s = pipelineString(cfg);
    EXPECT_EQ(s.find("appsrc name=src_rgb"), std::string::npos);
    EXPECT_EQ(s.find("appsrc name=src_tsdf"), std::string::npos);
}

TEST(PipelineBuilder, FrameRateAndIdrInterval) {
    WebRtcStreamingConfig cfg;
    cfg.fps = 30;
    const auto s = pipelineString(cfg);
    EXPECT_NE(s.find("framerate=30/1"), std::string::npos);
    EXPECT_NE(s.find("key-int-max=30"), std::string::npos);
}
