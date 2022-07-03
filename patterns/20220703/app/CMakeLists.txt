add_executable(App)

target_sources(App
    PRIVATE App.cpp
)

target_link_libraries(App
    PRIVATE Actor
)
