#pragma once

#include "types/HybridTypes.hpp"

class IGaussianResidualModel {
  public:
    virtual ~IGaussianResidualModel();

    virtual void seedFromResidual(
        const RgbdFrameView& frame,
        const TsdfRaycastBuffers& raycast,
        const ResidualSeedConfig& config
    ) = 0;

    [[nodiscard]] virtual HybridRenderOutput renderHybrid(const HybridRenderInput& input) = 0;

    [[nodiscard]] virtual float optimizeLocalWindow(
        BufferView<RgbdFrameView> frames,
        BufferView<TsdfRaycastBuffers> raycasts
    ) = 0;
};
