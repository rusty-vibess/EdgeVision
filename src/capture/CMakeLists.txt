# ---- Deps ----
find_package(k4a CONFIG REQUIRED)

# ---- Subdir Build ----
add_library(capture)
target_sources(capture PRIVATE
    src/Camera.cpp
    src/Frame.cpp
    src/interfaces/k4a.cpp
    src/interfaces/CameraInterface.cpp
)
target_include_directories(capture PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(capture PUBLIC k4a::k4a logs)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
