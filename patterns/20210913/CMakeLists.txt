cmake_minimum_required(VERSION 3.16)

project(broker)

add_executable(example)

target_sources(example
    PRIVATE example.cpp Broker.hpp JsonIO.hpp JsonIO.cpp
)

target_include_directories(example
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
)
