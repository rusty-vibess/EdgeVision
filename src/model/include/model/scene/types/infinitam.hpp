#pragma once

#include "ITMLib/ITMLibDefines.h"
#include "ITMLib/Objects/Scene/ITMScene.h"

namespace edgevision::model::scene {

    /// Concrete InfiniTAM scene type used by the model module.
    using InfiniTamScene = ITMLib::ITMScene<ITMVoxel, ITMVoxelIndex>;

} // namespace edgevision::model::scene
