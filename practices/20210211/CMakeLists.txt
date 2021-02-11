cmake_minimum_required(VERSION 3.15)

project(factory)

#设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(example)
target_sources(example
    PRIVATE example.cpp factory.hpp 
)

