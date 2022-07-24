cmake_minimum_required(VERSION 3.16)

project(editor)

include(DeployQtRuntime.cmake)

# 采用C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 并行编译
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  /MP")
endif()

# 控制VS工程生成使用"文件夹"组织结构
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# 统一动态库输出位置
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>") 
endif()

include(CMakePrintHelpers)
list(APPEND CMAKE_PREFIX_PATH $ENV{QTDIR})
cmake_print_variables(CMAKE_PREFIX_PATH)

add_subdirectory(source)
