add_library(archive)

target_sources(archive
    PRIVATE archive.h archive.cpp
)

target_include_directories(archive
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)
