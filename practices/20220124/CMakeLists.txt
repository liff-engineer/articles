cmake_minimum_required(VERSION 3.16)

project(values)

add_executable(example)

target_sources(example
    PRIVATE complex_value.hpp 
    example.cpp
)
