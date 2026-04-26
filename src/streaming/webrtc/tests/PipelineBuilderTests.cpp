#include "streaming/webrtc/PipelineBuilder.hpp"

#include <gtest/gtest.h>

using edgevision::streaming::webrtc::pipelineString;
using edgevision::streaming::webrtc::StreamConfig;

TEST(PipelineBuilder, ContainsBothBranchesAndWebrtcbin) {
    StreamConfig cfg;
    cfg.enableRgb = true;
    cfg.width = 854;
    cfg.height = 480;
    cfg.fps = 30;
    cfg.bitrateBps = 1500000;
    const auto s = pipelineString(cfg);
    EXPECT_NE(s.find("appsrc name=src_rgb"), std::string::npos);
    EXPECT_NE(s.find("appsrc name=src_tsdf"), std::string::npos);
    EXPECT_NE(s.find("webrtcbin name=sendrecv"), std::string::npos);
    EXPECT_NE(s.find("x264enc"), std::string::npos);
    EXPECT_NE(s.find("bitrate=1500"), std::string::npos);
    EXPECT_NE(s.find("tune=zerolatency"), std::string::npos);
    EXPECT_NE(s.find("threads=2"), std::string::npos);
}

TEST(PipelineBuilder, TsdfOnlyOmitsRgbBranch) {
    StreamConfig cfg;
    cfg.enableRgb = false;
    const auto s = pipelineString(cfg);
    EXPECT_EQ(s.find("appsrc name=src_rgb"), std::string::npos);
    EXPECT_NE(s.find("appsrc name=src_tsdf"), std::string::npos);
    EXPECT_NE(s.find("webrtcbin name=sendrecv"), std::string::npos);
}

TEST(PipelineBuilder, DefaultsToTsdfOnlyForOrinNano) {
    StreamConfig cfg;  // defaults: enableRgb=false
    const auto s = pipelineString(cfg);
    EXPECT_EQ(s.find("appsrc name=src_rgb"), std::string::npos);
    EXPECT_NE(s.find("appsrc name=src_tsdf"), std::string::npos);
}

TEST(PipelineBuilder, FrameRateAndIdrInterval) {
    StreamConfig cfg;
    cfg.fps = 30;
    const auto s = pipelineString(cfg);
    EXPECT_NE(s.find("framerate=30/1"), std::string::npos);
    EXPECT_NE(s.find("idrinterval=30"), std::string::npos);
}
