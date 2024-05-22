cmake_minimum_required(VERSION 3.17)

project(zoo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# add header files
include_directories(inc)

# add source files
file(GLOB SOURCES "src/*.cpp")


# Add the include directory
include_directories("include")

# Create a library target
add_library(zoo ${SOURCES})

message(STATUS "Hello from zoo")

# Install the library and headers
install(TARGETS zoo DESTINATION zoo/lib)
install(DIRECTORY include/ DESTINATION include)




