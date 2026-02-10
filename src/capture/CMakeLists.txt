# ---- Deps ----
find_package(k4a CONFIG REQUIRED)

# ---- Subdir Build ----
add_library(capture)
target_sources(capture PRIVATE
  src/CameraCapture.cpp
  src/FrameBuilder.cpp
)
target_include_directories(capture PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(capture PUBLIC k4a::k4a)

if(ENABLE_TESTS)
  add_subdirectory(tests)
endif()
