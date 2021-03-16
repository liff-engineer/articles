# 基于`sol2`整合`Lua`到`C++`应用(1)

为`C++`应用提供脚本化能力,可以解决很多问题,譬如:

- 支持用户输入表达式
- 支持用户扩展应用程序
- 热更新
- ......

针对某些简单场景,有能力的开发者可能会自行开发.但是与其吭哧吭哧造轮子,选一门完备的编程语言会省心省力很多.

这里基于`sol2`来展示如何为`C++`应用整合`Lua`脚本运行能力.

## 示例源码及运行效果

这里从`sol2`官方文档选取一个示例`example.cpp`,内容如下:

```C++
#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>
#include <iostream>


int main() {
    std::cout << "=== opening a state ===\n";
    
    //lua运行环境
    sol::state lua;
    //打开标准库
    lua.open_libraries(sol::lib::base, sol::lib::package);
    //执行lua脚本
    lua.script("print('bark bark bark!')");

    std::cout << "\n";
    return 0;
}
```

运行效果如下:

```bash
=== opening a state ===
bark bark bark!

```


## `lua`库配置

`CMake`提供了`find_package(Lua)`的支持,前提是`Lua`已经被安装.在`Windows`下,可以利用`FetchContent`模块直接下载构建好的`Lua`库.

假设`C++`应用的第三方库管理脚本都存储在`external`目录下,那么在目录下创建`lua/CMakeLists.txt`文件,内容如下:

```cmake
include(FetchContent)
include(CMakePrintHelpers)

## 64位
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	#从服务器下载构建好的lua动态库
	#注意URL_HASH,刚开始可以随意填写,CMake下载过程会告知正确的HASH是多少
    FetchContent_Declare(
        lua
        URL  "https://sourceforge.net/projects/luabinaries/files/5.4.2/Windows%20Libraries/Dynamic/lua-5.4.2_Win64_dll14_lib.zip/download"
        URL_HASH SHA512=b653d7d33680a3688b438538edfe2611e3a2f3529449ec159060d32cdeb2642a6f89b08b46ea432f62d24b0f2cfec0ba54cd7a4b819a30009768261e230764be
        DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/downloads/lua
    )
else()
	#32位:注意这里的URL_HASH不是正确的,使用时修改为正确的
    FetchContent_Declare(
        lua
        URL  "https://sourceforge.net/projects/luabinaries/files/5.4.2/Windows%20Libraries/Dynamic/lua-5.4.2_Win32_dll14_lib.zip/download"
        URL_HASH SHA512=b653d7d33680a3688b438538edfe2611e3a2f3529449ec159060d32cdeb2642a6f89b08b46ea432f62d24b0f2cfec0ba54cd7a4b819a30009768261e230764be
        DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/downloads/lua
    )
endif()

FetchContent_GetProperties(lua)
if(NOT lua_POPULATED)
	#下载并解压
    FetchContent_Populate(lua)
    #cmake_print_variables(lua_SOURCE_DIR)
    #将目录保存到
    list(APPEND CMAKE_PREFIX_PATH "${lua_SOURCE_DIR}") 
endif()

#这里find一下,然后把动态库路径保存到LUA_BINARIES中供后续拷贝
find_package(Lua REQUIRED)
#cmake_print_variables(LUA_INCLUDE_DIR)
set(LUA_BINARIES "${LUA_INCLUDE_DIR}/../lua${LUA_VERSION_MAJOR}${LUA_VERSION_MINOR}.dll" CACHE STRING "lua动态库")
```

由于`MSVC`的`v140`、`v141`、`v142`三个工具集是二进制兼容的,这里为了简单期间,并没有区分工具集,如何判断工具集并下载对应的`SDK`请自行谷歌.

在`external`中添加`CMakeLists.txt`:

```cmake
add_subdirectory(lua)
```

在工程根目录的`CMakeLists.txt`中添加`external`目录,然后就可以使用`find_package(Lua REQUIRED)`来获取`Lua`库,进行进一步的配置.

## `sol2`库配置

`sol2`库自身提供了`CMake`支持,可以非常简单地配置并集成到工程中.在`external`中添加`sol2/CMakeLists.txt`,内容如下:

```cmake
include(FetchContent)

FetchContent_Declare(
    sol2
    URL "https://github.com/ThePhD/sol2/archive/v3.2.2.tar.gz"
    URL_HASH SHA512=e5a739b37aea7150f141f6a003c2689dd33155feed5bb3cf2569abbfe9f0062eacdaaf346be523d627f0e491b35e68822c80e1117fa09ece8c9d8d5af09fdbec
    DOWNLOAD_DIR  ${CMAKE_SOURCE_DIR}/downloads/sol2
)

FetchContent_MakeAvailable(sol2)
```

修改`external/CMakeLists.txt`,添加`sol2`子目录:

```cmake
add_subdirectory(lua)
add_subdirectory(sol2)
```

## 示例工程配置

`sol2`目前的分支要求`C++17`或以上标准支持,注意需要开启;另外虽然可以使用`find_package(Lua REQUIRED)`查找库配置,但是却无法使用`target_xxx`来使用`Lua`库,其中头文件存储在变量`LUA_INCLUDE_DIR`中,库文件存储在变量`LUA_LIBRARIES`中. 示例工程配置如下:

```cmake
cmake_minimum_required(VERSION 3.15)

project(sol2User)

##设置要求编译器支持C++17标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#生成Visual Studio工程时使用文件夹组织结构
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

##添加lua和sol2库
add_subdirectory(external)

add_executable(example)
target_sources(example
    PRIVATE example.cpp
)


find_package(Lua REQUIRED)

##指定lua头文件路径
target_include_directories(example 
    PRIVATE ${LUA_INCLUDE_DIR}
)
## 配置sol2库和lua库依赖
target_link_libraries(example
    PRIVATE sol2::sol2 ${LUA_LIBRARIES}
)

##拷贝lua动态库到输出目录
add_custom_command(TARGET example POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${LUA_BINARIES}" "$<TARGET_FILE_DIR:example>"
)
```

## 未完待续

这一部分简单演示了如何整合`Lua`到`C++`应用,还有很多具体的`sol2`使用方法,譬如如何注册`C++`的类、方法、变量到`Lua`运行环境等等,留待后续讲解.





