cmake_minimum_required(VERSION 3.16)

project(XMLWriter)

#设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#Qt配置
set(QtSdkPath $ENV{QTDIR}) ##环境变量的配置方法
set(QtSdkPath "C:/Qt/Qt5.12.12/5.12.12/msvc2017_64")#绝对路径的配置方法

list(APPEND CMAKE_PREFIX_PATH ${QtSdkPath})
include(DeployQtRuntime.cmake)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()
if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()


add_executable(example)

target_sources(example
    PRIVATE example.cpp
)

find_package(Qt5 COMPONENTS Xml REQUIRED)
target_link_libraries(example
    PRIVATE Qt5::Xml
)

DeployQtRuntime(TARGET example)
