#pragma once

#include "types/HybridTypes.hpp"

struct ReconstructionPipelineConfig {
    ResidualSeedConfig residualSeedConfig{};
    // Placeholder cadence for the later online optimization loop.
    int localOptimizationInterval = 1;
    bool optimizeWithKeyframes = true;
    bool includeCurrentObservationInOptimization = true;
};
