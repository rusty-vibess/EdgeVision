#pragma once

#include <string>

#include "types/TsdfTypes.hpp"

struct ReconstructionObservation {
    std::string observationId{};
    int sequenceIndex = -1;
    bool useCameraDepth = false;
    bool hasRaycast = false;
    RgbdFrameView frame{};
    // Populated once the TSDF raycast for this observation is available.
    TsdfRaycastBuffers raycast{};
};
