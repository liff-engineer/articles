cmake_minimum_required(VERSION 3.15)

project(serializer)

#配置cmake脚本模块路径
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(create_members_file)

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(example)

create_members_file(
    OUTPUT example.members.hpp
    INPUTS example.hpp
)

target_sources(example
    PRIVATE example.cpp members.hpp print.hpp example.hpp
    PRIVATE example.members.hpp
)
