cmake_minimum_required(VERSION 3.14)
project(ecewo_hello_world VERSION 1.0.0 LANGUAGES C)

if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
if(POLICY CMP0092)
    cmake_policy(SET CMP0092 NEW)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Release build" FORCE)

if(NOT MSVC)
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
    
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -flto")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fomit-frame-pointer")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -funroll-loops")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -ffast-math")
    
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -flto")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
endif()

include(FetchContent)

FetchContent_Declare(
    ecewo
    GIT_REPOSITORY https://github.com/savashn/ecewo.git
    GIT_TAG v3.0.1
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(ecewo)

add_executable(server
    main.c
    ecewo-cluster.c
)

target_link_libraries(server PRIVATE ecewo)
