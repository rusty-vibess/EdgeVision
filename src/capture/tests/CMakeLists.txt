add_executable(capture_tests
    CameraCapture.cpp
)

target_link_libraries(capture_tests PRIVATE capture)

add_test(NAME capture_unit_tests COMMAND capture_tests)
register_bundle_test(capture_tests)
