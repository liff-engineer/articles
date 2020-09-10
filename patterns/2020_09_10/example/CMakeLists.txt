cmake_minimum_required(VERSION 3.15)

project(unidirectional_data_flow)

#设置C++标准为C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#设置动态链接库输出目录
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>)
endif()

##从环境变量得到QT路径,也可以指定绝对路径
#set(QTDIR ENV{QTDIR})
set(QTDIR "C:/Users/Garfield/Desktop/repos/modern-cmake-practice/build/_deps/qt-src/msvc2017_64")
#追加到CMAKE_PREFIX_PATH确保find_package能够找到
list(APPEND CMAKE_PREFIX_PATH ${QTDIR}) 

##添加应用程序
add_executable(demo)

##指定源代码
target_sources(demo
    PRIVATE main.cpp
            action.hpp 
            dispatcher.hpp 
            store.hpp 
            view.hpp 
            view.cpp
)

##设置目标属性
set_target_properties(demo PROPERTIES 
    AUTOMOC ON #自动moc
)

##设置库依赖
find_package(Qt5 COMPONENTS Widgets Qml REQUIRED)
target_link_libraries(demo
    PRIVATE Qt5::Widgets  Qt5::Qml
)

##部署运行时依赖

#找到Qt的部署程序
#find_program(__qt5_deploy windeployqt PATHS ${QTDIR})
add_custom_command(TARGET demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E env QTDIR="${QTDIR}" "PATH=${QTDIR}/bin"  windeployqt.exe $<TARGET_FILE:demo> 
    COMMENT "deplot qt runtime dependencies"
)
