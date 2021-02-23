# gRPC库C++构建及示例

> gRPC is a modern, open source, high-performance remote procedure call (RPC) framework that can run anywhere. gRPC enables client and server applications to communicate transparently, and simplifies the building of connected systems.

`C++`开发者做什么事情都不太容易,网络编程原本已经很艰难了,想要使用`gRPC`来降低难度,库构建又是一道坎儿.这里展示如何在残酷的网络环境下使用`CMake`构建`gRPC`库,并附带示例验证库构建结果.

## 如何下载`gRPC`库源代码?

[`gRPC`](https://github.com/grpc/grpc)自身内容很庞大,依赖也比较复杂,如果使用`Github`下载,耗时费力不说,还容易中断.所幸[Gitee](https://gitee.com/)上提供了部分镜像仓库,可以将`gRPC`库自身内容快速拉取下来:

```bash
git clone -b v1.34.0 https://gitee.com/mirrors/grpc-framework.git grpc
```

注意,目前在`Gitee`上只能找到`gRPC`依赖的部分"官方"镜像仓库,网友提供的镜像仓库较旧,因而只能构造`v1.34.0`版本.通过上述指令可以将`v1.34.0`版本的`gRPC`代码下载到`grpc`目录.

`gRPC`的依赖是通过`git`的`submodules`来关联的,代码下载下来之后可以看到`.gitmodules`文件,内部的`git`仓库地址都需要替换成`Gitee`的,例如:

```bash
[submodule "third_party/zlib"]
	path = third_party/zlib
	url = https://github.com/madler/zlib
	# When using CMake to build, the zlib submodule ends up with a
	# generated file that makes Git consider the submodule dirty. This
	# state can be ignored for day-to-day development on gRPC.
	ignore = dirty
```

使用了`zlib`,在`Gitee`上搜索其代码仓库为`https://gitee.com/mirrors/zlib`,可以使用如下指令`clone`:

```bash
git clone https://gitee.com/mirrors/zlib.git
```

因而替换成:

```bash
[submodule "third_party/zlib"]
	path = third_party/zlib
	#url = https://github.com/madler/zlib
	url = https://gitee.com/mirrors/zlib.git
	# When using CMake to build, the zlib submodule ends up with a
	# generated file that makes Git consider the submodule dirty. This
	# state can be ignored for day-to-day development on gRPC.
	ignore = dirty
```

通过这种方法可以找到部分依赖库的最新镜像仓库,但是有一些找不到最新的,例如`protobuf`等库,用户`local-grpc`提供了`gRPC`依赖的全部代码仓库,可以使用这些仓库(注意代码不是同步镜像,导致`gRPC`只能构造相应版本),其中`protobuf`链接为:

```bash
https://gitee.com/local-grpc/protobuf.git
```

这里将`.gitmodules`修改为如下内容即可:

```bash
[submodule "third_party/zlib"]
	path = third_party/zlib
	#url = https://github.com/madler/zlib
	url = https://gitee.com/mirrors/zlib.git
	# When using CMake to build, the zlib submodule ends up with a
	# generated file that makes Git consider the submodule dirty. This
	# state can be ignored for day-to-day development on gRPC.
	ignore = dirty
[submodule "third_party/protobuf"]
	path = third_party/protobuf
	#url = https://github.com/google/protobuf.git
	url = https://gitee.com/local-grpc/protobuf.git
[submodule "third_party/googletest"]
	path = third_party/googletest
	#url = https://github.com/google/googletest.git
	url = https://gitee.com/local-grpc/googletest.git
[submodule "third_party/benchmark"]
	path = third_party/benchmark
	#url = https://github.com/google/benchmark
	url = https://gitee.com/mirrors/google-benchmark.git
[submodule "third_party/boringssl-with-bazel"]
	path = third_party/boringssl-with-bazel
	#url = https://github.com/google/boringssl.git
	url = https://gitee.com/mirrors/boringssl.git
[submodule "third_party/re2"]
	path = third_party/re2
	#url = https://github.com/google/re2.git
	url = https://gitee.com/local-grpc/re2.git
[submodule "third_party/cares/cares"]
	path = third_party/cares/cares
	#url = https://github.com/c-ares/c-ares.git
	url = https://gitee.com/mirrors/c-ares.git
	branch = cares-1_12_0
[submodule "third_party/bloaty"]
	path = third_party/bloaty
	#url = https://github.com/google/bloaty.git
	url = https://gitee.com/local-grpc/bloaty.git
[submodule "third_party/abseil-cpp"]
	path = third_party/abseil-cpp
	#url = https://github.com/abseil/abseil-cpp.git
	url = https://gitee.com/mirrors/abseil-cpp.git
	branch = lts_2020_02_25
[submodule "third_party/envoy-api"]
	path = third_party/envoy-api
	#url = https://github.com/envoyproxy/data-plane-api.git
	url = https://gitee.com/local-grpc/data-plane-api.git
[submodule "third_party/googleapis"]
	path = third_party/googleapis
	#url = https://github.com/googleapis/googleapis.git
	url = https://gitee.com/mirrors/googleapis.git
[submodule "third_party/protoc-gen-validate"]
	path = third_party/protoc-gen-validate
	#url = https://github.com/envoyproxy/protoc-gen-validate.git
	url = https://gitee.com/local-grpc/protoc-gen-validate.git
[submodule "third_party/udpa"]
	path = third_party/udpa
	#url = https://github.com/cncf/udpa.git
	url = https://gitee.com/local-grpc/udpa.git
[submodule "third_party/libuv"]
	path = third_party/libuv
	#url = https://github.com/libuv/libuv.git
	url = https://gitee.com/mirrors/libuv.git

```

使用如下指令拉取`gRPC`所有依赖:

```bash
cd grpc
git submodule update --init
```

如果你希望使用`CMake`的`FetchContent`模块将`gRPC`整合到自身工程中,可以将上述步骤下载完成的完整源代码打包成压缩包,存放在自己的`ftp`等服务器上,然后使用如下`Python`脚本计算出`SHA512`:

```python
import sys
import hashlib

# BUF_SIZE is totally arbitrary, change for your app!
BUF_SIZE = 65536  # lets read stuff in 64kb chunks!

sha512 = hashlib.sha512()
with open(sys.argv[1], 'rb') as f:
    while True:
        data = f.read(BUF_SIZE)
        if not data:
            break
        sha512.update(data)
print("SHA512: {0}".format(sha512.hexdigest()))
```

以压缩包完整/相对路径为参数,执行上述脚本,复制得到的`SHA512`内容,然后在你工程的`CMakeLists.txt`中以如下方式使用:

```cmake
include(FetchContent)

FetchContent_Declare(
   gRPC
   URL  "gRPC源码压缩包服务器路径"
   URL_HASH SHA512= "gRPC源码压缩包的SHA512"
   ##DOWNLOAD_DIR可以根据需要修改
   DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/external/downloads/spdlog
)
set(FETCHCONTENT_QUIET OFF)
FetchContent_MakeAvailable(gRPC)

##工程中库使用方式
target_link_libraries(YourTarget grpc++)
```

注意在之前要安装必备的依赖,例如 [nasm](https://www.nasm.us/).



不过上述使用方式构建时速度特别慢,以下展示如何直接构建出`gRPC`库,并安装到指定路径.

## 如何构建`gRPC`库

首先需要安装必要的依赖,例如在`Windows`上需要以下内容:

- `Visual Studio 2015`或`Visual Studio 2017`
- `Git`
- `CMake`
- `nasm`,并且配置到`PATH`环境变量中
- 可选的`Ninja`

`CMake`的使用方式大同小异,这里以`Windows`为例展示,首先要进行配置,假设已经处于源代码路径中:

```bash
cmake -S . -B .build -G"Visual Studio 15 2017" -T v141 -A x64 -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_BUILD_CSHARP_EXT=OFF -DCMAKE_INSTALL_PREFIX="安装路径"
```

上述配置使用的是`Visual Studio 2017`及对应工具集,64位构建,使能安装动作,禁止构造测试用例和`C#`扩展,并指定了安装路径.如果使用的是`Visual Studio 2015`则替换成`Visual Studio 14 2015`.

然后通过如下指令构建并安装:

```bash
cmake --build .build --target install --config Release
```

经过一段时间的等待,在安装路径就可以看到构建出的结果了.

这里需要说明的是,`gRPC`无法将`Debug`和`Release`等多个配置安装到同一位置.开发者只能选择构建某一配置,然后在使用时工程构建也只能使用这一配置,通常可以选择`Release`构造,如果面临调试需求,可以选择`RelWithDebInfo`,即上述指令修改位:

```bash
cmake --build .build --target install --config RelWithDebInfo
```

上述**.build**路径为官方示例建议的路径,可以自行修改,但无必要.



经过上述操作,在安装路径下就有了可以使用的`gRPC`库了.下面来看一下如何使用它.

## 如何运行`Hello World`

在`gRPC`源代码的`\examples\cpp\helloworld`路径下有如下代码文件:

```bash
greeter_server.cc
greeter_client.cc
greeter_async_server.cc
greeter_async_client.cc
greeter_async_client2.cc
```

在`\examples\protos`下有对应的`helloworld.proto`文件.

将上述文件拷贝到示例目录,例如`helloworld`目录下,并添加`CMakeList.txt`工程配置,最终目录结构如下:

```bash
greeter_server.cc
greeter_client.cc
greeter_async_server.cc
greeter_async_client.cc
greeter_async_client2.cc
helloworld.proto
CMakeLists.txt
```

将`CMakeLists.txt`修改为类似如下内容:

```cmake
cmake_minimum_required(VERSION 3.15)
#工程名,可自行修改
project(grpc-examples CXX)

#以下三个find_package需要添加,否则找不到对应的target会报错
find_package(Threads REQUIRED)#注意不要加CONFIG
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

##添加共享的静态库,包含helloworld.proto中定义的RPC协议代码
add_library(helloworld)
target_sources(helloworld
	PRIVATE "helloworld.proto"
)

##生成helloworld.proto对应的C++代码
protobuf_generate(TARGET helloworld LANGUAGE cpp)

##获取proto的grpc插件
get_target_property(grpc_cpp_plugin_location gRPC::grpc_cpp_plugin LOCATION)

##生成helloworld.proto对应的gRPC-C++代码,以此来支持gRPC协议
protobuf_generate(TARGET helloworld LANGUAGE grpc GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc PLUGIN "protoc-gen-grpc=${grpc_cpp_plugin_location}")

target_link_libraries(helloworld
    PRIVATE gRPC::grpc++
)

##上述protobuf_generate将自动生成的代码存放于该位置,需要添加到include路径
target_include_directories(helloworld
    PUBLIC  ${CMAKE_CURRENT_BINARY_DIR}
)

##遍历列表创建应用程序
foreach(_target
    greeter_client greeter_server
    greeter_async_client greeter_async_client2 greeter_async_server)
    add_executable(${_target} "${_target}.cc")
    target_link_libraries(${_target}
        PRIVATE helloworld 
                gRPC::grpc++ gRPC::grpc++_reflection
    )
endforeach()
```

这里需要强调,官方文档在`Windows`下构建存在问题,必须添加`gRPC::grpc++_reflection`依赖,否则构建示例会报如下错误.

```bash
无法解析的外部符号 "void __cdecl grpc::reflection::InitProtoReflectionServerBuilderPlugin(void)
```

`CMake`的`protobuf_generate`模块可以用来辅助代码生成动作,开发者只需要将`.proto`文件作为源代码添加到`target`中,然后`protobuf_generate`会根据配置自动生成对应代码,在`.proto`文件发生变化时能够自动刷新.详细信息参见[gRPC and Plugin support in CMake](https://www.falkoaxmann.de/dev/2020/11/08/grpc-plugin-cmake-support.html).

在进行`CMake`配置时需要添加`-DCMAKE_PREFIX_PATH=gRPC安装路径`,这样`find_package`才能找到`gRPC`,例如:
```bash
cmake -S . -B build -G"Visual Studio 15 2017" -T v141 -A x64 -DCMAKE_PREFIX_PATH="gRPC安装路径"  
```

在输出目录找到生成的应用程序,例如`greeter_server.exe`:

```bash
./greeter_server.exe
```

然后启动客户端:

```bash
./greeter_client.exe
```

客户端输出如下内容并退出:

```bash
Greeter received: Hello world
```

**现在就可以查阅官方教程来学习`gRPC`了**.