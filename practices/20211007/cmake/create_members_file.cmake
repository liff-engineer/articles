# 基于libclang为结构体、枚举生成成员遍历实现
# 
# - OUTPUT为输出的头文件
# - INTPUS为要处理的头文件列表
# 如果文件不提供绝对路径,则会默认为当前源代码路径
# 只需要将output作为源码添加到target即可添加到构建依赖,进行代码生成
#
# 只要输入头文件、代码生成脚本发生变化,构建时就会自动刷新,如无变化且
# 输出头文件存在,则不会执行代码生成.

set(__members_codegen_path ${CMAKE_CURRENT_LIST_DIR})
function(create_members_file)
    set(oneValueArgs   OUTPUT)
    set(multiValueArgs INPUTS)
    cmake_parse_arguments(Gen "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    #从开发者配置的OUTPUT获取绝对路径
    get_filename_component(output "${Gen_OUTPUT}" ABSOLUTE)

    #将输入文件转换为绝对路径
    foreach(file ${Gen_INPUTS})
        get_filename_component(file_ ${file} ABSOLUTE)
        list(APPEND files ${file_})
    endforeach()

    find_package(Python COMPONENTS Interpreter Development REQUIRED)
    add_custom_command(
        #注意OUTPUT要提供完整路径,默认路径并非CMAKE_CURRENT_SOURCE_DIR
        #如果生成的文件路径与其不一致,会导致CMake一直认为缺少输出文件
        #从而一直触发该自定义命令执行
        OUTPUT ${output}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND  "${Python_EXECUTABLE}" "${__members_codegen_path}/members.py" 
            ${files}
            -o ${output}
        DEPENDS "${__members_codegen_path}/members.py" ${files}
        VERBATIM 
    )
endfunction()