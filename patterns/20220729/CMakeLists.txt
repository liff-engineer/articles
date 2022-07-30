cmake_minimum_required(VERSION 3.16)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(Aggregator)

add_executable(Aggregator)

target_sources(Aggregator
    PRIVATE Aggregator.cpp
)
