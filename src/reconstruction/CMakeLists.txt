# ---- Subdir Build ----
add_library(reconstruction)
target_sources(reconstruction PRIVATE
    src/interfaces/GaussianResidualModel.cpp
    src/interfaces/TsdfRaycaster.cpp
)
target_include_directories(reconstruction PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_features(reconstruction PUBLIC cxx_std_17)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
