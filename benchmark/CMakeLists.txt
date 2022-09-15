cmake_minimum_required (VERSION 2.6)

set(CMAKE_CXX_FLAGS "-std=c++17 -g3 -march=native -fno-unroll-loops")

set(CMAKE_BUILD_TYPE "Release")

project(ZooBenchmark)

include_directories(
    ../../benchmark/include
    ../inc
    ../junkyard/inc
    ../test/third_party/Catch2/single_include
)

# build google benchmark (target: benchmark)
# do not build tests of benchmarking lib
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)

add_subdirectory(dependencies/google_benchmark)

add_executable(
    zooBenchmark
        catch2BenchmarkMain.cpp
        cfs.cpp cfs/cfs_utility.cpp sort.cpp  catch2Functions.cpp
)

add_executable(
  catch2Benchmark
    catch2BenchmarkMain.cpp catch2Functions.cpp egyptian.cpp RobinHood.cpp
)

target_link_libraries(zooBenchmark benchmark)
