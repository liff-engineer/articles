cmake_minimum_required(VERSION 3.15)

project(json_example)

add_subdirectory(json)

add_executable(jsonUsecase)

target_sources(jsonUsecase
    PRIVATE jsonUsecase.cpp
)

target_link_libraries(jsonUsecase
    PRIVATE nlohmann_json::nlohmann_json
)
