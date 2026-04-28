# ---- Render server (TCP streaming) ----
# Single-client TCP server adapted from GPS-SLAM's remote_viewer.cpp.
# Dependencies:
#   - POSIX sockets (standard Linux — works on Jetson aarch64, no library)
#   - nlohmann/json (header-only via build-deps overlay install)
#   - libjpeg (from JetPack sysroot — libjpeg-turbo on Jetson)
#
# If libjpeg-dev is missing from the rsync'd sysroot:
#   apt-get install libjpeg-turbo8-dev   (on the Jetson, then re-rsync)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(JPEG REQUIRED)

add_library(streaming STATIC
    src/streaming/RenderServer.cpp
)
target_include_directories(streaming PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(streaming
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE JPEG::JPEG
)
