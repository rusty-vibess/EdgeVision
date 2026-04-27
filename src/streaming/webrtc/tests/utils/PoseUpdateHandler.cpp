#include "streaming/webrtc/utils/PoseUpdateHandler.hpp"

#include <gtest/gtest.h>

#include "model/frame/types/camera.hpp"
#include "model/frame/types/image.hpp"

namespace {

    using edgevision::model::frame::CameraIntrinsics;
    using edgevision::model::frame::ImageSize;
    using edgevision::model::viewer::ViewerPose;
    using edgevision::streaming::webrtc::PoseUpdate;
    using edgevision::streaming::webrtc::utils::makeUpdatedViewerPose;
    using edgevision::streaming::webrtc::utils::parsePoseUpdateMessage;

    [[nodiscard]] ViewerPose makeViewerPose() {
        ViewerPose viewerPose{};
        viewerPose.pose.matrix = {
            1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.1f,
            0.0f,
            0.0f,
            1.0f,
        };
        viewerPose.intrinsics = CameraIntrinsics{525.0f, 526.0f, 4.0f, 3.0f};
        viewerPose.imageSize = ImageSize{8, 6};
        viewerPose.generation = 7;
        return viewerPose;
    }

    [[nodiscard]] PoseUpdate makePoseUpdate() {
        PoseUpdate poseUpdate{};
        poseUpdate.matrix = {
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            -1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.2f,
            0.1f,
            0.0f,
            1.0f,
        };
        return poseUpdate;
    }

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

TEST(PoseUpdateHandler, ReturnsNoUpdateWithoutSeededViewerPose) {
    const auto updatedPose = makeUpdatedViewerPose(std::nullopt, makePoseUpdate());

    EXPECT_FALSE(updatedPose.has_value());
}

TEST(PoseUpdateHandler, ReturnsNoUpdateWhenMatrixIsUnchanged) {
    const ViewerPose currentPose = makeViewerPose();
    PoseUpdate poseUpdate{};
    poseUpdate.matrix = currentPose.pose.matrix;

    const auto updatedPose = makeUpdatedViewerPose(currentPose, poseUpdate);

    EXPECT_FALSE(updatedPose.has_value());
}

TEST(PoseUpdateHandler, ReturnsUpdatedViewerPoseWhenMatrixChanges) {
    const ViewerPose currentPose = makeViewerPose();
    const PoseUpdate poseUpdate = makePoseUpdate();

    const auto updatedPose = makeUpdatedViewerPose(currentPose, poseUpdate);

    ASSERT_TRUE(updatedPose.has_value());
    EXPECT_EQ(updatedPose->pose.matrix, poseUpdate.matrix);
}

TEST(PoseUpdateHandler, PreservesIntrinsicsAndImageSizeWhenMatrixChanges) {
    const ViewerPose currentPose = makeViewerPose();
    const PoseUpdate poseUpdate = makePoseUpdate();

    const auto updatedPose = makeUpdatedViewerPose(currentPose, poseUpdate);

    ASSERT_TRUE(updatedPose.has_value());
    EXPECT_EQ(updatedPose->intrinsics.fx, currentPose.intrinsics.fx);
    EXPECT_EQ(updatedPose->intrinsics.fy, currentPose.intrinsics.fy);
    EXPECT_EQ(updatedPose->intrinsics.cx, currentPose.intrinsics.cx);
    EXPECT_EQ(updatedPose->intrinsics.cy, currentPose.intrinsics.cy);
    EXPECT_EQ(updatedPose->imageSize, currentPose.imageSize);
    EXPECT_EQ(updatedPose->generation, currentPose.generation);
}
