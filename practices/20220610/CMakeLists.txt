cmake_minimum_required(VERSION 3.16)

project(Factory)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(Factory)

target_sources(Factory
    PRIVATE factory.h factory.cpp
)

add_executable(Registry)

target_sources(Registry
    PRIVATE Registry.hpp Registry.cpp
)
