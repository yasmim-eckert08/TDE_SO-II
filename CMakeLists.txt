cmake_minimum_required(VERSION 4.0)
project(TDE_SO_II)

set(CMAKE_CXX_STANDARD 17)

add_executable(TDE_SO_II
    src/main.cpp
    src/utils.cpp
    src/io.cpp
    src/directory.cpp
    src/operations.cpp
    src/contiguous.cpp
    src/linked.cpp
    src/indexed.cpp
)

target_include_directories(TDE_SO_II PRIVATE include)
