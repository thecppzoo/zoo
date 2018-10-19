cmake_minimum_required (VERSION 2.6)

set(ZooTest_VERSION_MAJOR 1)
set(ZooTest_VERSION_MINOR 0)
set(CMAKE_CXX_FLAGS -std=c++17)

project(ZooTest)

configure_file (
    "${PROJECT_SOURCE_DIR}/ZooTestConfig.h.in"
    "${PROJECT_BINARY_DIR}/ZooTestConfig.h"
)

include_directories(
    "${PROJECT_BINARY_DIR}"
    ../../Catch2/single_include
    ../inc
)

add_executable(zooTest catch_main.cpp algorithm/cfs.cpp)
