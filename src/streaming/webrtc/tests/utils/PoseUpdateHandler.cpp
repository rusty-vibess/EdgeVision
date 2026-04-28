#include "streaming/webrtc/utils/PoseUpdateHandler.hpp"

#include <gtest/gtest.h>

namespace {

    using edgevision::streaming::webrtc::utils::parsePoseUpdateMessage;

} // namespace

TEST(PoseUpdateHandler, ParsesMatrixPayload) {
    constexpr std::string_view payload =
        R"({"type":"pose","matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0.1,0,0,1]})";

    const auto poseUpdate = parsePoseUpdateMessage(payload);

    ASSERT_TRUE(poseUpdate.has_value());
    EXPECT_FLOAT_EQ(poseUpdate->matrix[12], 0.1f);
    EXPECT_FLOAT_EQ(poseUpdate->matrix[15], 1.0f);
}

TEST(PoseUpdateHandler, ParsesLegacyPosePayload) {
    constexpr std::string_view payload =
        R"({"type":"pose","pose":[1,0,0,0,0,1,0,0,0,0,1,0,0.2,0,0,1]})";

    const auto poseUpdate = parsePoseUpdateMessage(payload);

    ASSERT_TRUE(poseUpdate.has_value());
    EXPECT_FLOAT_EQ(poseUpdate->matrix[12], 0.2f);
}

TEST(PoseUpdateHandler, IgnoresExtraSizingFields) {
    constexpr std::string_view payload =
        R"({"type":"pose","matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0.3,0,0,1],"width":1920,"height":1080,"fov_x":1.0,"fov_y":0.8})";

    const auto poseUpdate = parsePoseUpdateMessage(payload);

    ASSERT_TRUE(poseUpdate.has_value());
    EXPECT_FLOAT_EQ(poseUpdate->matrix[12], 0.3f);
}

TEST(PoseUpdateHandler, RejectsInvalidJson) {
    constexpr std::string_view payload = R"({"type":"pose","matrix":[1,2,3])";

    const auto poseUpdate = parsePoseUpdateMessage(payload);

    EXPECT_FALSE(poseUpdate.has_value());
}

TEST(PoseUpdateHandler, RejectsMissingMatrix) {
    constexpr std::string_view payload = R"({"type":"pose"})";

    const auto poseUpdate = parsePoseUpdateMessage(payload);

    EXPECT_FALSE(poseUpdate.has_value());
}

TEST(PoseUpdateHandler, RejectsWrongSizedMatrix) {
    constexpr std::string_view payload = R"({"type":"pose","matrix":[1,0,0]})";

    const auto poseUpdate = parsePoseUpdateMessage(payload);

    EXPECT_FALSE(poseUpdate.has_value());
}

TEST(PoseUpdateHandler, IgnoresNonPoseMessage) {
    constexpr std::string_view payload =
        R"({"type":"hello","matrix":[1,0,0,0,0,1,0,0,0,0,1,0,0.1,0,0,1]})";

    const auto poseUpdate = parsePoseUpdateMessage(payload);

    EXPECT_FALSE(poseUpdate.has_value());
}
