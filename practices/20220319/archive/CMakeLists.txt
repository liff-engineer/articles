add_library(archive SHARED)

target_sources(archive
    PRIVATE archive.h archive.cpp  meta.h
)

target_include_directories(archive
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

# target_precompile_headers(archive
#     PUBLIC "archive.h"
# )
