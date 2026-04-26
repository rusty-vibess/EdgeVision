include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(googletest)

add_executable(streaming_webrtc_tests
    PipelineBuilderTests.cpp
    FrameSourceTests.cpp
)
target_link_libraries(streaming_webrtc_tests
    PRIVATE streaming_webrtc gtest_main
)
register_bundle_test(streaming_webrtc_tests)
add_test(NAME streaming_webrtc_tests COMMAND streaming_webrtc_tests)
