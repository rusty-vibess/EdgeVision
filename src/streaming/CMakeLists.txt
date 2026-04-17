# ---- Render server (TCP streaming) ----
# Single-client TCP server adapted from GPS-SLAM's remote_viewer.cpp.
# Dependencies:
#   - POSIX sockets (standard Linux — works on Jetson aarch64, no library)
#   - nlohmann/json (header-only via FetchContent — no cross-compile concern)
#   - libjpeg (from JetPack sysroot — libjpeg-turbo on Jetson)
#
# If libjpeg-dev is missing from the rsync'd sysroot:
#   apt-get install libjpeg-turbo8-dev   (on the Jetson, then re-rsync)

include(FetchContent)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

find_package(JPEG REQUIRED)

add_library(streaming STATIC RenderServer.cpp)
target_include_directories(streaming PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(streaming
    PRIVATE "${TORCH_LIBRARIES}"
    PRIVATE nlohmann_json::nlohmann_json
    PRIVATE JPEG::JPEG
)
