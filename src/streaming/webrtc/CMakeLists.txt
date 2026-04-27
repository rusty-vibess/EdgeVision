# ---- WebRTC streaming module ----
# H.264 over webrtcbin + JSON signalling on a tiny WebSocket listener.
# Designed for Jetson Orin Nano: 854x480@30, 1.5 Mbps, software x264enc
# (Orin Nano has no NVENC). Default mode is TSDF-only; RGB lives on
# the existing TCP RenderServer to keep within the single-stream budget.
#
# Sysroot must include gstreamer1.0-plugins-ugly (provides x264enc) on
# top of the standard gst-plugins-base/good/bad set already present on
# JetPack r36.4.4.
#
# Deps come from JetPack r36.4.4 sysroot (gstreamer-1.0 + plugins) plus:
# - websocketpp + standalone asio headers (from third-party overlay install)
# - nlohmann/json (from third-party overlay install)

find_package(nlohmann_json CONFIG REQUIRED)

# ---- gstreamer (system, from JetPack sysroot) --------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-sdp-1.0
    gstreamer-webrtc-1.0
)

# ---- the library ------------------------------------------------------
add_library(streaming_webrtc STATIC
    src/WebRtcServer.cpp
    src/PipelineBuilder.cpp
    src/SignallingHandler.cpp
    src/FrameSource.cpp
)
set_target_properties(streaming_webrtc PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
target_include_directories(streaming_webrtc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(streaming_webrtc PRIVATE
    ${SYSROOT_OVERLAY}/install/include
)
target_compile_definitions(streaming_webrtc PRIVATE
    ASIO_STANDALONE
    ASIO_HAS_STD_INVOKE_RESULT
    _WEBSOCKETPP_CPP11_STL_
)
target_link_libraries(streaming_webrtc
    PUBLIC  
        PkgConfig::GSTREAMER
        nlohmann_json::nlohmann_json
    PRIVATE 
        capture
        model
        reconstruction
)

# ---- tests ------------------------------------------------------------
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
