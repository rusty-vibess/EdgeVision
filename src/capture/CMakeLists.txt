# ---- Deps ----
find_package(k4a CONFIG REQUIRED)

# ---- Subdir Build ----
add_library(capture)
target_sources(capture PRIVATE
    src/camera/CameraCapture.cpp
    src/frame/FrameIngestor.cpp
    src/frame/K4aFrameBuilder.cpp
    src/frame/resources/CaptureHandle.cpp
    src/frame/resources/ImageHandle.cpp
    src/frame/resources/TransformationHandle.cpp
    src/frame/utils/DepthAlignmentResult.cpp
    src/frame/utils/FrameBuildResult.cpp
    src/frame/utils/FrameIngestResult.cpp
    src/frame/utils/K4aDepthAlignmentValidator.cpp
    src/frame/utils/K4aDepthAligner.cpp
    src/frame/utils/K4aFrameAssembler.cpp
    src/frame/utils/K4aFrameValidator.cpp
    src/frame/utils/MonotonicTimestamp.cpp
    src/interfaces/K4aInterface.cpp
)
target_include_directories(capture PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(capture PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frame/include
)
target_link_libraries(capture PUBLIC k4a::k4a model)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
