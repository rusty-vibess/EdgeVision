cmake_minimum_required(VERSION 3.22)
project(third_party LANGUAGES C CXX)

# Install prefix (can be overridden by -DCMAKE_INSTALL_PREFIX=...)
set(CMAKE_INSTALL_PREFIX
    "/opt/jetson-sysroot-overlay/install"
    CACHE PATH "Third-party install prefix" FORCE)

include(extern/k4a.cmake)


# ---- Apply third_party patches ----
# Any known file issues in third_party deps, that need patching before config + build
execute_process(
    COMMAND "${CMAKE_CURRENT_LIST_DIR}/../../scripts/apply-patches.sh"
    "${CMAKE_BINARY_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/../../patches/third_party"
    RESULT_VARIABLE r
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
)

if(NOT r EQUAL 0)
    message(STATUS "apply-patches stdout:\n${out}")
    message(STATUS "apply-patches stderr:\n${err}")
    message(FATAL_ERROR "Patching failed (exit ${r})")
endif()
