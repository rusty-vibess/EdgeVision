cmake_minimum_required(VERSION 3.22)
project(third_party LANGUAGES C CXX)

# Install prefix (can be overridden by -DCMAKE_INSTALL_PREFIX=...)
set(CMAKE_INSTALL_PREFIX
    "/opt/jetson-sysroot-overlay/install"
    CACHE PATH "Third-party install prefix" FORCE)

# Mirror close source third-party libs into the active sysroot overlay install if present
set(EDGEVISION_THIRD_PARTY_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../../third_party/libs_ubuntu20.04_aarch64")
set(EDGEVISION_SYSROOT_OVERLAY_LIB_DIR "${CMAKE_INSTALL_PREFIX}/lib")

if(EXISTS "${EDGEVISION_THIRD_PARTY_LIB_DIR}")
    file(MAKE_DIRECTORY "${EDGEVISION_SYSROOT_OVERLAY_LIB_DIR}")
    file(COPY "${EDGEVISION_THIRD_PARTY_LIB_DIR}/"
        DESTINATION "${EDGEVISION_SYSROOT_OVERLAY_LIB_DIR}")
    message(STATUS "Mirrored ${EDGEVISION_THIRD_PARTY_LIB_DIR} -> ${EDGEVISION_SYSROOT_OVERLAY_LIB_DIR}")
else()
    message(STATUS "Third-party lib directory not found at ${EDGEVISION_THIRD_PARTY_LIB_DIR}; skipping mirror")
endif()

# Include cmake for deps to be pulled in
include(extern/k4a.cmake)
include(extern/json.cmake)
include(extern/torch.cmake)
include(extern/readerwriterqueue.cmake)
include(extern/infinitam.cmake)


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
