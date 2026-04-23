#include "scene/state/SharedSceneState.hpp"

#include <memory>

namespace edgevision::model::scene {

    SharedSceneState::SharedSceneState(SceneReadPolicy readPolicy) : readPolicy(readPolicy) {
        scene = std::make_unique<InfiniTamScene>(
            &settings.sceneParams,
            settings.swappingMode == ITMLib::ITMLibSettings::SWAPPINGMODE_ENABLED,
            settings.GetMemoryType()
        );
    }

} // namespace edgevision::model::scene
