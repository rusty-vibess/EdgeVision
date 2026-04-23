include(FetchContent)


FetchContent_Declare(
    infiniTAM
    GIT_REPOSITORY https://github.com/rusty-vibess/InfiniTAM.git
    GIT_TAG cmake-install
    SOURCE_SUBDIR  InfiniTAM
)

set(WITH_CUDA ON CACHE BOOL "Build InfiniTAM with CUDA" FORCE)
set(CUDA_COMPUTE_CAPABILITY 87 CACHE STRING "CUDA compute capability for Jetson Orin" FORCE)



FetchContent_MakeAvailable(infiniTAM)
