# CMake build : third party
#
# Catch2 used to be a git submodule under Catch2/. It is now fetched with
# FetchContent so the repo is submodule-free and stays CPM/clone friendly.
# We pin the same release the submodule tracked (v2.13.10) and keep using
# the single-header include + the ParseAndAddCatchTests contrib script, so
# the rest of the test build is unchanged.

include(FetchContent)

FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v2.13.10
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

# Point the existing variables at the fetched source tree.
set(CATCH_MODULE_PATH "${catch2_SOURCE_DIR}")
set(CATCH_INCLUDE_PATH "${CATCH_MODULE_PATH}/single_include")

# Include custom cmake function (single-header CTest integration).
include("${CATCH_MODULE_PATH}/contrib/ParseAndAddCatchTests.cmake")

# Export the include path to the parent (test/) scope.
set(TEST_THIRD_PARTY_INCLUDE_PATH ${CATCH_INCLUDE_PATH} PARENT_SCOPE)
