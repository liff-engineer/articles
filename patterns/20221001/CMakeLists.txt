cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(Project)

add_executable(Project)

target_sources(Project
    PRIVATE
		Project.cpp
    DataSet.h DataSet.cpp DataSetCluster.hpp
)

