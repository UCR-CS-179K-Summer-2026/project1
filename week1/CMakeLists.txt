cmake_minimum_required(VERSION 3.16)
project(json_jsonl_parser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

set(CORE_SOURCES
    scanner.cpp
    parser.cpp
    file-reader.cpp
    file-reader.h
)


add_executable(tests ${CORE_SOURCES} ../tests/tests.cpp)
target_include_directories(tests PRIVATE week1)

