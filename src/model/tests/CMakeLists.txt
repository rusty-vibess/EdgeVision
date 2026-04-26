add_executable(model_tests
    model/frame/FrameStore.cpp
)

target_link_libraries(model_tests PRIVATE model)

add_test(NAME model_unit_tests COMMAND model_tests)
register_bundle_test(model_tests)

add_executable(model_scene_tests
    model/scene/SharedScene.cpp
)

target_link_libraries(model_scene_tests PRIVATE model)

add_test(NAME model_scene_unit_tests COMMAND model_scene_tests)

register_bundle_test(model_scene_tests)

add_executable(model_scene_version_tests
    model/scene/SceneVersionStore.cpp
)

target_link_libraries(model_scene_version_tests PRIVATE model)

add_test(NAME model_scene_version_unit_tests COMMAND model_scene_version_tests)

register_bundle_test(model_scene_version_tests)

add_executable(model_viewer_pose_tests
    model/viewer/ViewerPoseStore.cpp
)

target_link_libraries(model_viewer_pose_tests PRIVATE model)

add_test(NAME model_viewer_pose_unit_tests COMMAND model_viewer_pose_tests)

register_bundle_test(model_viewer_pose_tests)

add_executable(model_viewer_output_tests
    model/viewer/RenderOutputStore.cpp
)

target_link_libraries(model_viewer_output_tests PRIVATE model)

add_test(NAME model_viewer_output_unit_tests COMMAND model_viewer_output_tests)

register_bundle_test(model_viewer_output_tests)
