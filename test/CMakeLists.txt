cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_UBSAN "-fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1 -g")
# set(CMAKE_EXE_LINKER_FLAGS_UBSAN -fsanitize=undefined)
# set(CMAKE_SHARED_LINKER_FLAGS_UBSAN -fsanitize=undefined)
set(CMAKE_CXX_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")


project(ZooTest VERSION 1.0)

configure_file (
    "${PROJECT_SOURCE_DIR}/ZooTestConfig.h.in"
    "${PROJECT_BINARY_DIR}/ZooTestConfig.h"
)

add_subdirectory(third_party EXCLUDE_FROM_ALL)
enable_testing()

include_directories(
    "${PROJECT_BINARY_DIR}"
    ./inc
    ../inc
    ${TEST_THIRD_PARTY_INCLUDE_PATH}
)

if("UBSAN" STREQUAL "${CMAKE_BUILD_TYPE}")
    set(ADDITIONAL_SOURCES "ubsan.cpp")
endif()

message(STATUS "Additional sources:" ${ADDITIONAL_SOURCES})

add_executable(
    zooTest${CMAKE_BUILD_TYPE}
    catch_main.cpp variant.cpp AnyCallable.cpp AnyExtended.cpp any.cpp
    algorithm/cfs.cpp algorithm/quicksort.cpp egyptian.cpp
    ${ADDITIONAL_SOURCES}
)
