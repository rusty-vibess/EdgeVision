#include "streaming/webrtc/PipelineBuilder.hpp"

#include <gtest/gtest.h>

using edgevision::streaming::webrtc::pipelineString;
using edgevision::streaming::webrtc::StreamConfig;

TEST(PipelineBuilder, ContainsBothBranchesAndWebrtcbin) {
    StreamConfig cfg;
    cfg.width = 854;
    cfg.height = 480;
    cfg.fps = 30;
    cfg.bitrateBps = 1500000;
    const auto s = pipelineString(cfg);
    EXPECT_NE(s.find("appsrc name=src_rgb"), std::string::npos);
    EXPECT_NE(s.find("appsrc name=src_tsdf"), std::string::npos);
    EXPECT_NE(s.find("webrtcbin name=sendrecv"), std::string::npos);
    EXPECT_NE(s.find("nvv4l2h264enc"), std::string::npos);
    EXPECT_NE(s.find("bitrate=1500000"), std::string::npos);
}

TEST(PipelineBuilder, FrameRateAndIdrInterval) {
    StreamConfig cfg;
    cfg.fps = 30;
    const auto s = pipelineString(cfg);
    EXPECT_NE(s.find("framerate=30/1"), std::string::npos);
    EXPECT_NE(s.find("idrinterval=30"), std::string::npos);
}
