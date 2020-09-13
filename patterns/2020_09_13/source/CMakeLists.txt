cmake_minimum_required(VERSION 3.15)

project(Concept)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(NormalImpl)

target_sources(NormalImpl
    PRIVATE NormalImpl.cpp
)

add_executable(DynConceptImpl)

target_sources(DynConceptImpl
    PRIVATE DynConceptImpl.cpp
)
