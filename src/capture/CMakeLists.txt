add_library(capture)
target_sources(capture PRIVATE
  src/CameraCapture.cpp
  src/FrameBuilder.cpp
)
target_include_directories(capture PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
