#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "builder/state/SceneBuilder.hpp"

namespace edgevision::viewer::tests {

    struct PopulatedSceneData {
        edgevision::model::frame::Frame frame{};
        edgevision::model::scene::SceneVersion sceneVersion{};
    };

    inline edgevision::model::frame::FrameBuffer makeBuffer(std::vector<std::byte> bytes) {
        auto storage = std::make_shared<std::vector<std::byte>>(std::move(bytes));
        const std::shared_ptr<const void> owner(storage, storage.get());
        return edgevision::model::frame::FrameBuffer(storage->data(), storage->size(), owner);
    }

    inline edgevision::model::frame::Frame makeValidFrame(
        edgevision::model::frame::FrameId frameId,
        std::int64_t timestampTicks
    ) {
        using namespace edgevision::model::frame;

        constexpr int width = 16;
        constexpr int height = 12;

        std::vector<std::byte> rgbStorage(width * height * 4);
        for (std::size_t index = 0; index < rgbStorage.size(); ++index) {
            rgbStorage[index] = static_cast<std::byte>(index % 255);
        }

        std::vector<std::byte> depthStorage(width * height * 2);
        for (int index = 0; index < width * height; ++index) {
            const std::uint16_t depthMillimetres = 1000;
            std::memcpy(
                depthStorage.data() + static_cast<std::size_t>(index) * sizeof(std::uint16_t),
                &depthMillimetres,
                sizeof(depthMillimetres)
            );
        }

        Frame frame{};
        frame.frameId = frameId;
        frame.timestamp = FrameTimestamp{timestampTicks};
        frame.rgb.size = ImageSize{width, height};
        frame.rgb.strideBytes = width * 4;
        frame.rgb.buffer = makeBuffer(std::move(rgbStorage));
        frame.depth.size = ImageSize{width, height};
        frame.depth.strideBytes = width * 2;
        frame.depth.buffer = makeBuffer(std::move(depthStorage));
        frame.intrinsics = CameraIntrinsics{525.0f, 525.0f, width / 2.0f, height / 2.0f};
        return frame;
    }

    inline std::optional<PopulatedSceneData> populateScene(
        edgevision::model::scene::SharedScene& sharedScene,
        edgevision::model::scene::SceneVersionStore& sceneVersionStore
    ) {
        builder::SceneBuilder builder(sharedScene, sceneVersionStore);
        PopulatedSceneData data{};
        data.frame = makeValidFrame(1, 100);

        const builder::FrameBuildResult result = builder.buildFrame(data.frame);
        if (result.status != builder::FrameBuildStatus::FrameConsumed) {
            return std::nullopt;
        }

        const std::optional<edgevision::model::scene::SceneVersion> sceneVersion =
            sceneVersionStore.latest();
        if (!sceneVersion.has_value()) {
            return std::nullopt;
        }

        data.sceneVersion = *sceneVersion;
        return data;
    }

    inline edgevision::model::viewer::ViewerPose makeViewerPose(
        const PopulatedSceneData& data,
        edgevision::model::viewer::ViewerPoseGeneration generation
    ) {
        edgevision::model::viewer::ViewerPose viewerPose{};
        viewerPose.pose = data.sceneVersion.cameraToWorld;
        viewerPose.intrinsics = data.frame.intrinsics;
        viewerPose.imageSize = data.frame.rgb.size;
        viewerPose.generation = generation;
        return viewerPose;
    }

} // namespace edgevision::viewer::tests
