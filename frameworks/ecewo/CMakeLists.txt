cmake_minimum_required(VERSION 3.14)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)  # DOWNLOAD_EXTRACT_TIMESTAMP
endif()
if(POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)  # Don't add /W3 by default
endif()

project(ecewo_hello_world VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Release build" FORCE)
endif()

include(FetchContent)

FetchContent_Declare(
    ecewo
    GIT_REPOSITORY https://github.com/savashn/ecewo.git
    GIT_TAG experimental
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(ecewo)

add_executable(server
    src/main.c
    vendors/cJSON.c
)

if(MSVC)
    target_compile_options(server PRIVATE 
        /W4
        /wd4996
    )
    target_compile_definitions(server PRIVATE 
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
    )
else()
    target_compile_options(server PRIVATE -Wall -Wextra -std=c99)
    target_compile_options(server PRIVATE -O3)
    target_compile_definitions(server PRIVATE NDEBUG=1)
endif()

target_link_libraries(server PRIVATE ecewo::ecewo)

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/vendors
)
