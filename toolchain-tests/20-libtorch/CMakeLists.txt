cmake_minimum_required(VERSION 3.16)

project(t20_libtorch LANGUAGES CXX)

set(TORCH_ROOT "/site-packages/torch" CACHE PATH "Torch root (contains share/cmake/Torch)")
set(TORCH_RUNTIME_LIB_DIR "${TORCH_ROOT}/lib" CACHE PATH "Torch runtime lib dir on target")

list(PREPEND CMAKE_PREFIX_PATH "${TORCH_ROOT}")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_package(Torch REQUIRED)

# ---- Target ----
add_executable(t20_libtorch main.cpp)
target_link_libraries(t20_libtorch PRIVATE ${TORCH_LIBRARIES})
target_include_directories(t20_libtorch PRIVATE ${TORCH_INCLUDE_DIRS})

set_target_properties(t20_libtorch PROPERTIES
  BUILD_RPATH "${TORCH_RUNTIME_LIB_DIR};\$ORIGIN/../lib"
  INSTALL_RPATH "${TORCH_RUNTIME_LIB_DIR};\$ORIGIN/../lib"
)
