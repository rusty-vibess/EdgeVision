add_executable(builder_tests
    builder/SceneBuilder.cpp
)

target_include_directories(builder_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/include
)

target_link_libraries(builder_tests PRIVATE
    builder
)

add_test(NAME builder_unit_tests COMMAND builder_tests)
register_bundle_test(builder_tests)

add_executable(builder_runner_tests
    builder/SceneBuilderRunner.cpp
)

target_include_directories(builder_runner_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/include
)

target_link_libraries(builder_runner_tests PRIVATE
    builder
)

add_test(NAME builder_runner_unit_tests COMMAND builder_runner_tests)
register_bundle_test(builder_runner_tests)
