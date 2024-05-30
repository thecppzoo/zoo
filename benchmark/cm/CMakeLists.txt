cmake_minimum_required(VERSION 3.8)

project(CMB)

set(CMAKE_CXX_STANDARD 17)

add_executable(
    cmb
    main.cpp
)

target_include_directories(cmb PRIVATE "/home/eddie/git/zoo-cm/inc")

