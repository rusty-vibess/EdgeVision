#include "reconstruction/TorchGaussianResidualModel.hpp"

#include <torch/torch.h>

struct TorchGaussianResidualModel::Impl {
    TorchGaussianBackendConfig config{};
    bool torchAvailable = true;
    bool cudaAvailable = torch::cuda::is_available();

    explicit Impl(const TorchGaussianBackendConfig& backendConfig) : config(backendConfig) {}
};

TorchGaussianResidualModel::TorchGaussianResidualModel()
    : TorchGaussianResidualModel(TorchGaussianBackendConfig{}) {}

TorchGaussianResidualModel::TorchGaussianResidualModel(const TorchGaussianBackendConfig& config)
    : m_impl(std::make_unique<Impl>(config)) {}

TorchGaussianResidualModel::~TorchGaussianResidualModel() = default;

void TorchGaussianResidualModel::seedFromResidual(
    const ReconstructionObservation&,
    const ResidualSeedConfig&
) {}

HybridRenderOutput TorchGaussianResidualModel::renderHybrid(const HybridRenderInput& input) {
    HybridRenderOutput renderOutput{};
    renderOutput.size = input.observation.frame.rgb.size;
    return renderOutput;
}

float TorchGaussianResidualModel::optimizeObservationSet(const OptimizationObservationSet&) {
    return m_impl->config.placeholderLoss;
}

bool TorchGaussianResidualModel::isBackendAvailable() const {
    if (m_impl == nullptr) {
        return false;
    }

    return m_impl->torchAvailable && (!m_impl->config.requireCuda || m_impl->cudaAvailable);
}
