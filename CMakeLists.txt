cmake_minimum_required(VERSION 3.12)
project(zoo LANGUAGES CXX)
include(${CMAKE_CURRENT_BINARY_DIR}/nlo_link.cmake OPTIONAL)

# library core
add_library(zoo INTERFACE)
target_include_directories(zoo INTERFACE inc)

nlo_export(zoo)
