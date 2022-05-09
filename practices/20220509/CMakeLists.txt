cmake_minimum_required(VERSION 3.16)

project(repository)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(example)

target_sources(example
    PRIVATE example.cpp
	RepoImpl.hpp Repo.hpp Repo.cpp
)
