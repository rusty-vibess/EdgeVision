set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Sysroot and required dependency overlay
set(CMAKE_SYSROOT "/opt/jetson-sysroot" CACHE PATH "" FORCE)
set(SYSROOT_OVERLAY "/opt/jetson-sysroot-overlay" CACHE PATH "" FORCE)

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
set(CMAKE_CUDA_HOST_COMPILER aarch64-linux-gnu-g++)

set(CMAKE_AR aarch64-linux-gnu-ar)
set(CMAKE_RANLIB aarch64-linux-gnu-ranlib)
set(TORCH_CUDA_ARCH_LIST "8.7" CACHE STRING "")

# Rooted search for target headers/libs/packages
set(CMAKE_FIND_ROOT_PATH "${CMAKE_SYSROOT};${SYSROOT_OVERLAY}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${CMAKE_SYSROOT}")
set(ENV{PKG_CONFIG_LIBDIR} "${CMAKE_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:${CMAKE_SYSROOT}/usr/lib/pkgconfig:${CMAKE_SYSROOT}/usr/share/pkgconfig:${SYSROOT_OVERLAY}/install/lib/pkgconfig:${SYSROOT_OVERLAY}/install/share/pkgconfig")

set(Torch_DIR "/opt/jetson-sysroot-overlay/install/share/cmake/Torch" CACHE PATH "" FORCE)

set(CUDA_NVCC_EXECUTABLE "/opt/jetson-sysroot/usr/local/cuda-12.6/bin/nvcc" CACHE PATH "" FORCE)
set(CMAKE_CUDA_COMPILER "/opt/jetson-sysroot/usr/local/cuda-12.6/bin/nvcc" CACHE PATH "" FORCE)

# CUDA location within target fs
set(CUDAToolkit_ROOT "/opt/jetson-sysroot/usr/local/cuda-12.6")
set(CUDA_TOOLKIT_ROOT_DIR "/opt/jetson-sysroot/usr/local/cuda-12.6")  # for older logic
set(CUDA_HOME "/opt/jetson-sysroot/usr/local/cuda-12.6")
set(CUDA_CUDART_LIBRARY "/opt/jetson-sysroot/usr/local/cuda-12.6/targets/aarch64-linux/lib/libcudart.so" CACHE FILEPATH "" FORCE)

list(PREPEND CMAKE_LIBRARY_PATH "/usr/local/cuda-12.6/targets/aarch64-linux/lib")
list(PREPEND CMAKE_LIBRARY_PATH "/usr/local/cuda-12.6/targets/aarch64-linux/lib/stubs")
list(PREPEND CMAKE_LIBRARY_PATH "/usr/local/cuda-12.6/lib64")
list(PREPEND CMAKE_INCLUDE_PATH "/usr/local/cuda-12.6/include")
