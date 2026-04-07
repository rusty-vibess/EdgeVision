#pragma once

#include <array>

#include "types/BufferView.hpp"

struct ImageSize {
    int width = 0;
    int height = 0;
};

struct CameraIntrinsics {
    float fx = 0.0f;
    float fy = 0.0f;
    float cx = 0.0f;
    float cy = 0.0f;
};

struct Pose4f {
    std::array<float, 16> matrix{};
};

struct RgbFrameView {
    ImageSize size{};
    BufferView<float> data{};
};

struct DepthFrameView {
    ImageSize size{};
    BufferView<float> data{};
};

struct RgbdFrameView {
    RgbFrameView rgb{};
    DepthFrameView depth{};
    CameraIntrinsics intrinsics{};
    Pose4f cameraToWorld{};
    Pose4f cameraToWorldSlam{};
    bool hasDepth = true;
};

struct TsdfRaycastBuffers {
    ImageSize size{};
    BufferView<float> colorMap{};
    // One world-space XYZ surface hit per pixel.
    BufferView<float> vertexMap{};
    BufferView<float> confidenceMap{};
    // Camera-space Z for the visible hit.
    BufferView<float> depthMap{};
    // Derived from vertexMap rather than camera input.
    BufferView<float> normalMap{};
};
