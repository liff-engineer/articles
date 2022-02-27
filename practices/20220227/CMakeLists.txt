cmake_minimum_required(VERSION 3.16)

project(libUser)

#IDE使用文件夹分组
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

###########
##配置SFML库
###########
include(CMakeParseArguments) #书写CMake函数需要

#从环境变量读取SFML包解压路径
set(SFMLPath  $ENV{SFMLSdkPath})

#从相对/绝对路径读取SFML包解压路径
set(SFMLPath  "${CMAKE_SOURCE_DIR}/downloads/SFML-2.5.1")

if(NOT TARGET SFML::All)
    #定义一个导入库:动态库、全局可用
    add_library(SFML::All IMPORTED SHARED GLOBAL)

    #随便选取一个.lib及配套的.dll
    #不带DEBUG标识的会被应用到各种配置上
    set_target_properties(SFML::All PROPERTIES
        #头文件路径
        INTERFACE_INCLUDE_DIRECTORIES  "${SFMLPath}/include"
        #库文件路径
        IMPORTED_IMPLIB  ${SFMLPath}/lib/sfml-system.lib
        #动态库文件路径
        IMPORTED_LOCATION ${SFMLPath}/bin/sfml-system-2.dll
        #调试版本库文件路径
        IMPORTED_IMPLIB_DEBUG  ${SFMLPath}/lib/sfml-system-d.lib        
        #调试版本动态库文件路径
        IMPORTED_LOCATION_DEBUG  ${SFMLPath}/bin/sfml-system-d-2.dll        
    )

    #提取所有调试库
    #当debug/release被区分文件夹时,
    #可以使用如下方式直接提取所有.lib:
    # file(GLOB_RECURSE SFMLDebugLibs
    #     LIST_DIRECTORIES False 
    #     "${SFMLPath}/lib/debug/*.lib"
    # )
    set(SFMLDebugLibs  
        ${SFMLPath}/lib/sfml-audio-d.lib 
        ${SFMLPath}/lib/sfml-graphics-d.lib 
        ${SFMLPath}/lib/sfml-main-d.lib 
        ${SFMLPath}/lib/sfml-network-d.lib   
        ${SFMLPath}/lib/sfml-window-d.lib                                
    )

    #设置调试配置时的库依赖
    target_link_libraries(SFML::All
        INTERFACE $<$<CONFIG:Debug>:${SFMLDebugLibs}>
    )

    #提取所有发布库
    set(SFMLReleaseLibs  
        ${SFMLPath}/lib/sfml-audio.lib 
        ${SFMLPath}/lib/sfml-graphics.lib 
        ${SFMLPath}/lib/sfml-main.lib 
        ${SFMLPath}/lib/sfml-network.lib   
        ${SFMLPath}/lib/sfml-window.lib                                
    )

    #设置发布配置时的库依赖
    target_link_libraries(SFML::All
        INTERFACE $<$<NOT:$<CONFIG:Debug>>:${SFMLReleaseLibs}>
    )

    #配置无关库
    set(SFMLLibs  
        ${SFMLPath}/lib/flac.lib 
        ${SFMLPath}/lib/freetype.lib 
        ${SFMLPath}/lib/ogg.lib 
        ${SFMLPath}/lib/openal32.lib   
        ${SFMLPath}/lib/vorbis.lib
        ${SFMLPath}/lib/vorbisenc.lib 
        ${SFMLPath}/lib/vorbisfile.lib                                                   
    )
    target_link_libraries(SFML::All
        INTERFACE ${SFMLLibs}
    )

    #定义函数用来拷贝SFML运行时所需动态库等资源
    function(DeploySFMLRuntime)
        set(options)
        set(oneValueArgs TARGET)
        set(multiValueArgs)
        cmake_parse_arguments(Gen "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

        if(Gen_TARGET)
            add_custom_command(TARGET ${Gen_TARGET} POST_BUILD
                COMMAND robocopy /e /mt /XO 
                    "$<TARGET_FILE_DIR:SFML::All>"
                    "$<TARGET_FILE_DIR:${Gen_TARGET}>"
                    || (if %errorlevel% LEQ 7 set errorlevel=0)
                COMMENT "部署SFML运行时"
                COMMAND_EXPAND_LISTS
            )
        endif()
    endfunction()
endif()

##示例程序

add_executable(example)

target_sources(example
    PRIVATE example.cpp
)

target_link_libraries(example
    PRIVATE SFML::All
)

#拷贝动态库到输出目录
DeploySFMLRuntime(TARGET example)
