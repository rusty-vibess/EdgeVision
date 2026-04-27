find_package(GTest CONFIG REQUIRED)

add_executable(streaming_webrtc_tests
    PipelineBuilderTests.cpp
    utils/PoseUpdateHandler.cpp
)
target_include_directories(streaming_webrtc_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/include
)
target_link_libraries(streaming_webrtc_tests
    PRIVATE model streaming_webrtc GTest::gtest_main
)
register_bundle_test(streaming_webrtc_tests)
add_test(NAME streaming_webrtc_tests COMMAND streaming_webrtc_tests)
