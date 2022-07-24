set(TARGET_NAME editor)

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})


## 提取头文件
file(GLOB_RECURSE  HEADER_FILES
    LIST_DIRECTORIES False  CONFIGURE_DEPENDS
    "${SOURCE_DIR}/*.h*"
)

## 提取源文件
file(GLOB_RECURSE  SOURCE_FILES
    LIST_DIRECTORIES False  CONFIGURE_DEPENDS
    "${SOURCE_DIR}/*.c*"
)

# 为VS设置源代码文件夹
source_group(
    TREE "${SOURCE_DIR}"
    PREFIX "头文件"
    FILES ${HEADER_FILES}
)

source_group(
    TREE "${SOURCE_DIR}"
    PREFIX "源文件"
    FILES ${SOURCE_FILES}
)

add_executable(${TARGET_NAME})

target_sources(${TARGET_NAME}
    PRIVATE ${HEADER_FILES}
    PRIVATE ${SOURCE_FILES}
)

# 设置预处理器定义
if(MSVC)
    target_compile_definitions(${TARGET_NAME}
        PRIVATE UNICODE NOMINMAX
    )
endif()

# 配置构建/使用时的头文件路径
target_include_directories(${TARGET_NAME} 
    PRIVATE 
        "${SOURCE_DIR}"
)

target_precompile_headers(${TARGET_NAME}
    PRIVATE 
        [["qevent.h"]]
        <functional>
        <QObject>
        <QtWidgets/QGraphicsView>
        <QtCore/qvariant.h>
)

set_target_properties(${TARGET_NAME} PROPERTIES
    AUTOMOC ON
)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
target_link_libraries(${TARGET_NAME} 
    PRIVATE
        Qt5::Widgets
)

DeployQtRuntime(
    TARGET ${TARGET_NAME}    
)
