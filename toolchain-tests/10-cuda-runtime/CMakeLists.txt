# include(${CMAKE_CURRENT_LIST_DIR}/../cmake/JetsonSysroot.cmake)

add_executable(t10_cuda_runtime main.cpp)

target_include_directories(t10_cuda_runtime PRIVATE
  "${JETSON_CUDA_ROOT}/include"
)

target_link_directories(t10_cuda_runtime PRIVATE
  "${JETSON_CUDA_ROOT}/lib64"
)

target_link_libraries(t10_cuda_runtime PRIVATE cudart)

# Make runtime library discovery easy on Jetson
set_target_properties(t10_cuda_runtime PROPERTIES
  BUILD_RPATH "/usr/local/cuda-12.6/lib64"
)
