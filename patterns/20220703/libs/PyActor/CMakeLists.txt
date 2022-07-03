pybind11_add_module(PyActor)

target_sources(PyActor
    PRIVATE PyActor.cpp 
)

##设置所在文件夹
set_target_properties(PyActor PROPERTIES
    FOLDER "Python-libs"
)

target_link_libraries(PyActor
    PRIVATE Actor
)
