add_executable(model_tests
    model/frame/FrameStore.cpp
)

target_link_libraries(model_tests PRIVATE model)

add_test(NAME model_unit_tests COMMAND model_tests)
register_bundle_test(model_tests)
