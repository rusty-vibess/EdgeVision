add_executable(app_viewer_pose_seeder_tests
    runtime/ViewerPoseSeeder.cpp
    ../src/runtime/ViewerPoseSeeder.cpp
)

target_include_directories(app_viewer_pose_seeder_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/include
)

target_link_libraries(app_viewer_pose_seeder_tests PRIVATE
    config
    model
)

add_test(NAME app_viewer_pose_seeder_unit_tests COMMAND app_viewer_pose_seeder_tests)
register_bundle_test(app_viewer_pose_seeder_tests)

add_executable(app_viewer_frame_dumper_tests
    runtime/ViewerFrameDumper.cpp
    ../src/runtime/ViewerFrameDumper.cpp
)

target_include_directories(app_viewer_frame_dumper_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/include
)

target_link_libraries(app_viewer_frame_dumper_tests PRIVATE
    config
    model
)

add_test(NAME app_viewer_frame_dumper_unit_tests COMMAND app_viewer_frame_dumper_tests)
register_bundle_test(app_viewer_frame_dumper_tests)
