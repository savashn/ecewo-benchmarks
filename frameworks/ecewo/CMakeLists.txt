cmake_minimum_required(VERSION 3.10)
project(ecewo_hello_world VERSION 1.0.0 LANGUAGES C)

include(FetchContent)

FetchContent_Declare(
    ecewo
    GIT_REPOSITORY https://github.com/savashn/ecewo.git
    GIT_TAG v0.26.0
)

FetchContent_MakeAvailable(ecewo)

add_executable(server
    src/main.c
    vendors/cJSON.c
)

target_link_libraries(server PRIVATE ecewo)

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors
)
