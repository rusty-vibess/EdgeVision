add_executable(t10_cuda_runtime main.cpp)

find_package(CUDAToolkit REQUIRED)
target_link_libraries(t10_cuda_runtime PRIVATE CUDA::cudart)
