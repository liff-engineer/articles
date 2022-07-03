include(GenerateExportHeader)

##设置target名称
set(TARGET_NAME Actor)

##设置源代码目录
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# 添加target及别名
add_library(${TARGET_NAME} SHARED)

# 创建动态库的符号导出头文件
generate_export_header(${TARGET_NAME}
    BASE_NAME "${TARGET_NAME}"
    EXPORT_FILE_NAME "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}Api.h"
)

# 指定源文件
target_sources(${TARGET_NAME}
    PRIVATE Actor.h Actor.cpp
)

# 设置预处理器定义
if(MSVC)
    target_compile_definitions(${TARGET_NAME}
        PRIVATE UNICODE NOMINMAX
    )
endif()

# 配置构建/使用时的头文件路径
target_include_directories(${TARGET_NAME} PUBLIC 
    "$<BUILD_INTERFACE:${SOURCE_DIR}>"
    "$<INSTALL_INTERFACE:include>"
)

##设置所在文件夹
set_target_properties(${TARGET_NAME} PROPERTIES
    FOLDER "libs"
)
