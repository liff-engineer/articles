cmake_minimum_required(VERSION 3.15)

project(cppImpl)

#添加nng库https://github.com/nanomsg/nng
include(FetchContent)

FetchContent_Declare(
    nng
    URL   "https://github.com/nanomsg/nng/archive/v1.3.2.tar.gz"
    URL_HASH SHA512=8288c47930cc9cc84821a945be8ff81589eafb341801b8a273fdbde516233cb5455b0ed962574f4b3306bddc19b70723ed02707218df4820cb39850277a340f1
)

FetchContent_MakeAvailable(nng)

FetchContent_Declare(
    nngpp
    URL   "https://github.com/cwzx/nngpp/archive/nng-v1.3.0.tar.gz"
    URL_HASH SHA512=6f93c852a172977f45b65b396c45d23725b39dc0ff30a0285209b27bb80480a61a291a8f29244fd453228eb89a638e2a8fecfff465a21f28906574766e2f79df
)

####
####nngpp要求nng必须安装完成,这样无法使用FetchContent模块这种方式了
####如果想要这样使用,需要在nngpp下载完成后修改其CMakeLists.txt
####1. 屏蔽find_package(nng)
####2. target_include_directories添加 $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/>
####3. 修改nngpp的库依赖nng::nng 为nng
####这样本地就可正常使用,如果需要保证其它人可以正常使用,请修改CMakeLists.txt
####之后重新打包,存放于某处,然后使用FetchContent下载即可

FetchContent_MakeAvailable(nngpp)

add_executable(pipelineImpl)

target_sources(pipelineImpl
    PRIVATE pipeline.cpp 
)
target_link_libraries(pipelineImpl
    PRIVATE nngpp
)

add_executable(reqrepImpl)
target_sources(reqrepImpl
    PRIVATE reqrep.cpp 
)
target_link_libraries(reqrepImpl
    PRIVATE nngpp
)

add_executable(pairImpl)
target_sources(pairImpl
    PRIVATE pair.cpp 
)
target_link_libraries(pairImpl
    PRIVATE nngpp
)

add_executable(pubsubImpl)
target_sources(pubsubImpl
    PRIVATE pubsub.cpp 
)
target_link_libraries(pubsubImpl
    PRIVATE nngpp
)

add_executable(surveyImpl)
target_sources(surveyImpl
    PRIVATE survey.cpp 
)
target_link_libraries(surveyImpl
    PRIVATE nngpp
)

add_executable(busImpl)
target_sources(busImpl
    PRIVATE bus.cpp 
)
target_link_libraries(busImpl
    PRIVATE nngpp
)