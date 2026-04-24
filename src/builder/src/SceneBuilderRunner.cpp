#include "builder/SceneBuilderRunner.hpp"

#include <memory>
#include <utility>

#include "builder/state/SceneBuilder.hpp"

namespace edgevision::builder {

    SceneBuilderRunner::SceneBuilderRunner(
        edgevision::model::frame::FrameStore& frameStore,
        edgevision::model::scene::SharedScene& sharedScene,
        edgevision::model::scene::SceneVersionStore& sceneVersionStore
    )
        : m_builder(
              std::make_unique<SceneBuilder>(frameStore, sharedScene, sceneVersionStore)
          ) {}

    SceneBuilderRunner::~SceneBuilderRunner() = default;
    SceneBuilderRunner::SceneBuilderRunner(SceneBuilderRunner&& other) noexcept = default;
    SceneBuilderRunner& SceneBuilderRunner::operator=(SceneBuilderRunner&& other) noexcept =
        default;

    void SceneBuilderRunner::run() {
        m_builder->run();
    }

    void SceneBuilderRunner::requestStop() {
        m_builder->requestStop();
    }

} // namespace edgevision::builder
