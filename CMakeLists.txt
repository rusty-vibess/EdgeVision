# ----- FLAGS -----
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
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

add_executable(EdgeVision src/main.cpp)
# third_party deps install
list(APPEND CMAKE_PREFIX_PATH "/opt/jetson-sysroot/edgevision/install")

add_subdirectory(src/capture)

target_link_libraries(EdgeVision PRIVATE capture)
