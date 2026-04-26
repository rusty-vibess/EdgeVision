add_executable(viewer_tests
    viewer/SceneViewer.cpp
)

target_include_directories(viewer_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../builder/src/include
)

target_link_libraries(viewer_tests PRIVATE
    builder
    viewer
)

add_test(NAME viewer_unit_tests COMMAND viewer_tests)
register_bundle_test(viewer_tests)

add_executable(viewer_runner_tests
    viewer/ViewerRunner.cpp
)

target_include_directories(viewer_runner_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../builder/src/include
)

target_link_libraries(viewer_runner_tests PRIVATE
    builder
    viewer
)

add_test(NAME viewer_runner_unit_tests COMMAND viewer_runner_tests)
register_bundle_test(viewer_runner_tests)
