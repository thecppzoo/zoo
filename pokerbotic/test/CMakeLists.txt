cmake_minimum_required (VERSION 3.8)

set(CMAKE_BUILD_TYPE Debug)


project(ZooPokerbotic VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(
    ../../test/third_party EXCLUDE_FROM_ALL
    ${CMAKE_CURRENT_BINARY_DIR}/3rdParty
)

MESSAGE(STATUS "TTP ${TEST_THIRD_PARTY_INCLUDE_PATH} ")

include_directories(
    "${PROJECT_BINARY_DIR}"
    ./inc # inc inside pokerbotic/test
    ../../inc # the zoo library headers
    ${TEST_THIRD_PARTY_INCLUDE_PATH}
)

add_executable(
    pokerbotic-tests catch2_main.cpp tests.cpp
)
