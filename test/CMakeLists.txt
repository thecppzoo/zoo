cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_UBSAN "-fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1 -g")
# set(CMAKE_EXE_LINKER_FLAGS_UBSAN -fsanitize=undefined)
# set(CMAKE_SHARED_LINKER_FLAGS_UBSAN -fsanitize=undefined)
set(CMAKE_CXX_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")


project(ZooTest VERSION 1.0 LANGUAGES CXX)

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

set(CATCH2_MAIN_SOURCE catch_main.cpp)
set(
    TYPE_ERASURE_SOURCES
    any.cpp AlignedStorage.cpp AnyCallable.cpp AnyCallSignature.cpp
    AnyExtended.cpp GenericPolicy.cpp FunctionPolicy.cpp
)
set(
    SWAR_SOURCES
    swar/BasicOperations.cpp
)
set(
    MAP_SOURCES
    map/BasicMap.cpp map/RobinHood.test.cpp
)
set(ALGORITHM_SOURCES algorithm/cfs.cpp algorithm/quicksort.cpp)
set(MISCELLANEA_SOURCES egyptian.cpp var.cpp variant.cpp CopyMoveAbilities.cpp)
set(
    ZOO_TEST_SOURCES
    ${CATCH2_MAIN_SOURCE} ${TYPE_ERASURE_SOURCES} ${ALGORITHM_SOURCES}
    ${SWAR_SOURCES} ${MAP_SOURCES}
    ${MISCELLANEA_SOURCES}
)

add_library(Catch2Main OBJECT ${CATCH2_MAIN_SOURCE})
add_library(AlgorithmTest OBJECT ${ALGORITHM_SOURCES})
add_library(TypeErasureTest OBJECT ${TYPE_ERASURE_SOURCES})
add_library(SWARTest OBJECT ${SWAR_SOURCES})
add_library(MapTest OBJECT ${MAP_SOURCES})
add_library(Uncategorized OBJECT ${MISCELLANEA_SOURCES})

add_executable(
    ${CURRENT_EXECUTABLE} ${ADDITIONAL_SOURCES}
)
target_link_libraries(${CURRENT_EXECUTABLE} Catch2Main AlgorithmTest TypeErasureTest SWARTest MapTest Uncategorized)

add_executable(algorithm2 $<TARGET_OBJECTS:Catch2Main>)
target_link_libraries(algorithm2 AlgorithmTest)
add_executable(type_erasure $<TARGET_OBJECTS:Catch2Main>)
target_link_libraries(type_erasure TypeErasureTest)
add_executable(swar $<TARGET_OBJECTS:Catch2Main>)
target_link_libraries(swar SWARTest)
add_executable(maptest $<TARGET_OBJECTS:Catch2Main>)
target_link_libraries(maptest MapTest)

# CMake build : library tests

#configure variables
set(TEST_APP_NAME "${CURRENT_EXECUTABLE}Test")

add_executable(${TEST_APP_NAME} ${ZOO_TEST_SOURCES})

#set includes
include_directories(${TEST_THIRD_PARTY_INCLUDE_PATH})

# Turn on CMake testing capabilities
enable_testing()

#parse catch tests
ParseAndAddCatchTests(${TEST_APP_NAME})
