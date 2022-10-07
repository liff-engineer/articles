cmake_minimum_required(VERSION 3.15)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(DataSet)

add_executable(DRGraph)

target_sources(DRGraph
    PRIVATE
	DRGraph.hpp DRGraph.cpp
	Graph.hpp Graph.cpp
	DRGraphApp.cpp
)

