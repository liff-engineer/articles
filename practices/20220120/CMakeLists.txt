cmake_minimum_required(VERSION 3.16)

project(iterator)

add_executable(example)

target_sources(example
    PRIVATE iterator_facade.h 
    example.cpp
)
