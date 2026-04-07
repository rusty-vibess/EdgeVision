#pragma once

#include "types/TsdfTypes.hpp"

struct ResidualSeedConfig {
    float colorErrorThreshold = 0.0f;
    float newGaussianSampleRatio = 0.0f;
    float emptyAlphaThreshold = 0.0f;
    float depthMin = 0.0f;
    float depthMax = 0.0f;
};

struct HybridRenderInput {
    RgbdFrameView frame{};
    TsdfRaycastBuffers raycast{};
    float deltaDepth = 0.0f;
};

struct HybridRenderOutput {
    ImageSize size{};
    BufferView<float> rgb{};
    BufferView<float> depth{};
    BufferView<float> alpha{};
    BufferView<int> radii{};
    // Packed as N * 2 image-space means.
    BufferView<float> means2d{};
};
