include(FetchContent)

FetchContent_Declare(
    pybind11
    URL   "https://github.com/pybind/pybind11/archive/refs/tags/v2.9.2.tar.gz"
    URL_HASH SHA512=c6c18e5f59873adb3692640ade26472abd257607e7bb9fd48cfd1949878811e83d6ac6eb8c8dd926622d52ca4f13e5e6a58e0abaaaa1fa814ee831ea2b515272
    DOWNLOAD_DIR  ${CMAKE_SOURCE_DIR}/downloads/pybind11
)

FetchContent_MakeAvailable(pybind11)
