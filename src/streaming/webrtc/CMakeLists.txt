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
# - websocketpp (header-only, FetchContent)
# - asio standalone (header-only, FetchContent)
# - nlohmann/json (header-only — pulled by parent src/streaming/CMakeLists.txt
#   already; declared here defensively)

include(FetchContent)

# ---- websocketpp + asio (header-only) ----------------------------------
FetchContent_Declare(
    asio_standalone
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(asio_standalone)

add_library(asio_headers INTERFACE)
target_include_directories(asio_headers INTERFACE
    ${asio_standalone_SOURCE_DIR}/asio/include
)
target_compile_definitions(asio_headers INTERFACE
    ASIO_STANDALONE
    ASIO_HAS_STD_INVOKE_RESULT
)

FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
    GIT_TAG        0.8.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(websocketpp)

add_library(websocketpp_headers INTERFACE)
target_include_directories(websocketpp_headers INTERFACE
    ${websocketpp_SOURCE_DIR}
)
target_link_libraries(websocketpp_headers INTERFACE asio_headers)
target_compile_definitions(websocketpp_headers INTERFACE
    _WEBSOCKETPP_CPP11_STL_
)

# ---- gstreamer (system, from JetPack sysroot) --------------------------
find_package(PkgConfig REQUIRED)
pkg_check_modules(GST REQUIRED IMPORTED_TARGET
    gstreamer-1.0
    gstreamer-app-1.0
    gstreamer-sdp-1.0
    gstreamer-webrtc-1.0
)

# ---- nlohmann/json (defensive, parent already declared it) -------------
if(NOT TARGET nlohmann_json::nlohmann_json)
    FetchContent_Declare(
        json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(json)
endif()

# ---- the library ------------------------------------------------------
add_library(streaming_webrtc STATIC
    src/WebRtcServer.cpp
    src/PipelineBuilder.cpp
    src/SignallingHandler.cpp
    src/FrameSource.cpp
)
target_include_directories(streaming_webrtc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(streaming_webrtc
    PUBLIC  PkgConfig::GST
            nlohmann_json::nlohmann_json
            websocketpp_headers
            asio_headers
    PRIVATE capture
            model
            reconstruction
)
target_compile_features(streaming_webrtc PUBLIC cxx_std_20)

# ---- tests ------------------------------------------------------------
if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
