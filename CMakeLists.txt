cmake_minimum_required(VERSION 3.17)

project(zoo VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
option(ENABLE_TESTING "Enable testing" OFF)

# Include directories
include_directories(inc)

# Get all header files in the 'inc' directory
file(GLOB HEADERS "inc/*.h" "inc/*.hpp")

# Get all source files in the 'src' directory
file(GLOB SOURCES "src/*.cpp")

# Create a library target
add_library(zoo ${SOURCES} ${HEADERS})

# Set include directories for the library
target_include_directories(zoo PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/inc>
    $<INSTALL_INTERFACE:include>
)

# Installation
install(TARGETS zoo EXPORT zoo-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES ${HEADERS} DESTINATION include)

install(EXPORT zoo-targets
    FILE zoo-config.cmake
    NAMESPACE zoo::
    DESTINATION lib/cmake/zoo
)

export(TARGETS zoo FILE zoo-config.cmake)

if (ENABLE_TESTING)
    enable_testing()
    add_subdirectory(test)
endif()
