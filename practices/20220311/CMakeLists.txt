cmake_minimum_required(VERSION 3.16)

project(chain)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(example)

target_sources(example
    PRIVATE chain.hpp example.cpp
)
