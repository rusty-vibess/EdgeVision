#pragma once

#include <memory>
#include <shared_mutex>

#include "ITMLib/Utils/ITMLibSettings.h"
#include "model/scene/types/access.hpp"
#include "model/scene/types/infinitam.hpp"

namespace edgevision::model::scene {

    class SharedSceneState final {
      public:
        SharedSceneState();

        std::shared_mutex mutex{};
        SceneVersionId sceneVersionId = 0;
        ITMLib::ITMLibSettings settings{};
        std::unique_ptr<InfiniTamScene> scene{};
    };

} // namespace edgevision::model::scene
