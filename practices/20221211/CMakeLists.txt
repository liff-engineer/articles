cmake_minimum_required(VERSION 3.16)

project(AnyState)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AnyState)

target_sources(AnyState
    PRIVATE example.cpp
)
