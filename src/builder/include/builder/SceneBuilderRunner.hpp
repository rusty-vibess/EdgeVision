#pragma once

#include <memory>

namespace edgevision::model::frame {
    class FrameStore;
}

namespace edgevision::model::scene {
    class SceneVersionStore;
    class SharedScene;
} // namespace edgevision::model::scene

namespace edgevision::builder {

    class SceneBuilder;

    class SceneBuilderRunner final {
      public:
        SceneBuilderRunner(
            model::frame::FrameStore& frameStore,
            model::scene::SharedScene& sharedScene,
            model::scene::SceneVersionStore& sceneVersionStore
        );
        ~SceneBuilderRunner();

        SceneBuilderRunner(SceneBuilderRunner&& other) noexcept;
        SceneBuilderRunner& operator=(SceneBuilderRunner&& other) noexcept;

        SceneBuilderRunner(const SceneBuilderRunner&) = delete;
        SceneBuilderRunner& operator=(const SceneBuilderRunner&) = delete;

        void run();
        void requestStop();

      private:
        std::unique_ptr<SceneBuilder> m_builder{};
    };

} // namespace edgevision::builder
