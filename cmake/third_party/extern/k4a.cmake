include(FetchContent)

# NOTE: Currently feature selection is not utilised, but if we want to shrink binaries this can 
# look to be implemented

# ---- Feature Selection k4a ----
# Override with -DTHIRDPARTY_BUILD_K4A_TOOLS=ON/OFF
# option(THIRDPARTY_BUILD_K4A_TOOLS "Build k4a viewer/tools/examples/tests" OFF)
# Default ON for Debug/RelWithDebInfo, OFF for Release/MinSizeRel
# if(NOT DEFINED THIRDPARTY_BUILD_K4A_TOOLS)
#     if(CMAKE_BUILD_TYPE MATCHES "^(Debug|RelWithDebInfo)$")
#         set(THIRDPARTY_BUILD_K4A_TOOLS ON)
#     else()
#         set(THIRDPARTY_BUILD_K4A_TOOLS OFF)
#     endif()
#     message(STATUS "THIRDPARTY_BUILD_K4A_TOOLS: (${THIRDPARTY_BUILD_K4A_TOOLS})")
# endif()

# if(NOT THIRDPARTY_BUILD_K4A_TOOLS)
#     set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
#     set(K4A_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#     set(K4A_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
#     set(K4A_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
#     set(K4A_BUILD_VIEWER OFF CACHE BOOL "" FORCE)
#     set(K4A_ENABLE_TOOLS OFF CACHE BOOL "" FORCE)
#     set(K4A_ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)

#     set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
#     set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
#     set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
# else()
#     set(BUILD_TESTING ON CACHE BOOL "" FORCE)
#     set(K4A_BUILD_TESTS ON CACHE BOOL "" FORCE)
#     set(K4A_BUILD_EXAMPLES ON CACHE BOOL "" FORCE)
#     set(K4A_BUILD_TOOLS ON CACHE BOOL "" FORCE)
#     set(K4A_BUILD_VIEWER ON CACHE BOOL "" FORCE)
#     set(K4A_ENABLE_TOOLS ON CACHE BOOL "" FORCE)
#     set(K4A_ENABLE_EXAMPLES ON CACHE BOOL "" FORCE)

#     set(GLFW_BUILD_EXAMPLES ON CACHE BOOL "" FORCE)
#     set(GLFW_BUILD_TESTS ON CACHE BOOL "" FORCE)
#     set(GLFW_BUILD_DOCS ON CACHE BOOL "" FORCE)
# endif()

set(LINUX ON CACHE BOOL "" FORCE)

# ---- Fetch k4a ----
message(CHECK_START "Fetching Azure Kinect SDK (k4a) v1.4.2...")
FetchContent_Declare(
    k4a
    GIT_REPOSITORY https://github.com/microsoft/Azure-Kinect-Sensor-SDK.git
    GIT_TAG v1.4.2
)

# Populate so patches can be applied to internal files
FetchContent_GetProperties(k4a)
if(NOT k4a_POPULATED)
    FetchContent_Populate(k4a)
endif()

add_subdirectory(${k4a_SOURCE_DIR} ${k4a_BINARY_DIR})

# ---- Build corrections ----
# Disable Werror breakage in one of k4a deps
if(TARGET aziotsharedutil)
    target_compile_options(aziotsharedutil PRIVATE
        -Wno-array-parameter
        -Wno-error=array-parameter
    )
endif()

# Prefer vendored libs and headers for k4a deps where possible
# Some require help to find headers
if(TARGET uvc_static)
    # libusb-dev (1.1.1)
    if(DEFINED k4a_SOURCE_DIR AND EXISTS "${k4a_SOURCE_DIR}/extern/libusb/src/libusb")
        target_include_directories(uvc_static PRIVATE
            "${k4a_SOURCE_DIR}/extern/libusb/src/libusb"
        )
    else()
        message(FATAL_ERROR "Failed to find k4a_SOURCE_DIR: (${k4a_SOURCE_DIR}) OR '${k4a_SOURCE_DIR}/extern/libusb/src/libusb' does NOT exist")
    endif()
endif()

if(TARGET k4aviewer)
    # soundio typically vendored by host, hence sysroot include
    target_include_directories(k4aviewer PRIVATE
        "${CMAKE_SYSROOT}/usr/include/soundio"
    )
    target_link_libraries(k4aviewer PRIVATE soundio)
endif()
