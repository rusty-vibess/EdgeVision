find_package(readerwriterqueue CONFIG REQUIRED)
find_package(infinitam CONFIG REQUIRED)

add_library(model)
target_sources(model PRIVATE
    src/frame/FrameStore.cpp
    src/frame/queue/ReadyFrameQueue.cpp
    src/frame/state/FrameHistory.cpp
    src/frame/state/FrameLifecycleTracker.cpp
    src/frame/utils/FrameValidator.cpp
    src/frame/utils/Submission.cpp
    src/viewer/RenderOutputStore.cpp
    src/viewer/ViewerPoseStore.cpp
    src/scene/SceneAccess.cpp
    src/scene/state/SceneAccessLease.cpp
    src/scene/state/SceneVersionHistory.cpp
    src/scene/state/SharedSceneState.cpp
    src/scene/SceneVersionStore.cpp
    src/scene/SharedScene.cpp
)

target_include_directories(model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(model PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frame/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/include
)
target_include_directories(model SYSTEM PUBLIC
    ${CMAKE_SYSROOT}/usr/local/cuda-12.6/include
)

target_link_libraries(model PUBLIC
    readerwriterqueue::readerwriterqueue
    InfiniTAM::ITMLib
)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
