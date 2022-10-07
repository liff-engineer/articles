cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(Graph)

add_executable(Graph)

target_sources(Graph
    PRIVATE Graph.cpp Graph.hpp AppGraph.cpp
)

