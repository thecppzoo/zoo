cmake_minimum_required (VERSION 3.8)

set(CMAKE_BUILD_TYPE Debug)


project(ZooTest VERSION 1.0 LANGUAGES CXX)

configure_file (
    "${PROJECT_SOURCE_DIR}/ZooTestConfig.h.in"
    "${PROJECT_BINARY_DIR}/ZooTestConfig.h"
)

if(MSVC)
    # MSVC specific configuration
    # Avoids multiple problems

    # Due to multiple bugs, forced upgrade to C++ 20
    set(CMAKE_CXX_STANDARD 20)

    # Set the policy to use the new behavior
    if(POLICY CMP0067)
        cmake_policy(SET CMP0067 NEW)
        message(STATUS "Set policy")
    endif()

    include_directories(
        "${PROJECT_BINARY_DIR}"
        ./inc
        ../inc
        ./third_party/Catch2/single_include
    )

    set(
        ZOO_TEST_SOURCES
        catch_main.cpp
        any.cpp AlignedStorage.cpp AnyCallable.cpp AnyCallSignature.cpp
        AnyExtended.cpp GenericPolicy.cpp FunctionPolicy.cpp
        swar/BasicOperations.cpp
        # map/BasicMap.cpp
        # map/RobinHood.test.cpp
        # map/RobinHood.hybrid.test.cpp
        algorithm/cfs.cpp
        algorithm/quicksort.cpp
        egyptian.cpp var.cpp variant.cpp CopyMoveAbilities.cpp
    )

    add_subdirectory(third_party EXCLUDE_FROM_ALL)

    # Simple executable for MSVC
    add_executable(ZooTest ${ZOO_TEST_SOURCES})
    enable_testing()
    ParseAndAddCatchTests(ZooTest)

    try_compile(
        MSVC_BUG_BUILD_RESULT
        ${CMAKE_BINARY_DIR}/temporary
        SOURCES
            ${CMAKE_SOURCE_DIR}/../compiler_bugs/msvc/sfinae.cpp
        CMAKE_FLAGS "-DCMAKE_CXX_STANDARD=17"
        COMPILE_DEFINITIONS
            -DTRIGGER_MSVC_SFINAE_BUG
        OUTPUT_VARIABLE RESULT
    )
    if(MSVC_BUG_BUILD_RESULT)
        MESSAGE(FATAL_ERROR "Compilation of MSVC bug file succeeded, was the compiler bug fixed? ${RESULT}")
    else()
        MESSAGE(STATUS "File with MSVC bug build failed as expected ${RESULT}")
    endif()
    try_compile(
        MSVC_OK_BUILD_RESULT
        ${CMAKE_BINARY_DIR}/temporary
        SOURCES
            ${CMAKE_SOURCE_DIR}/../compiler_bugs/msvc/sfinae.cpp
        CMAKE_FLAGS -DCMAKE_CXX_STANDARD=20
        OUTPUT_VARIABLE MSVC_OK_OUTPUT
    )
    if(MSVC_OK_BUILD_RESULT)
        MESSAGE(STATUS "CMAKE try_compile succeeded as expected: ${MSVC_OK_OUTPUT}")
    else()
        MESSAGE(FATAL_ERROR "CMAKE try_compile of non-problematic file did not succeed: ${MSVC_OK_OUTPUT}")
    endif()
else()
    # Non-MSVC specific configuration (original content)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_FLAGS_UBSAN "-fsanitize=undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls -O1 -g")
    set(CMAKE_CXX_FLAGS_ASAN "-fsanitize=address -fno-omit-frame-pointer")

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
        swar/BasicOperations.cpp swar/sublanes.cpp
    )
    set(
        MAP_SOURCES
        map/BasicMap.cpp map/RobinHood.test.cpp map/RobinHood.hybrid.test.cpp
    )
    set(ALGORITHM_SOURCES algorithm/cfs.cpp algorithm/quicksort.cpp)
    set(MISCELLANEA_SOURCES egyptian.cpp var.cpp variant.cpp CopyMoveAbilities.cpp)
    set(
        ZOO_TEST_SOURCES
        ${CATCH2_MAIN_SOURCE} ${TYPE_ERASURE_SOURCES} ${ALGORITHM_SOURCES}
        ${SWAR_SOURCES} 
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
    target_link_libraries(${CURRENT_EXECUTABLE} Catch2Main AlgorithmTest TypeErasureTest SWARTest Uncategorized)

    add_executable(algorithm2 $<TARGET_OBJECTS:Catch2Main>)
    target_link_libraries(algorithm2 AlgorithmTest)
    add_executable(type_erasure $<TARGET_OBJECTS:Catch2Main>)
    target_link_libraries(type_erasure TypeErasureTest)
    add_executable(swar $<TARGET_OBJECTS:Catch2Main>)
    target_link_libraries(swar SWARTest)
    add_executable(mapt $<TARGET_OBJECTS:Catch2Main>)
    target_link_libraries(mapt MapTest)

    # CMake build: library tests
    set(TEST_APP_NAME "${CURRENT_EXECUTABLE}Test")
    add_executable(${TEST_APP_NAME} ${ZOO_TEST_SOURCES})
    include_directories(${TEST_THIRD_PARTY_INCLUDE_PATH})
    enable_testing()
    ParseAndAddCatchTests(${TEST_APP_NAME})
endif()
