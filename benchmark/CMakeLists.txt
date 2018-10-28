cmake_minimum_required (VERSION 2.6)

set(CMAKE_CXX_FLAGS "-std=c++17 -g3 -fno-unroll-loops")

set(CMAKE_BUILD_TYPE "Release")

project(ZooBenchmark)

include_directories(
    ../../benchmark/include
    ../inc
)

link_directories(
    /Users/ed/git/benchmarkbuild/src
)

add_executable(zooBenchmark cfs.cpp cfs/cfs_utility.cpp)

target_link_libraries(zooBenchmark benchmark)
