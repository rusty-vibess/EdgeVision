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

message(STATUS "LSan suppression file: ${CMAKE_SOURCE_DIR}/cmake/suppressions/lsan.supp")
set_tests_properties(model_scene_unit_tests PROPERTIES
    ENVIRONMENT "LSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/cmake/suppressions/lsan.supp:print_suppressions=1:verbosity=1"
)

register_bundle_test(model_scene_tests)
