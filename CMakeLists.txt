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
    week2/scanner.cpp
    week2/parser.cpp
    week2/file-reader.cpp
)

add_executable(sluice ${CORE_SOURCES} week2/main.cpp)
target_include_directories(sluice PRIVATE week2)

add_executable(tests ${CORE_SOURCES} tests/correctness/tests.cpp)
target_include_directories(tests PRIVATE week2)

add_executable(scanner_correctness_tests week2/scanner.cpp tests/correctness/scanner_tests.cpp)
target_include_directories(scanner_correctness_tests PRIVATE week2)

add_executable(w1tests ${CORE_SOURCES} week1/main.cpp)
target_include_directories(w1tests PRIVATE week1)
