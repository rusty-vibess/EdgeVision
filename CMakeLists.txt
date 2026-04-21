# ----- FLAGS -----
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Build Config ----
if(CMAKE_CONFIGURATION_TYPES)
    set(IS_MULTI_CONFIG ON)
    message(STATUS "Multi-config generator")
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        message(STATUS "Build type(s): (${config} config)")
    endforeach()
else()
    set(IS_MULTI_CONFIG OFF)
    message(STATUS "Single-config generator")
    message(STATUS "Build type: (${CMAKE_BUILD_TYPE})")
endif()

if(NOT IS_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    message(STATUS "Defaulting CMAKE_BUILD_TYPE to Debug")
endif()

# ---- Sanitizers ----
set(SANITIZE_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")

if(IS_MULTI_CONFIG)
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        if(config STREQUAL "Debug")
            message(STATUS "Enabling sanitizers for ${config} config")
            string(TOUPPER ${config} CONFIG_UPPER)
            set(CMAKE_C_FLAGS_${CONFIG_UPPER}
                "${CMAKE_C_FLAGS_${CONFIG_UPPER}} ${SANITIZE_FLAGS}")
            set(CMAKE_CXX_FLAGS_${CONFIG_UPPER}
                "${CMAKE_CXX_FLAGS_${CONFIG_UPPER}} ${SANITIZE_FLAGS}")
            set(CMAKE_EXE_LINKER_FLAGS_${CONFIG_UPPER}
                "${CMAKE_EXE_LINKER_FLAGS_${CONFIG_UPPER}} ${SANITIZE_FLAGS}")
            set(CMAKE_SHARED_LINKER_FLAGS_${CONFIG_UPPER}
                "${CMAKE_SHARED_LINKER_FLAGS_${CONFIG_UPPER}} ${SANITIZE_FLAGS}")
        endif()
    endforeach()
else()
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling sanitizers (build type: ${CMAKE_BUILD_TYPE})")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZE_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZE_FLAGS}")
    endif()
endif()

# ----- PROJECT -----
project(EdgeVision LANGUAGES CXX)

# ---- Custom Functions ----
# Provides fns required for bundling tests
include(cmake/functions/test_registry.cmake)

# ---- Tests ----
option(ENABLE_TESTS "Build unit tests" ON)
if(ENABLE_TESTS)
    enable_testing()
endif()

add_executable(EdgeVision src/main.cpp)
# third_party deps install
list(APPEND CMAKE_PREFIX_PATH "/opt/jetson-sysroot-overlay/install")
# Torch being lower in the superbuild caused strange issues hence find_package here
find_package(Torch CONFIG REQUIRED)

add_subdirectory(src/config)
add_subdirectory(src/model)
add_subdirectory(src/capture)
add_subdirectory(src/reconstruction)
add_subdirectory(src/streaming)

target_link_libraries(EdgeVision PRIVATE config)
target_link_libraries(EdgeVision PRIVATE capture)
target_link_libraries(EdgeVision PRIVATE model)
target_link_libraries(EdgeVision PRIVATE reconstruction)
target_link_libraries(EdgeVision PRIVATE streaming)

# ---- Bundle ----
# Creates a self-contained bundle without affecting normal build/install
set(EDGEVISION_OVERLAY_ROOT "/opt/jetson-sysroot-overlay" CACHE PATH "Overlay root to bundle from")
set(EDGEVISION_BUNDLE_DIR "${CMAKE_BINARY_DIR}/bundle" CACHE PATH "Bundle output directory")

collect_test_registry()
set(_edgevision_bundle_extra_binaries "")
if(ENABLE_TESTS)
    foreach(_test_target IN LISTS EDGEVISION_TEST_TARGETS)
        if(_edgevision_bundle_extra_binaries)
            string(APPEND _edgevision_bundle_extra_binaries "$<SEMICOLON>")
        endif()
        string(APPEND _edgevision_bundle_extra_binaries "$<TARGET_FILE:${_test_target}>")
    endforeach()
endif()

add_custom_target(bundle
    COMMAND
    "${CMAKE_COMMAND}"
    -D EDGEVISION_OVERLAY_ROOT="${EDGEVISION_OVERLAY_ROOT}"
    -D EDGEVISION_BUNDLE_DIR="${EDGEVISION_BUNDLE_DIR}"
    -D EDGEVISION_BINARY="$<TARGET_FILE:EdgeVision>"
    -D EDGEVISION_EXTRA_BINARIES="${_edgevision_bundle_extra_binaries}"
    -P "${CMAKE_CURRENT_LIST_DIR}/cmake/bundle.cmake"
    DEPENDS EdgeVision
)

if(ENABLE_TESTS AND EDGEVISION_TEST_TARGETS)
    add_dependencies(bundle ${EDGEVISION_TEST_TARGETS})
endif()
