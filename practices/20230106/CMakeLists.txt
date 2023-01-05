cmake_minimum_required(VERSION 3.16)

project(message)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(message)

target_sources(message
    PRIVATE example.cpp message.hpp message.cpp
)
