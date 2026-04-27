include(FetchContent)

FetchContent_Declare(
    asio_standalone
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-30-2
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(asio_standalone)
if(NOT asio_standalone_POPULATED)
    FetchContent_Populate(asio_standalone)
endif()

FetchContent_Declare(
    websocketpp
    GIT_REPOSITORY https://github.com/zaphoyd/websocketpp.git
    GIT_TAG        0.8.2
    GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(websocketpp)
if(NOT websocketpp_POPULATED)
    FetchContent_Populate(websocketpp)
endif()

install(DIRECTORY "${websocketpp_SOURCE_DIR}/websocketpp" DESTINATION include)
install(DIRECTORY "${asio_standalone_SOURCE_DIR}/asio/include/asio" DESTINATION include)
install(FILES "${asio_standalone_SOURCE_DIR}/asio/include/asio.hpp" DESTINATION include)
