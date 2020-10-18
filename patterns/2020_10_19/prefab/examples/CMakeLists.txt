cmake_minimum_required(VERSION 3.15)

add_executable(example)

target_sources(example
    PRIVATE example.cpp
)

target_link_libraries(example 
    PRIVATE prefab
)
