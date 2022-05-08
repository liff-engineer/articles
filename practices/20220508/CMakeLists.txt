cmake_minimum_required(VERSION 3.16)

project(TypeRegistry)


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")

## 基于类型ID的公共库
add_library(TypeRegistry SHARED)

target_sources(TypeRegistry
    PRIVATE 
        TypeRegistry.hpp 
        TypeRegistry.cpp
        AnySubject.hpp 
        AnySubject.cpp
)

target_compile_definitions(TypeRegistry
    PRIVATE TYPE_REGISTRY_EXPORT
)

## 使用公共库的动态加载库A
add_library(AUser SHARED)

target_sources(AUser
    PRIVATE AUser.cpp 
)

target_link_libraries(AUser
    PRIVATE TypeRegistry
)

## 使用公共库的动态加载库A
add_library(BUser SHARED)

target_sources(BUser
    PRIVATE BUser.cpp 
)

target_link_libraries(BUser
    PRIVATE TypeRegistry
)

## 加载动态库A、B的应用程序
add_executable(App)

target_sources(App
    PRIVATE App.cpp 
)

target_link_libraries(App
    PRIVATE TypeRegistry
)
