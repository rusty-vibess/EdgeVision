add_library(config)
target_sources(config PRIVATE
    src/CommandLineParser.cpp
)
target_include_directories(config PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif()
