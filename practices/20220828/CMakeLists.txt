cmake_minimum_required(VERSION 3.16)

project(task_registry)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(task_registry)

target_sources(task_registry
    PRIVATE TaskRegistry.hpp 
            example.cpp
)
