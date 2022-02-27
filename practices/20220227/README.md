# 如何使用`CMake`快速配置二进制库依赖

在[基于`CMake`管理库依赖的几种方式]()中曾经介绍过如何为二进制库提供`CMake`支持.在真实场景中,开发者总是希望有比较**糙快猛**的方法来解决问题:

- 无论用不用的上,头文件路径全加;
- 库依赖全配置上,省的报错;
- 动态库全拷贝过去,避免一个个找.

这种方式有其应用场景和价值,因而这里就介绍一种更为简单快速的方法,迅速搞定二进制库依赖配置.

 ## 设计目标

针对二进制库包,例如`SFML`,直接定义一个`target`-**SFML::All**,使得开发者可以仅依赖一个库从而完成所有配置:

```cmake
#example依赖SFML包的所有头文件路径、库依赖
target_link_libraries(example
    PRIVATE SFML::All
)
```

然后提供`CMake`函数可以直接拷贝所有可能依赖的动态库到输出目录:

```cmake
#拷贝动态库及资源到输出目录
DeploySFMLRuntime(TARGET example)
```

开发者可以将`SFML`二进制包解压到任意目录,通过以下方式使用:

- 环境变量
- 绝对路径
- 相对路径

## 实现方式

二进制包需要一个`CMakeLists.txt`来配置,开发者可以写到根目录的`CMakeLists.txt`中,也可以为其专门提供文件夹存放,例如:

```bash
CMakeLists.txt
external
	CMakeLists.txt
	SFML
		CMakeLists.txt
```

将所有第三方库的配置放在`external`目录下,`external/CMakeLists.txt`内容类似如下:

```cmake
add_subdirectory(SFML)
```

然后在根目录的`CMakeLists.txt`中引入`external`文件夹:

```cmake
add_subdirectory(external)
```

然后开始向`external/SFML/CMakeLists.txt`填充内容.

### 找到二进制包路径

二进制包可以解压到任意路径,开发者自行设计,这里根据不同的情况,将其读取到`SFMLPath`变量中,例如环境变量的方式:

```cmake
#从环境变量读取SFML包解压路径
set(SFMLPath  $ENV{SFMLSdkPath})
```

或者相对/绝对路径的方式:

```cmake
#从相对/绝对路径读取SFML包解压路径
set(SFMLPath  "${CMAKE_SOURCE_DIR}/downloads/SFML-2.5.1")
```

### 创建`target`

这里要提供类似于`include guard`的方式避免重复导入:

```cmake
if(NOT TARGET SFML::All)
    #定义一个导入库:动态库、全局可用
    add_library(SFML::All IMPORTED SHARED GLOBAL)
endif()
```

### 选择任意一个`lib`来完成初始配置

由于要在`SFML::All`上完成所有配置,使用哪个具体的`.lib`并不重要,这里选择一个配置上:

```cmake
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
```

注意调试版本的单独设置,这样只有切换到`Debug`配置时才使用它,其它情况只会使用非调试版本配置.

### 根据配置设置不同的库依赖

`SFML`提供的二进制包把`Debug`、`Release`以及静态库全部混在一起了,导致需要根据配置手工列出所有库依赖:

```cmake
#提取所有调试库
set(SFMLDebugLibs  
    ${SFMLPath}/lib/sfml-audio-d.lib 
    ${SFMLPath}/lib/sfml-graphics-d.lib 
    ${SFMLPath}/lib/sfml-main-d.lib 
    ${SFMLPath}/lib/sfml-network-d.lib   
    ${SFMLPath}/lib/sfml-window-d.lib                                
)
#提取所有发布库
set(SFMLReleaseLibs  
    ${SFMLPath}/lib/sfml-audio.lib 
    ${SFMLPath}/lib/sfml-graphics.lib 
    ${SFMLPath}/lib/sfml-main.lib 
    ${SFMLPath}/lib/sfml-network.lib   
    ${SFMLPath}/lib/sfml-window.lib                                
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
```

如果对应的二进制包根据`Debug`、`Release`区分了不同的文件夹,则不需要手工写,使用`file(GLOB)`根据后缀名提取全部文件即可:

```cmake
#当debug/release被区分文件夹时,
#可以使用如下方式直接提取所有.lib:
file(GLOB_RECURSE SFMLDebugLibs
     LIST_DIRECTORIES False 
     "${SFMLPath}/lib/debug/*.lib"
)

file(GLOB_RECURSE SFMLReleaseLibs
     LIST_DIRECTORIES False 
     "${SFMLPath}/lib/release/*.lib"
)     
file(GLOB_RECURSE SFMLLibs
     LIST_DIRECTORIES False 
     "${SFMLPath}/lib/common/*.lib"
)                
```

然后根据不同的配置设置库依赖:

```cmake
#设置调试配置时的库依赖
target_link_libraries(SFML::All
	INTERFACE $<$<CONFIG:Debug>:${SFMLDebugLibs}>
)
#设置发布配置时的库依赖
target_link_libraries(SFML::All
	INTERFACE $<$<NOT:$<CONFIG:Debug>>:${SFMLReleaseLibs}>
)

target_link_libraries(SFML::All
	INTERFACE ${SFMLLibs}
)
```

注意库依赖是`INTERFACE`,代表着任何依赖`SFML::All`的`target`会被自动配置上该库依赖,这种特性被称为依赖传播,是非常有用、能够极大简化配置的`CMake`特性,譬如使用`Qt`的`Widgets`时,不需要配置`Core`、`Gui`相关依赖,因为`Widget`对它们的依赖是`INTERFACE`,类似于:

```C++
target_link_libraries(Qt5::Widgets
	INTERFACE Qt5::Core Qt5::Gui
)
```

### 提供部署运行时的函数

针对`SFML`,开发者希望将动态库所在的目录`bin`下面的内容全部拷贝到输出目录,可以采用类似方式:

```cmake
include(CMakeParseArguments) #书写CMake函数需要,放在CMakeLists.txt开头位置

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
```

这里使用`Windows`自带的`robocopy`将`SFML::All`中`IMPORTED_LOCATION`所属目录下内容全部拷贝到`TARGET`输出目录.

注意提供`|| (if %errorlevel% LEQ 7 set errorlevel=0)`是因为`robocopy`返回的不完全是错误码,小于`7`的返回值都可以认为指令运行正常.如果不设置,构建时会被视为构建错误,向`TARGET`添加的其它指令均不会再执行.

## `SFMl`完整的`CMakeLists.txt`

```cmake
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
```



