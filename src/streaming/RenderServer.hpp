#pragma once

#include <cstdint>
#include <functional>
#include <vector>

/// Single-client TCP render server. Accepts one client at a time (the
/// edgevision-mobile Flutter app, via its Python WebSocket-TCP adapter),
/// reads JSON pose requests, calls a user-supplied render callback, and
/// streams JPEG-compressed frames back.
///
/// Adapted from GPS-SLAM's `remote_viewer.cpp` — same wire protocol so
/// the existing Flutter client + adapter connect with zero changes.
///
/// Wire format (per frame, "minimal" mode — the only mode the Flutter
/// edgevision-mobile client ever uses):
///
///   Client -> Server:  [u32 json_len][json_len bytes of JSON]
///     JSON: { fov_x, fov_y, resolution_x, resolution_y, pose: [16 floats], minimal: bool }
///
///   Server -> Client:  [u32 w][u32 h][u32 jpeg_size][jpeg_size bytes]
///                      [36 bytes rotation][12 bytes translation]
///                      [u32 info_len][info_len bytes][64 bytes mvp]
///
/// Pose convention: the 16 floats are a row-major 4x4 camera-to-world
/// matrix. The server transposes and negates Y/Z columns to convert from
/// OpenGL wire convention to OpenCV/Colmap rendering convention.
/// See GPS-SLAM remote_viewer.cpp:31-33.

struct RenderPoseRequest {
    float fx = 0.0f;
    float fy = 0.0f;
    /// Derived from resolution / 2 (not on the wire).
    float cx = 0.0f;
    float cy = 0.0f;
    int width = 0;
    int height = 0;
    /// Row-major 4x4 c2w after Y/Z negation (OpenCV/Colmap convention).
    float pose[16]{};
    bool minimal = false;
};

/// Callback the server invokes per frame. Receives the parsed pose
/// request; `rgbOut` is pre-sized to width*height*3. Fill with uint8
/// sRGB pixels (row-major, top-left origin). Return true on success,
/// false to send a black frame.
using RenderCallback = std::function<bool(
    const RenderPoseRequest& request,
    std::vector<uint8_t>& rgbOut
)>;

class RenderServer {
  public:
    RenderServer() = default;
    RenderServer(const RenderServer&) = delete;
    RenderServer& operator=(const RenderServer&) = delete;

    /// Start listening on `port`. Blocks forever (accept loop).
    /// Call from main() after pipeline init.
    void serve(int port, RenderCallback onRender);
};
