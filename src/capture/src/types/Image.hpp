#pragma once

#include <k4a/k4a.h>

enum class PixelFormat : uint32_t {
    Unknown = 0,
    BGRA32
};

struct ImageMeta {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t stride_bytes = 0;
    PixelFormat format{};
    uint64_t frame_id = 0;
    uint64_t timestamp_usec = 0;
};

struct BufferRef {
    uint8_t* ptr = nullptr;
    size_t cap = 0;
    uint32_t id = 0;
};

struct ColorImage {
    ImageMeta meta{};
    BufferRef buf{};
    size_t bytes = 0;
};
