cmake_minimum_required(VERSION 3.16)

project(Modifyer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Modifyer)

target_sources(Modifyer
    PRIVATE DataRef.hpp 
            DataRefApp.cpp
)

add_executable(As)

target_sources(As
    PRIVATE As.hpp 
            AsApp.cpp
)
