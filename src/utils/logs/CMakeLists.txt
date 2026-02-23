# ---- Deps ----
find_package(k4a CONFIG REQUIRED)

# ---- Subdir Build ----
add_library(logs)
target_sources(logs PRIVATE
    src/logger.cpp
)
target_include_directories(logs PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(logs PUBLIC k4a::k4a)
