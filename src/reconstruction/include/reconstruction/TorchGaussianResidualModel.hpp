#pragma once

#include <memory>

#include "interfaces/GaussianResidualModel.hpp"
#include "types/TorchGaussianBackendConfig.hpp"

class TorchGaussianResidualModel final : public IGaussianResidualModel {
  public:
    TorchGaussianResidualModel();
    explicit TorchGaussianResidualModel(const TorchGaussianBackendConfig& config);
    ~TorchGaussianResidualModel() override;

    void seedFromResidual(
        const ReconstructionObservation& observation,
        const ResidualSeedConfig& config
    ) override;

    [[nodiscard]] HybridRenderOutput renderHybrid(const HybridRenderInput& input) override;

    [[nodiscard]] float optimizeObservationSet(
        const OptimizationObservationSet& observationSet
    ) override;

    [[nodiscard]] bool isBackendAvailable() const;

  private:
    struct Impl;

    std::unique_ptr<Impl> m_impl;
};
