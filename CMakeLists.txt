cmake_minimum_required(VERSION 3.14)

project(zoo CXX)

# ─── Consumer surface ────────────────────────────────────────────────
# zoo is header-only. Consumers (e.g. via CPM / add_subdirectory) link the
# `zoo` INTERFACE target and get inc/ on their include path as SYSTEM, so
# zoo's own headers never trip a downstream project's -Werror. The guard
# lets the same source tree be pulled in more than once without redefining
# the target.
if(NOT TARGET zoo)
    add_library(zoo INTERFACE)
    add_library(zoo::zoo ALIAS zoo)
    target_include_directories(zoo SYSTEM INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>)
    target_compile_features(zoo INTERFACE cxx_std_17)
endif()

# ─── Dev builds (tests / benchmarks) ─────────────────────────────────
# Only on by default when zoo is the top-level project. When zoo is a
# dependency these stay OFF, so the test/benchmark third-party fetches
# (Catch2, google-benchmark) never run for a consumer. PROJECT_IS_TOP_LEVEL
# needs CMake 3.21, so derive it ourselves to keep the 3.14 floor.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(ZOO_IS_TOP_LEVEL ON)
else()
    set(ZOO_IS_TOP_LEVEL OFF)
endif()

option(ZOO_BUILD_TESTS "Build zoo's Catch2 test suite" ${ZOO_IS_TOP_LEVEL})
option(ZOO_BUILD_BENCHMARKS "Build zoo's benchmarks" ${ZOO_IS_TOP_LEVEL})

if(ZOO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()

if(ZOO_BUILD_BENCHMARKS)
    add_subdirectory(benchmark)
endif()
