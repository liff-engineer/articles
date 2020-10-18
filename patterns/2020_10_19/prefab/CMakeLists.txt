cmake_minimum_required(VERSION 3.15)

##工程配置
set(PROJECT_NAME prefab)

set(${PROJECT_NAME}_MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(${PROJECT_NAME}_MASTER_PROJECT ON)
  message(STATUS "CMake version: ${CMAKE_VERSION}")
endif ()

if(${PROJECT_NAME}_MASTER_PROJECT AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "默认构建类型")
endif()

option(${PROJECT_NAME}_INSTALL "生成install target,构建SDK使用" ${${PROJECT_NAME}_MASTER_PROJECT})

set(PROJECT_VERSION "1.0.0.0" CACHE STRING "默认工程版本号")

project(${PROJECT_NAME}
    LANGUAGES CXX C
    VERSION ${PROJECT_VERSION}
)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

## https://crascit.com/2015/03/28/enabling-cxx11-in-cmake/
# set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS OFF)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()
if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()

##库配置
set(TARGET_NAME prefab) 
set(${TARGET_NAME}_SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE  ${TARGET_NAME}_HEADER_FILES
    LIST_DIRECTORIES  False  CONFIGURE_DEPENDS
    "${${TARGET_NAME}_SOURCE_DIR}/include/*.h*"
)
file(GLOB_RECURSE  ${TARGET_NAME}_SRC_FILES
    LIST_DIRECTORIES  False  CONFIGURE_DEPENDS
    "${${TARGET_NAME}_SOURCE_DIR}/src/*.h*"
    "${${TARGET_NAME}_SOURCE_DIR}/src/*.c*"
    "${${TARGET_NAME}_SOURCE_DIR}/src/*.rc"
)

source_group(
    TREE "${${TARGET_NAME}_SOURCE_DIR}/include"
    PREFIX "Header Files"
    FILES ${${TARGET_NAME}_HEADER_FILES}
)
source_group(
    TREE "${${TARGET_NAME}_SOURCE_DIR}/src/"
    PREFIX "Source Files" 
    FILES ${${TARGET_NAME}_SRC_FILES}
)

add_library(${TARGET_NAME} SHARED)
add_library(${PROJECT_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})

target_sources(${TARGET_NAME}
    PRIVATE  ${${TARGET_NAME}_SRC_FILES}
    PRIVATE  ${${TARGET_NAME}_HEADER_FILES}
)

set_target_properties(${TARGET_NAME}  PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS  True  ##自动导出符号
    FOLDER  "prefab"  ##设置VS路径
)

target_include_directories(${TARGET_NAME}
    PUBLIC 
        "$<BUILD_INTERFACE:${${TARGET_NAME}_SOURCE_DIR}/include>"
        "$<INSTALL_INTERFACE:include>"
    PRIVATE
        "$<BUILD_INTERFACE:${${TARGET_NAME}_SOURCE_DIR}/src>"
)

if(${PROJECT_NAME}_INSTALL)
    ##安装头文件
    install(DIRECTORY
        ${${TARGET_NAME}_SOURCE_DIR}/include/
        DESTINATION include
        FILES_MATCHING PATTERN "*.h*"
    )

    ##安装库/应用程序文件
    install(TARGETS  ${TARGET_NAME}
        EXPORT  ${PROJECT_NAME}_Targets
        RUNTIME  DESTINATION  bin/$<CONFIG>
        LIBRARY  DESTINATION  lib/$<CONFIG>
        ARCHIVE  DESTINATION  lib/$<CONFIG>
    )

    ##安装pdb(可选)
    install(FILES  $<TARGET_PDB_FILE:${TARGET_NAME}> DESTINATION  bin/$<CONFIG>  OPTIONAL)
endif()

add_subdirectory(examples)








## 包安装支持
if(${PROJECT_NAME}_INSTALL)
    include(CMakePackageConfigHelpers)

    ##生成包配置文件
    configure_package_config_file(
        cmake/PackageConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION lib/cmake/${PROJECT_NAME}
    )

    ##生成包版本文件
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        VERSION ${CMAKE_PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion
    )

    ##安装find_package支持文件
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION lib/cmake/${PROJECT_NAME}
    )

    ##安装导出目标
    install(EXPORT ${PROJECT_NAME}_Targets
        NAMESPACE   ${PROJECT_NAME}::
        FILE        ${PROJECT_NAME}--Targets.cmake
        DESTINATION lib/cmake/${PROJECT_NAME}
    )
endif()
