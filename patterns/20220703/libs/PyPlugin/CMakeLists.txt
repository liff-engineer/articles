add_library(PyPlugin SHARED)

target_sources(PyPlugin
    PRIVATE PyPlugin.cpp PyPlugin.h
)

##设置所在文件夹
set_target_properties(PyPlugin PROPERTIES
    FOLDER "libs"
)

target_link_libraries(PyPlugin
    PRIVATE pybind11::embed
)
