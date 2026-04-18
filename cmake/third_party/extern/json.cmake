include(FetchContent)

set(JSON_BuildTests OFF CACHE BOOL "" FORCE)
set(JSON_Install ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(json)
