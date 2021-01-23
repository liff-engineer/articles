cmake_minimum_required(VERSION 3.15)

project(GraphicsView)

#设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

##如何使用CMake配置Qt参见以下链接
##https://doc.qt.io/qt-5/cmake-get-started.html
list(APPEND CMAKE_PREFIX_PATH  "C:/Users/Garfield/Desktop/repos/modern-cmake-practice/build/_deps/qt-src/msvc2017_64")
list(APPEND CMAKE_PREFIX_PATH ENV{QTDIR})
list(APPEND CMAKE_MODULE_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(DeployQtRuntime)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()
if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()

add_executable(GraphicsView)
target_sources(GraphicsView
    PRIVATE main.cpp
    Dispatcher.hpp GraphicsViewHandler.hpp
)

set_target_properties(GraphicsView PROPERTIES
    AUTOMOC ON
)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(GraphicsView
    PRIVATE Qt5::Widgets
)

DeployQtRuntime(TARGET GraphicsView)
