cmake_minimum_required(VERSION 3.16)

project(ReqRep)

#设置使用C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#生成Visual Studio工程时使用文件夹组织结构
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

add_executable(broker)

target_sources(broker
    PRIVATE 
        broker.hpp app.cpp
)
