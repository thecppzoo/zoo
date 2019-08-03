# CMake build : third party

#configure directories
set (THIRD_PARTY_MODULE_PATH "${PROJECT_SOURCE_DIR}/third_party")

# catch2

#configure directories
set (CATCH2_MODULE_PATH "${THIRD_PARTY_MODULE_PATH}/Catch2")
set (CATCH2_INCLUDE_PATH "${CATCH2_MODULE_PATH}/single_include")

#include custom cmake function
include ( "${CATCH2_MODULE_PATH}/contrib/ParseAndAddCatchTests.cmake")
