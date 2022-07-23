cmake_minimum_required(VERSION 3.16)

project(Actor)


add_executable(exActor)

target_sources(exActor
    PRIVATE exActor.cpp Actor.hpp Actor.cpp
)
