# ---- Subdir Build ----
add_library(reconstruction)
target_sources(reconstruction PRIVATE
    src/gaussian/TorchGaussianResidualModel.cpp
    src/interfaces/GaussianResidualModel.cpp
    src/interfaces/ReconstructionPipeline.cpp
    src/interfaces/TsdfRaycaster.cpp
    src/reconstruction/ReconstructionPipeline.cpp
)
target_include_directories(reconstruction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(reconstruction PRIVATE "${TORCH_LIBRARIES}")

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
