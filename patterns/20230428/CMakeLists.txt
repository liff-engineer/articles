cmake_minimum_required(VERSION 3.16)

project(webview2-starter)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
include(CMakePrintHelpers)

# 从vcpkg中提取的webview2配置
# 可参见 https://github.com/microsoft/vcpkg/tree/master/ports/webview2
set(webview2_VERSION 1.0.1210.39)
FetchContent_Declare(
    webview2
    URL "https://www.nuget.org/api/v2/package/Microsoft.Web.WebView2/${webview2_VERSION}"
    URL_HASH SHA512=c87812247a466f4aaeac8be8b32be49166b4ddb5bbf65581d5df81d69eda340e9911144a8dd8401baa6e51f95337001cd18ff90cd36ca0af86cc65227e8e2ccf
    DOWNLOAD_NAME "microsoft.web.webview2.${webview2_VERSION}.zip"
    DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/downloads/webview2
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

set(WIL_VERSION 1.0.230411.1)
FetchContent_Declare(
    WIL
    URL "https://www.nuget.org/api/v2/package/Microsoft.Windows.ImplementationLibrary/${WIL_VERSION}"
    URL_HASH SHA512=8b03465c17b757b99d8ae105206973385f27c1fe99daecff83088d52abd16a001676423eff5eb389ce4b0a4fc065c9aca5682be46e3669ed1aa5f59749e73c3c
    DOWNLOAD_NAME "Microsoft.Windows.ImplementationLibrary.${WIL_VERSION}.zip"
    DOWNLOAD_DIR ${CMAKE_SOURCE_DIR}/downloads/webview2
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_GetProperties(webview2)

if(NOT TARGET webview2 AND NOT webview2_POPULATED)
    FetchContent_Populate(webview2)
    FetchContent_Populate(WIL)

    set(webview2_path "${webview2_SOURCE_DIR}/build/native")
    set(WIL_path "${wil_SOURCE_DIR}")

    add_library(webview2 IMPORTED SHARED GLOBAL)
    set_target_properties(webview2 PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${webview2_path}/include
        IMPORTED_LOCATION ${webview2_path}/x64/WebView2Loader.dll
        IMPORTED_IMPLIB ${webview2_path}/x64/WebView2Loader.dll.lib
    )

    add_library(webview2::static IMPORTED STATIC GLOBAL)
    set_target_properties(webview2::static PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${webview2_path}/include
        IMPORTED_LOCATION ${webview2_path}/x64/WebView2LoaderStatic.lib
    )

    target_include_directories(webview2 INTERFACE
        "${WIL_path}/include"
    )

    target_include_directories(webview2::static INTERFACE
        "${WIL_path}/include"
    )
endif()

add_executable(example)
target_sources(example PRIVATE example.cpp)

target_link_libraries(example PRIVATE
    webview2::static
)

if(MSVC)
    target_compile_definitions(example
        PRIVATE UNICODE NOMINMAX
    )
endif()

set_target_properties(example PROPERTIES
    WIN32_EXECUTABLE TRUE
)
