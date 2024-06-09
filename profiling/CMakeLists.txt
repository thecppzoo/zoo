cmake_minimum_required (VERSION 3.8)

project(ZooProfiling)


set(CMAKE_CXX_FLAGS "-fno-unroll-loops")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE "RelWithDebInfo")

include_directories(
	.
	../inc
    ../test/inc
    ../test/third_party/Catch2/single_include
)

add_executable(
	profiling-catch2-zoo
	catch2-main.cpp
	swar/multiplication.cpp
)
