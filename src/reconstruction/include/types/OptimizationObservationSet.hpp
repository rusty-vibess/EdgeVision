#pragma once

#include <vector>

#include "types/ReconstructionObservation.hpp"

struct OptimizationObservationSet {
    // The active observation used for residual seeding.
    ReconstructionObservation currentObservation{};
    bool hasCurrentObservation = false;
    bool includeCurrentObservationInOptimization = true;
    // Recent observations optimized as the local window.
    std::vector<ReconstructionObservation> localObservations{};
    // Optional older support observations sampled as keyframes.
    std::vector<ReconstructionObservation> keyframeObservations{};
};
