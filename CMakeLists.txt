# ----- FLAGS -----
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable sanitizers for all non-Release builds
set(SANITIZE_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")

if (CMAKE_CONFIGURATION_TYPES)
    # Multi-config generator (VS, Xcode)
    message(STATUS "Enabling sanitizers for non-Release configs")
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        if (NOT config STREQUAL "Release")
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
    # Single-config generator (Make, Ninja)
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
        message(STATUS "Enabling sanitizers (build type: ${CMAKE_BUILD_TYPE})")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZE_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZE_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZE_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZE_FLAGS}")
    endif()
endif()

# ----- PROJECT -----
project(EdgeVision LANGUAGES CXX)

add_subdirectory(src/capture)

add_executable(EdgeVision src/main.cpp)
target_link_libraries(EdgeVision PRIVATE capture)
