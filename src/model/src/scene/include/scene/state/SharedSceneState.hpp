#pragma once

#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>

#include "ITMLib/Utils/ITMLibSettings.h"
#include "model/scene/types/access.hpp"
#include "model/scene/types/infinitam.hpp"

namespace edgevision::model::scene {

    class SharedSceneState final {
      public:
        explicit SharedSceneState(SceneReadPolicy readPolicy);

        std::mutex mutex{};
        std::condition_variable condition{};
        std::size_t activeReaderCount = 0;
        std::size_t waitingWriterCount = 0;
        bool writerActive = false;
        bool blockedPriorityReadPending = false;
        SceneReadPolicy readPolicy = SceneReadPolicy::Greedy;
        SceneVersionId sceneVersionId = 0;
        ITMLib::ITMLibSettings settings{};
        std::unique_ptr<InfiniTamScene> scene{};
    };

} // namespace edgevision::model::scene
