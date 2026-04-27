find_package(infinitam CONFIG REQUIRED)

add_library(viewer)
target_sources(viewer PRIVATE
    src/SceneViewer.cpp
    src/SceneViewerRunner.cpp
    src/state/InfiniTamViewerPipeline.cpp
)

target_include_directories(viewer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_include_directories(viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_link_libraries(viewer PUBLIC config model)
target_link_libraries(viewer PRIVATE InfiniTAM::ITMLib)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
