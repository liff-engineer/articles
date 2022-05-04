cmake_minimum_required(VERSION 3.16)

project(archive)

set(CMAKE_SHARED_LIBRARY_PREFIX "Abc")
set(CMAKE_IMPORT_LIBRARY_PREFIX "Abc")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(archive)
add_subdirectory(examples)
