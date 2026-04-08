#pragma once

#include "interfaces/GaussianResidualModel.hpp"
#include "interfaces/ReconstructionPipeline.hpp"
#include "interfaces/TsdfRaycaster.hpp"
#include "types/ReconstructionPipelineConfig.hpp"

class ReconstructionPipeline final : public IReconstructionPipeline {
  public:
    ReconstructionPipeline(
        ITsdfRaycaster& tsdfRaycaster,
        IGaussianResidualModel& gaussianResidualModel,
        const ReconstructionPipelineConfig& config = ReconstructionPipelineConfig{}
    );
    ~ReconstructionPipeline() override;

    [[nodiscard]] ReconstructionObservation prepareObservation(
        const ReconstructionObservation& observation
    ) const override;

    [[nodiscard]] OptimizationObservationSet prepareObservationSet(
        const OptimizationObservationSet& observationSet
    ) const override;

    void seedCurrentObservation(const ReconstructionObservation& observation) override;

    [[nodiscard]] float optimizeObservationSet(
        const OptimizationObservationSet& observationSet
    ) override;

    [[nodiscard]] float runOptimizationCycle(
        const OptimizationObservationSet& observationSet
    ) override;

  private:
    ITsdfRaycaster& m_tsdfRaycaster;
    IGaussianResidualModel& m_gaussianResidualModel;
    ReconstructionPipelineConfig m_config{};
};
