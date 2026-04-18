add_library(model)
target_sources(model PRIVATE
    src/frame/FrameStore.cpp
    src/frame/queue/FramePacketQueue.cpp
    src/frame/state/FrameHistory.cpp
    src/frame/state/FrameLifecycleTracker.cpp
    src/frame/utils/FrameValidator.cpp
    src/frame/utils/Submission.cpp
)
target_include_directories(model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(model PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/frame/include
)
target_include_directories(model SYSTEM PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/moodycamel/readerwriterqueue
)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
