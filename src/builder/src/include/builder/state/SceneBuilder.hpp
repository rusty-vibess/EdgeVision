#pragma once

#include <atomic>

#include "builder/state/InfiniTamBuilderContext.hpp"
#include "builder/types/BuildStepResult.hpp"
#include "builder/types/SceneBuilderConfig.hpp"
#include "model/frame/FrameStore.hpp"
#include "model/scene/SceneVersionStore.hpp"
#include "model/scene/SharedScene.hpp"

namespace edgevision::builder {

    class SceneBuilder final {
      public:
        SceneBuilder(
            edgevision::model::frame::FrameStore& frameStore,
            edgevision::model::scene::SharedScene& sharedScene,
            edgevision::model::scene::SceneVersionStore& sceneVersionStore,
            const SceneBuilderConfig& config = {}
        );

        void run();
        void requestStop();

        [[nodiscard]] BuildStepResult tryProcessNextFrame();
        [[nodiscard]] BuildStepResult processFrame(edgevision::model::frame::Frame frame);

      private:
        [[nodiscard]] edgevision::model::scene::SceneVersion makeSceneVersion(
            const edgevision::model::frame::Frame& frame,
            edgevision::model::scene::SceneVersionId versionId,
            edgevision::model::scene::TrackingStatus trackingStatus,
            edgevision::model::scene::IntegrationStatus integrationStatus
        ) const;

        edgevision::model::frame::FrameStore& m_frameStore;
        edgevision::model::scene::SharedScene& m_sharedScene;
        edgevision::model::scene::SceneVersionStore& m_sceneVersionStore;
        InfiniTamBuilderContext m_context;
        std::atomic<bool> m_stopRequested = false;
    };

} // namespace edgevision::builder
