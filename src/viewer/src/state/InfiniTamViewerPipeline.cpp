#include "viewer/state/InfiniTamViewerPipeline.hpp"

#include <chrono>
#include <cstddef>
#include <cstring>
#include <memory>
#include <vector>

namespace edgevision::viewer {
    namespace {
        [[nodiscard]] ORUtils::MemoryBlock<Vector4u>::MemoryCopyDirection makeCopyDirection(
            const ITMLib::ITMLibSettings& settings
        ) {
            return settings.deviceType == ITMLib::ITMLibSettings::DEVICE_CUDA
                ? ORUtils::MemoryBlock<Vector4u>::CUDA_TO_CPU
                : ORUtils::MemoryBlock<Vector4u>::CPU_TO_CPU;
        }
    } // namespace

    InfiniTamViewerPipeline::InfiniTamViewerPipeline() = default;

    edgevision::model::viewer::RenderOutput InfiniTamViewerPipeline::render(
        const edgevision::model::scene::InfiniTamScene& scene,
        edgevision::model::scene::SceneVersionId sceneVersionId,
        const edgevision::model::viewer::ViewerPose& viewerPose
    ) {
        if (needsLayoutReset(viewerPose)) {
            resetLayout(scene, viewerPose);
        }

        const ORUtils::SE3Pose pose = makePose(viewerPose);
        const ITMLib::ITMIntrinsics intrinsics = makeIntrinsics(viewerPose);

        m_visualisationEngine->FindVisibleBlocks(&scene, &pose, &intrinsics, m_renderState.get());
        m_visualisationEngine->CreateExpectedDepths(
            &scene, &pose, &intrinsics, m_renderState.get()
        );
        m_visualisationEngine->RenderImage(
            &scene,
            &pose,
            &intrinsics,
            m_renderState.get(),
            m_renderState->raycastImage,
            ITMLib::IITMVisualisationEngine::RENDER_COLOUR_FROM_VOLUME
        );
        copyRenderToHost();

        edgevision::model::viewer::RenderOutput output{};
        output.rgb = makeRgbBuffer();
        output.imageSize = viewerPose.imageSize;
        output.poseGeneration = viewerPose.generation;
        output.sceneVersionId = sceneVersionId;
        output.renderTimestamp = std::chrono::steady_clock::now();
        return output;
    }

    bool InfiniTamViewerPipeline::needsLayoutReset(
        const edgevision::model::viewer::ViewerPose& viewerPose
    ) const {
        return !m_hasLayout || viewerPose.imageSize != m_renderImageSize;
    }

    void InfiniTamViewerPipeline::resetLayout(
        const edgevision::model::scene::InfiniTamScene& scene,
        const edgevision::model::viewer::ViewerPose& viewerPose
    ) {
        if (!m_visualisationEngine) {
            m_visualisationEngine.reset(
                ITMLib::ITMVisualisationEngineFactory::
                    MakeVisualisationEngine<ITMVoxel, ITMVoxelIndex>(m_settings.deviceType)
            );
        }

        const Vector2i renderSize(viewerPose.imageSize.width, viewerPose.imageSize.height);
        m_renderState.reset(
            ITMLib::ITMRenderStateFactory<ITMVoxelIndex>::CreateRenderState(
                renderSize, scene.sceneParams, m_settings.GetMemoryType()
            )
        );
        m_outputImage = std::make_unique<ITMUChar4Image>(renderSize, MEMORYDEVICE_CPU);

        m_renderImageSize = viewerPose.imageSize;
        m_hasLayout = true;
    }

    ITMLib::ITMIntrinsics InfiniTamViewerPipeline::makeIntrinsics(
        const edgevision::model::viewer::ViewerPose& viewerPose
    ) const {
        ITMLib::ITMIntrinsics intrinsics{};
        intrinsics.SetFrom(
            viewerPose.imageSize.width,
            viewerPose.imageSize.height,
            viewerPose.intrinsics.fx,
            viewerPose.intrinsics.fy,
            viewerPose.intrinsics.cx,
            viewerPose.intrinsics.cy
        );
        return intrinsics;
    }

    ORUtils::SE3Pose InfiniTamViewerPipeline::makePose(
        const edgevision::model::viewer::ViewerPose& viewerPose
    ) const {
        Matrix4f cameraToWorld{};
        std::memcpy(cameraToWorld.m, viewerPose.pose.matrix.data(), sizeof(cameraToWorld.m));

        ORUtils::SE3Pose pose{};
        pose.SetInvM(cameraToWorld);
        return pose;
    }

    void InfiniTamViewerPipeline::copyRenderToHost() {
        m_outputImage->SetFrom(m_renderState->raycastImage, makeCopyDirection(m_settings));
    }

    edgevision::model::frame::FrameBuffer InfiniTamViewerPipeline::makeRgbBuffer() const {
        const auto pixelCount = static_cast<std::size_t>(m_renderImageSize.width)
            * static_cast<std::size_t>(m_renderImageSize.height);

        auto storage = std::make_shared<std::vector<std::byte>>(pixelCount * 3);
        const Vector4u* source = m_outputImage->GetData(MEMORYDEVICE_CPU);
        for (std::size_t index = 0; index < pixelCount; ++index) {
            (*storage)[index * 3] = static_cast<std::byte>(source[index].r);
            (*storage)[index * 3 + 1] = static_cast<std::byte>(source[index].g);
            (*storage)[index * 3 + 2] = static_cast<std::byte>(source[index].b);
        }

        const std::shared_ptr<const void> owner(storage, storage.get());
        return edgevision::model::frame::FrameBuffer(storage->data(), storage->size(), owner);
    }

} // namespace edgevision::viewer
