find_package(infinitam CONFIG REQUIRED)

# ---- Subdir Build ----
add_library(builder)
target_sources(builder PRIVATE
    src/SceneBuilderRunner.cpp
    src/state/InfiniTamPipeline.cpp
    src/state/SceneBuilder.cpp
    src/utils/InfiniTamViewConverter.cpp
)

target_include_directories(builder PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(builder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)
target_include_directories(model SYSTEM PUBLIC
    ${CMAKE_SYSROOT}/usr/local/cuda-12.6/include
)

target_link_libraries(builder PUBLIC config model)
target_link_libraries(builder PRIVATE InfiniTAM::ITMLib)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
