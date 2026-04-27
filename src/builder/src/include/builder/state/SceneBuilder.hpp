#pragma once

#include "builder/state/InfiniTamPipeline.hpp"
#include "builder/types/frame_build.hpp"
#include "config/types/builder.hpp"
#include "model/scene/SceneVersionStore.hpp"
#include "model/scene/SharedScene.hpp"

namespace edgevision::builder {

    class SceneBuilder final {
      public:
        SceneBuilder(
            edgevision::model::scene::SharedScene& sharedScene,
            edgevision::model::scene::SceneVersionStore& sceneVersionStore,
            const edgevision::config::BuilderRuntimeConfig& config = {}
        );

        /// Builds and publishes scene state for one already-dequeued frame.
        [[nodiscard]] FrameBuildResult buildFrame(edgevision::model::frame::Frame frame);

      private:
        [[nodiscard]] edgevision::model::scene::SceneVersion makeSceneVersion(
            const edgevision::model::frame::Frame& frame,
            edgevision::model::scene::SceneVersionId versionId,
            edgevision::model::scene::TrackingStatus trackingStatus,
            edgevision::model::scene::IntegrationStatus integrationStatus,
            const edgevision::model::scene::Pose4f& cameraToWorld
        ) const;

        edgevision::model::scene::SharedScene& m_sharedScene;
        edgevision::model::scene::SceneVersionStore& m_sceneVersionStore;
        InfiniTamPipeline m_pipeline;
    };

} // namespace edgevision::builder
