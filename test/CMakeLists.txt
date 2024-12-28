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
        egyptian.cpp var.cpp
        # variant.cpp investigate why this is failing
        CopyMoveAbilities.cpp
        root/mem.cpp
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
        MESSAGE(
            FATAL_ERROR
            "Compilation of MSVC bug file succeeded, was the compiler bug fixed? ${RESULT}"
        )
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
        MESSAGE(
            STATUS "CMAKE try_compile succeeded as expected: ${MSVC_OK_OUTPUT}"
        )
    else()
        MESSAGE(
            FATAL_ERROR
            "CMAKE try_compile of non-problematic file did not succeed: ${MSVC_OK_OUTPUT}"
        )
    endif()
else()
    # Non-MSVC specific configuration
    set(CMAKE_CXX_STANDARD 17)

    # Add the third-party directory for dependencies
    add_subdirectory(third_party EXCLUDE_FROM_ALL)

    # Include directories for the project
    include_directories(
        "${PROJECT_BINARY_DIR}" ./inc ../inc ${TestThirdPartyIncludePath}
    )

    if("UBSAN" STREQUAL "${CMAKE_BUILD_TYPE}")
        set(AdditionalSources "ubsan.cpp")
    endif()

    message(STATUS "Additional sources: ${AdditionalSources}")

    # CMake policy for allowing arbitrary characters in test names
    cmake_policy(SET CMP0110 NEW)

    # Object libraries and executables
    add_library(
        Catch2Main OBJECT catch_main.cpp
    )

    function(addTestTarget targetName)
        set(sources ${ARGN})
        add_library(${targetName} OBJECT ${sources})
        add_executable(${targetName}_test $<TARGET_OBJECTS:${targetName}>)
        target_link_libraries(${targetName}_test ${targetName} Catch2Main)

        # Accumulate sources for the all-in-one target directly in the parent scope
        list(APPEND AllInOneSources ${sources})
        set(AllInOneSources "${AllInOneSources}" PARENT_SCOPE)
        message(STATUS "Added for ${targetName} \"${sources}\" resulting in ${AllInOneSources}")
    endfunction()

    # Global variable to accumulate all sources
    set(AllInOneSources "")

    addTestTarget(
        AlgorithmTest

        algorithm/cfs.cpp algorithm/quicksort.cpp
    )

    addTestTarget(
        TypeErasureTest

        any.cpp AlignedStorage.cpp AnyCallable.cpp AnyCallSignature.cpp
        AnyExtended.cpp GenericPolicy.cpp FunctionPolicy.cpp
    )

    addTestTarget(
        SWARTest

        swar/BasicOperations.cpp swar/sublanes.cpp
    )

    set(
        GeneticProgrammingSources
        gp/ArtificialAntEnvironment.cpp
    )

    # Add the executable target
    add_executable(
        cursesSantaFeTrail
        cursesSantaFe.cpp
        ${GeneticProgrammingSources}
    )

    target_link_libraries(cursesSantaFeTrail PRIVATE ncurses)

    addTestTarget(
        GeneticProgramming
        gp/GeneticProgramming.cpp
        ${GeneticProgrammingSources}
    )

    addTestTarget(
        Uncategorized

        egyptian.cpp var.cpp variant.cpp CopyMoveAbilities.cpp root/mem.cpp
        compilation/Iota.cpp
    )

    # Uncomment if needed
    # addTestTarget(
    #     MapTest
    #
    #     map/BasicMap.cpp map/RobinHood.test.cpp map/RobinHood.hybrid.test.cpp
    # )

    # Main executable
    set(CurrentExecutable "zooTest${CMAKE_BUILD_TYPE}")
    add_executable(
        ${CurrentExecutable} ${AdditionalSources}
    )
    target_link_libraries(
        ${CurrentExecutable}

        Catch2Main AlgorithmTest TypeErasureTest SWARTest Uncategorized
    )

    message(STATUS "All-in-one set: " "${AllInOneSources}")
    # All-in-one test app
    set(TestAppName "${CurrentExecutable}Test")
    add_executable(
        ${TestAppName}

        ${AllInOneSources}
        catch_main.cpp
    )

    # Use Catch2's `catch_discover_tests` for test integration
    catch_discover_tests(${TestAppName})
endif()
