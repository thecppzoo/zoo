cmake_minimum_required(VERSION 3.17)

project(zoo VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)

option (ENABLE_TESTS "Build tests" ON)
# todo jamie the benchmarks don't build from the root CMakeLists.txt
option (ENABLE_BENCHMARKS "Build benchmarks" OFF)

if (ENABLE_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if (ENABLE_BENCHMARKS)
    add_subdirectory(benchmark)
endif()
