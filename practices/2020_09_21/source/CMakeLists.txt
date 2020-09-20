cmake_minimum_required(VERSION 3.15)

project(codegen)

add_executable(codegen)
target_sources(codegen
    PRIVATE example.hpp 
    example_ostream.hpp 
    main.cpp
)