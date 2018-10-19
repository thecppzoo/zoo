cmake_minimum_required (VERSION 2.6)

set(CMAKE_CXX_FLAGS -std=c++17)

project(ZooBenchmark)

include_directories(
    ../../benchmark/include
    ../inc
)

link_directories(
    /Users/ed/git/benchmarkbuild/src
)

add_executable(zooBenchmark cfs.cpp)

target_link_libraries(zooBenchmark benchmark)
