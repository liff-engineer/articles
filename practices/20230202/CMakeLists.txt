cmake_minimum_required(VERSION 3.16)

project(value)

set(CMAKE_CXX_STANDARD 17)

add_executable(polymorphic
    polymorphic.cpp 
    polymorphic_value.h 
)

add_executable(indirect
    indirect.cpp 
    indirect_value.h 
)

add_executable(DynValue 
    DynValue.h 
    ExDynValue.cpp
)
