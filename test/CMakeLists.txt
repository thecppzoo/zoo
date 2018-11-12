cmake_minimum_required (VERSION 3.0)

set(CMAKE_CXX_FLAGS -std=c++17)

set(CMAKE_CXX_FLAGS_UBSAN "-fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1 -g")
# set(CMAKE_EXE_LINKER_FLAGS_UBSAN -fsanitize=undefined)
# set(CMAKE_SHARED_LINKER_FLAGS_UBSAN -fsanitize=undefined)
set(CMAKE_CXX_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")


project(ZooTest VERSION 1.0)

configure_file (
    "${PROJECT_SOURCE_DIR}/ZooTestConfig.h.in"
    "${PROJECT_BINARY_DIR}/ZooTestConfig.h"
)

include_directories(
    "${PROJECT_BINARY_DIR}"
    ./inc
    ../inc
    ../../metatools/inc
    ../../Catch2/single_include
)

if("UBSAN" STREQUAL "${CMAKE_BUILD_TYPE}")
    set(ADDITIONAL_SOURCES "ubsan.cpp")
endif()

message(STATUS "Additional sources:" ${ADDITIONAL_SOURCES})

add_executable(
    zooTest${CMAKE_BUILD_TYPE}
    catch_main.cpp AnyCallable.cpp AnyExtended.cpp any.cpp
     algorithm/cfs.cpp algorithm/quicksort.cpp
     ${ADDITIONAL_SOURCES}
)
