## 使用`CMake`部署`Qt`应用程序

`Qt`应用程序在运行时依赖于动态链接库以及资源文件,在调试和生成`Qt`安装包时都需要进行处理,否则经常出现本地运行可以,而在客户电脑上不行的场景.

这时,保持开发者本地环境和真实运行环境一致是比较好的实践.即:

1. 开发者环境不配置`Qt`相关环境变量;

2. 不使用专门的`IDE`及插件(例如`Qt Visual Studio Tools`).

这种情况下`CMake`是比较好的选择,这时就需要解决调试时部署`Qt`运行时的问题:

- 依赖的`Qt`动态链接库
- 依赖的`Qt`插件
- `Qt`翻译文件
- `Qml`等资源

好在`Qt`在`Windows`上提供了`windeployqt`来帮助开发者.那么如何整合到`CMakeLists`中呢?

## 工程构建结果的部署

工程构建结果包含构造出来的`exe`、`dll`、`qm`以及其它资源文件.`CMake`默认会配置`exe`、`dll`等内容的输出目录,而资源文件是需要开发者自行处理的.这里也分两种场景来处理:

### 调整运行时输出位置

在`CMake`中 `exe`、`dll`等被成为运行时(`runtime`),通过指定`CMAKE_RUNTIME_OUTPUT_DIRECTORY`可以调整其输出位置,假设要根据不同的配置(`Debug`、`Release`等)输出到构建目录,则在工程的主`CMakeLists.txt`中通过这种方式定义:

```cmake
if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>")
endif()
```

### 拷贝资源到输出位置

要执行这一操作,首先要获取输出位置,假设你为库或者应用程序定义的`target`名称为`TARGET_NAME`,通过如下指令可以获取输出位置:

```cmake
$<TARGET_FILE_DIR:TARGET_NAME>
```

`qm`文件的路径存储在`TARGET_NAME_QM_FILE`变量中,那么可以以如下命令完成目录创建、翻译文件拷贝动作:

```cmake
## 复制资源文件
add_custom_command(TARGET TARGET_NAME POST_BUILD
	## 创建翻译文件目录
	COMMAND ${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:TARGET_NAME>/translations/
	## 翻译文件
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${TARGET_NAME_QM_FILE} $<TARGET_FILE_DIR:TARGET_NAME>/translations/
)
```

拷贝资源文件也是相同的处理方式,假设在`CMakeLists.txt`同目录下有`data`文件夹存储了运行时资源,拷贝命令为:

```cmake
## 复制资源文件
add_custom_command(TARGET TARGET_NAME POST_BUILD
	## 复制数据
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data/ $<TARGET_FILE_DIR:TARGET_NAME>/
)    
```

这里建议你将库、应用程序的`name`定义为变量,这样碰见相同场景直接复制上述代码即可,例如:

```cmake
## 复制资源文件
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
	## 复制数据
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/data/ $<TARGET_FILE_DIR:${TARGET_NAME}>/
	## 创建翻译文件目录
	COMMAND ${CMAKE_COMMAND} -E make_directory  $<TARGET_FILE_DIR:${TARGET_NAME}>/translations/    
	## 翻译文件
	COMMAND ${CMAKE_COMMAND} -E copy_if_different ${${TARGET_NAME}_QM_FILE} $<TARGET_FILE_DIR:${TARGET_NAME}>/translations/
)
```

另外,`TS`以及`QM`文件均可以自动生成,`CMake`中写法如下:

```cmake
##查找Qt翻译家
find_package(Qt5LinguistTools)
##创建TS文件并提供qm生成
qt5_create_translation(${TARGET_NAME}_QM_FILE
    ${${TARGET_NAME}_HEADER_FILES} #所有头文件
    ${${TARGET_NAME}_SRC_FILES} #所有源文件
    ${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}_zh.ts #生成的ts文件完整路径
)
```

## `Qt`运行时及资源部署

经过上述操作,应用程序自身内容的部署就已经完成.而`Qt`的部署鉴于有`windeployqt`,可以实现得相对简单且通用.

### 基本思路

定义`DeployQtRuntime.cmake`,包含`DeployQtRuntime`函数,能够处理各种`Qt`运行时及资源部署场景.该函数具备如下能力:

- 部署基本的`Qt`运行时
- 可选的`WebEngine`部署
- 可选的`Qml`部署
- 可配置的`Qt`插件部署列表

函数使用方式如下:

```cmake
DeployQtRuntime(
	TARGET YourTargetName
	WebEngine #部署WebEngine
	QmlFilesPath "工程qml文件路径,供windeployqt扫描"
	PLUGINS "xml;svg"
)
```

在工程的主`CMakeLists.txt`中要将`DeployQtRuntime.cmake`所在路径添加到`CMAKE_MODULE_PATH`中,例如:

```cmake
## 配置本地cmake脚本路径
list(APPEND CMAKE_MODULE_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(DeployQtRuntime)
```

### 函数声明

```cmake
include(CMakeParseArguments)

## 部署Qt运行时及插件
function(DeployQtRuntime)
    set(options WebEngine)
    set(oneValueArgs TARGET QmlFilesPath)
    set(multiValueArgs PLUGINS)
    cmake_parse_arguments(Gen "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
endfunction()
```

`CMakeParseArguments`为`CMake`提供的参数解析辅助函数,上述声明会将参数解析成为`Gen_`开头的变量供后续使用.

### 查找`windeployqt`

```cmake
find_package(Qt5 COMPONENTS Core CONFIG)

#加载Qt5::Core对应的库配置文件,能够找到Qt的bin目录
if(NOT _qt5_install_prefix)
	return()
endif()

## message(STATUS "Qt cmake模块位于:${_qt5_install_prefix}")
## _qt5_install_prefix基本上在lib/cmake位置,需要定位到bin路径下面找到部署程序
find_program(__qt5_deploy windeployqt PATHS "${_qt5_install_prefix}/../../bin")

## 获取QTDIR
set(QTDIR "${_qt5_install_prefix}/../../")
```

### 插件参数格式处理

`Gen_PLUGINS`要拆分成一个个插件,然后加上`-`前缀,并且在`WINDOWS`下还要调整以下参数样式:

```cmake
set(qt_plugins "")
foreach(__plugin ${Gen_PLUGINS})
	string(APPEND qt_plugins " -${__plugin}") 
endforeach()
separate_arguments(qt_plugins_list WINDOWS_COMMAND ${qt_plugins})
```

### 调用`windeployqt`

`qml`默认扫描路径取自`target`输出路径:

```cmake
set(QmlFilesPath "$<TARGET_FILE_DIR:${Gen_TARGET}>")
if(Gen_QmlFilesPath)
    #如果用户设置了qml文件路径则直接使用
	set(QmlFilesPath "${Gen_QmlFilesPath}")
endif()

add_custom_command(TARGET ${Gen_TARGET} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E env QTDIR="${QTDIR}" "PATH=${QTDIR}/bin" windeployqt.exe --qmldir ${QmlFilesPath} $<TARGET_FILE:${Gen_TARGET}> ${qt_plugins_list}
COMMENT "deploy Qt runtime dependencies"  
)
```

注意上述命令在执行过程中,调整了环境变量`QTDIR`,这是因为用户的环境下可能配置了`QTDIR`,会导致执行了环境变量配置中的`windeployqt.exe`,从而导致错误.

### 处理`WebEngine`运行时

由于`WebEngine`运行需要一些特殊的资源文件以及`QtWebEngineProcess`,需要专门处理:

```cmake
## Qt如果部署包含Qt WebEngine的应用,需要处理无法自动部署的资源
if(Gen_WebEngine)
    add_custom_command(TARGET ${Gen_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory 
        	"${_qt5_install_prefix}/../../resources/"  ##拷贝 resources 目录下内容
        	"$<TARGET_FILE_DIR:${Gen_TARGET}>"
        COMMENT "deploy Qt WebEngine resources"

        COMMAND ${CMAKE_COMMAND} -E copy        	"${_qt5_install_prefix}/../../bin/$<IF:$<CONFIG:DEBUG>,QtWebEngineProcessd.exe,QtWebEngineProcess.exe>" 
            		"$<TARGET_FILE_DIR:${Gen_TARGET}>/$<IF:$<CONFIG:DEBUG>,QtWebEngineProcessd.exe,QtWebEngineProcess.exe>"            
        COMMENT "deploy Qt WebEngine process"
    )
endif()
```











