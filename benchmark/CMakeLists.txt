cmake_minimum_required (VERSION 3.8)

project(ZooBenchmark)


set(CMAKE_CXX_FLAGS "-fno-unroll-loops")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "RelWithDebInfo")

if(CMAKE_BUILD_TYPE MATCHES "Release|RelWithDebInfo")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        string(APPEND CMAKE_CXX_FLAGS_RELEASE " -march=native -fno-unroll-loops")
        string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -march=native -fno-unroll-loops")
    endif()
endif()

macro(set_xcode_properties TARGET_NAME)
    if(
        CMAKE_GENERATOR STREQUAL Xcode AND
        CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64"
    )
        target_compile_options(
            # Todo: Introduce support for AVX512BW
            ${TARGET_NAME} PRIVATE -mavx -mavx2 -mbmi2
        )
    endif()
endmacro()

include_directories(
    ../test/inc
    dependencies/google_benchmark/include
    ../inc
    ../junkyard/inc
    ../test/third_party/Catch2/single_include
)

# build google benchmark (target: benchmark)
# do not build tests of benchmarking lib
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Suppressing benchmark's tests" FORCE)
set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
add_subdirectory(dependencies/google_benchmark)

add_executable(
  catch2Benchmark
    catch2BenchmarkMain.cpp catch2Functions.cpp catch2swar-demo.cpp
    c_str-functions/c_str.cpp c_str-functions/catch2.cpp
    egyptian.cpp
    # RobinHood.benchmark.cpp
)
set_xcode_properties(catch2Benchmark)

add_executable(
    zoo-google-benchmark benchmark_main.cpp cfs.cpp cfs/cfs_utility.cpp
)
set_xcode_properties(zoo-google-benchmark)

add_executable(
    zoo-demo-benchmark
    benchmark_main.cpp bm-swar.cpp c_str-functions/c_str.cpp swar/compress.cpp
)
set_xcode_properties(zoo-demo-benchmark)

target_link_libraries(zoo-google-benchmark benchmark::benchmark)
target_link_libraries(zoo-demo-benchmark benchmark::benchmark)

add_library(zoo-c_str-implementations SHARED c_str-functions/c_str.cpp)
add_executable(
    zoo-c_str-benchmark-from-dynamic-library benchmark_main.cpp bm-swar.cpp
)
target_link_libraries(
    zoo-c_str-benchmark-from-dynamic-library
    zoo-c_str-implementations
    benchmark::benchmark
)
