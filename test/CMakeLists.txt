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

set(CURRENT_EXECUTABLE "zooTest${CMAKE_BUILD_TYPE}")

add_library(Catch2Main OBJECT catch_main.cpp)
add_library(AlgorithmTest OBJECT algorithm/cfs.cpp algorithm/quicksort.cpp)
add_library(TypeErasureTest OBJECT any.cpp AlignedStorage.cpp AnyCallable.cpp AnyExtended.cpp)
add_library(
    Uncategorized OBJECT
    egyptian.cpp var.cpp variant.cpp CopyMoveAbilities.cpp
)

add_executable(
    ${CURRENT_EXECUTABLE} ${ADDITIONAL_SOURCES}
)
target_link_libraries(${CURRENT_EXECUTABLE} Catch2Main AlgorithmTest TypeErasureTest Uncategorized)

add_executable(algorithm $<TARGET_OBJECTS:Catch2Main>)
target_link_libraries(algorithm AlgorithmTest)
add_executable(type_erasure $<TARGET_OBJECTS:Catch2Main>)
target_link_libraries(type_erasure TypeErasureTest)

# CMake build : library tests

#configure variables
set(TEST_APP_NAME "${CURRENT_EXECUTABLE}Test")

add_executable(${TEST_APP_NAME} $<TARGET_OBJECTS:Catch2Main>)
target_link_libraries(${TEST_APP_NAME} AlgorithmTest TypeErasureTest Uncategorized)

#set includes
include_directories(${TEST_THIRD_PARTY_INCLUDE_PATH})

# Turn on CMake testing capabilities
enable_testing()

#parse catch tests
ParseAndAddCatchTests(${TEST_APP_NAME})
