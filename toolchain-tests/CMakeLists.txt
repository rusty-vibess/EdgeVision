cmake_minimum_required(VERSION 3.16)
project(toolchain_tests LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(00-hello)
add_subdirectory(10-cuda-runtime)
# add_subdirectory(20-libtorch)
