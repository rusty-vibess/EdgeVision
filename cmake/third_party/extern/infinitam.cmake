include(FetchContent)


FetchContent_Declare(
    infiniTAM
    GIT_REPOSITORY https://github.com/victorprad/InfiniTAM.git
    GIT_TAG master
    SOURCE_SUBDIR  InfiniTAM
)
FetchContent_GetProperties(infiniTAM)
message(STATUS "infiniTAM source dir = ${infiniTAM_SOURCE_DIR}")

FetchContent_MakeAvailable(infiniTAM)
