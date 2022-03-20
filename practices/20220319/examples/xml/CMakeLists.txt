add_executable(xml)

target_sources(xml
    PRIVATE xml.cpp exXml.cpp
    pugiconfig.hpp pugixml.cpp pugixml.hpp
)

target_link_libraries(xml
    PRIVATE archive 
)

target_include_directories(xml
    PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}
)
