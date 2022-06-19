add_executable(json)

target_sources(json
    PRIVATE json.cpp exJson.cpp
)

target_link_libraries(json
    PRIVATE archive 
)

target_include_directories(json
    PRIVATE  ${CMAKE_CURRENT_SOURCE_DIR}
)
