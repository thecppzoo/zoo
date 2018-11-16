cmake_minimum_required (VERSION 3.0)

set(CMAKE_CXX_FLAGS -std=c++17)

set(CMAKE_CXX_FLAGS_UBSAN "-fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1 -g")
# set(CMAKE_EXE_LINKER_FLAGS_UBSAN -fsanitize=undefined)
# set(CMAKE_SHARED_LINKER_FLAGS_UBSAN -fsanitize=undefined)
set(CMAKE_CXX_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")


project(ZooDemonstrations VERSION 1.0)


include_directories(
    "${PROJECT_BINARY_DIR}"
    ../inc
    ../test/inc
    ../../metatools/inc
    ../../Catch2/single_include
)

if("UBSAN" STREQUAL "${CMAKE_BUILD_TYPE}")
    set(ADDITIONAL_SOURCES "")
endif()

message(STATUS "Additional sources:" ${ADDITIONAL_SOURCES})

add_compile_definitions(TESTS)

add_executable(
    zooDemonstrations${CMAKE_BUILD_TYPE}
    demoTightPolicy.cpp
    ../test/catch_main.cpp
     ${ADDITIONAL_SOURCES}
)
