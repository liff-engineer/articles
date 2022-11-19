cmake_minimum_required(VERSION 3.16)

project(Registry)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Registry)

target_sources(Registry
    PRIVATE RegistryStub.hpp RegistryTemplate.hpp
            ICommand.hpp IExtension.hpp 
            ICommand.cpp IExtension.cpp 
            CommandA.cpp CommandB.cpp Commands.cpp
            ExtensionAB.cpp Extensions.cpp
            AppRegistry.cpp
)
