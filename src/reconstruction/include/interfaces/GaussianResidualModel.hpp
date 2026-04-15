#pragma once

#include "types/HybridTypes.hpp"
#include "types/OptimizationObservationSet.hpp"

class IGaussianResidualModel {
  public:
    virtual ~IGaussianResidualModel();

    virtual void seedFromResidual(
        const ReconstructionObservation& observation,
        const ResidualSeedConfig& config
    ) = 0;

    [[nodiscard]] virtual HybridRenderOutput renderHybrid(const HybridRenderInput& input) = 0;

    [[nodiscard]] virtual float optimizeObservationSet(
        const OptimizationObservationSet& observationSet
    ) = 0;
};
