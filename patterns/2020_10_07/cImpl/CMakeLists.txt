cmake_minimum_required(VERSION 3.15)

project(cImpl)

#添加nng库https://github.com/nanomsg/nng
include(FetchContent)

FetchContent_Declare(
    nng
    URL   "https://github.com/nanomsg/nng/archive/v1.3.2.tar.gz"
    URL_HASH SHA512=8288c47930cc9cc84821a945be8ff81589eafb341801b8a273fdbde516233cb5455b0ed962574f4b3306bddc19b70723ed02707218df4820cb39850277a340f1
)

FetchContent_MakeAvailable(nng)

add_executable(pipelineImpl)
target_sources(pipelineImpl
    PRIVATE pipeline.cpp 
)
target_link_libraries(pipelineImpl
    PRIVATE nng
)

add_executable(reqrepImpl)
target_sources(reqrepImpl
    PRIVATE reqrep.cpp 
)
target_link_libraries(reqrepImpl
    PRIVATE nng
)

add_executable(pairImpl)
target_sources(pairImpl
    PRIVATE pair.cpp 
)
target_link_libraries(pairImpl
    PRIVATE nng
)

add_executable(pubsubImpl)
target_sources(pubsubImpl
    PRIVATE pubsub.cpp 
)
target_link_libraries(pubsubImpl
    PRIVATE nng
)

add_executable(surveyImpl)
target_sources(surveyImpl
    PRIVATE survey.cpp 
)
target_link_libraries(surveyImpl
    PRIVATE nng
)

add_executable(busImpl)
target_sources(busImpl
    PRIVATE bus.cpp 
)
target_link_libraries(busImpl
    PRIVATE nng
)