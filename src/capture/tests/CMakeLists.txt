add_executable(capture_camera_tests
    capture/camera/CameraCapture.cpp
)

target_link_libraries(capture_camera_tests PRIVATE capture)

add_test(NAME capture_camera_unit_tests COMMAND capture_camera_tests)
register_bundle_test(capture_camera_tests)

add_executable(capture_frame_builder_tests
    capture/frame/K4aFrameBuilder.cpp
)

target_link_libraries(capture_frame_builder_tests PRIVATE capture)

add_test(NAME capture_frame_builder_unit_tests COMMAND capture_frame_builder_tests)
register_bundle_test(capture_frame_builder_tests)

add_executable(capture_frame_ingestor_tests
    capture/frame/FrameIngestor.cpp
)

target_link_libraries(capture_frame_ingestor_tests PRIVATE capture)

add_test(NAME capture_frame_ingestor_unit_tests COMMAND capture_frame_ingestor_tests)
register_bundle_test(capture_frame_ingestor_tests)
