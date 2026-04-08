#pragma once

#include "types/OptimizationObservationSet.hpp"

class IReconstructionPipeline {
  public:
    virtual ~IReconstructionPipeline();

    [[nodiscard]] virtual ReconstructionObservation prepareObservation(
        const ReconstructionObservation& observation
    ) const = 0;

    [[nodiscard]] virtual OptimizationObservationSet prepareObservationSet(
        const OptimizationObservationSet& observationSet
    ) const = 0;

    virtual void seedCurrentObservation(const ReconstructionObservation& observation) = 0;

    [[nodiscard]] virtual float optimizeObservationSet(
        const OptimizationObservationSet& observationSet
    ) = 0;

    [[nodiscard]] virtual float runOptimizationCycle(
        const OptimizationObservationSet& observationSet
    ) = 0;
};
