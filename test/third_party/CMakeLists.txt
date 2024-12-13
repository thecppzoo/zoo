# CMake build: third party

message(STATUS "cmake-csd ${CMAKE_CURRENT_SOURCE_DIR}")

# Configure directories
set(ThirdPartyModulePath "${CMAKE_CURRENT_SOURCE_DIR}")

# Catch2 setup
set(CatchModulePath "${ThirdPartyModulePath}/Catch2")
set(CatchIncludePath "${CatchModulePath}/single_include")
set(CatchCMakeFile "${CatchModulePath}/contrib/Catch.cmake")

# Verify Catch.cmake exists
if(NOT EXISTS "${CatchCMakeFile}")
    message(FATAL_ERROR "Catch.cmake not found at ${CatchCMakeFile}")
endif()

# Include Catch2's Catch.cmake
include("${CatchCMakeFile}")

# Set the include path for tests
set(TestThirdPartyIncludePath "${CatchIncludePath}")

# Export include path for other parts of the project
set(TestThirdPartyIncludePath "${TestThirdPartyIncludePath}" PARENT_SCOPE)
