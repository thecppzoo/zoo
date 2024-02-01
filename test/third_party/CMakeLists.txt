# CMake build : third party

message(STATUS "cmake-csd ${CMAKE_CURRENT_SOURCE_DIR}")

# Configure directories
set(THIRD_PARTY_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}")

# Catch

# Configure directories
set(CATCH_MODULE_PATH "${THIRD_PARTY_MODULE_PATH}/Catch2")
set(CATCH_INCLUDE_PATH "${CATCH_MODULE_PATH}/single_include")

# Include custom cmake function
include("${CATCH_MODULE_PATH}/contrib/ParseAndAddCatchTests.cmake")

# Set variables for tests
set(TEST_THIRD_PARTY_INCLUDE_PATH ${CATCH_INCLUDE_PATH})

# Export vars
set(TEST_THIRD_PARTY_INCLUDE_PATH ${TEST_THIRD_PARTY_INCLUDE_PATH} PARENT_SCOPE)
