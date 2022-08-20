cmake_minimum_required(VERSION 3.16)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(ContainerView)

add_executable(ContainerView)

target_sources(ContainerView
    PRIVATE ContainerView.cpp
)
