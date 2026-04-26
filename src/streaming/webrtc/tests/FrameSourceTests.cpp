#include "streaming/webrtc/FrameSource.hpp"
#include "streaming/webrtc/types/PoseUpdate.hpp"

#include <gtest/gtest.h>

// Minimal stub — TsdfFrameSource stores a reference but never dereferences it.
namespace edgevision::model::scene { class SharedScene {}; }

using namespace edgevision::streaming::webrtc;

TEST(TsdfFrameSource, NullRendererReturnsFalse) {
    edgevision::model::scene::SharedScene scene;
    TsdfFrameSource src(scene, nullptr);
    std::vector<std::uint8_t> out;
    EXPECT_FALSE(src.render(PoseUpdate{}, out));
}

TEST(TsdfFrameSource, RendererCalledWithCorrectPose) {
    edgevision::model::scene::SharedScene scene;
    PoseUpdate captured{};
    TsdfFrameSource src(scene, [&](const PoseUpdate& p, std::vector<std::uint8_t>& buf) {
        captured = p;
        buf.assign(static_cast<std::size_t>(p.width) * p.height * 3u, 42u);
        return true;
    });

    PoseUpdate pose;
    pose.seq = 7;
    pose.width = 320;
    pose.height = 240;
    std::vector<std::uint8_t> out;
    EXPECT_TRUE(src.render(pose, out));
    EXPECT_EQ(captured.seq, 7u);
    EXPECT_EQ(out.size(), 320u * 240u * 3u);
    EXPECT_EQ(out[0], 42u);
}

TEST(TsdfFrameSource, SetRendererReplacesExisting) {
    edgevision::model::scene::SharedScene scene;
    TsdfFrameSource src(scene, nullptr);

    std::vector<std::uint8_t> out;
    EXPECT_FALSE(src.render(PoseUpdate{}, out));

    src.setRenderer([](const PoseUpdate&, std::vector<std::uint8_t>& buf) {
        buf = {1, 2, 3};
        return true;
    });
    EXPECT_TRUE(src.render(PoseUpdate{}, out));
    EXPECT_EQ(out, (std::vector<std::uint8_t>{1, 2, 3}));
}

TEST(TsdfFrameSource, RendererReturnFalsePassedThrough) {
    edgevision::model::scene::SharedScene scene;
    TsdfFrameSource src(scene, [](const PoseUpdate&, std::vector<std::uint8_t>&) {
        return false;
    });
    std::vector<std::uint8_t> out;
    EXPECT_FALSE(src.render(PoseUpdate{}, out));
}
