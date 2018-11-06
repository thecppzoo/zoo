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
    ./inc
    ../inc
    ../../metatools/inc
    ../../Catch2/single_include
)

add_executable(
    zooTest
    AnyCallable.cpp algorithm/cfs.cpp AnyExtended.cpp any.cpp catch_main.cpp
)
