#pragma once

struct TorchGaussianBackendConfig {
    bool requireCuda = true;
    float placeholderLoss = 0.0f;
};
