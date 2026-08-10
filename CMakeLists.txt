cmake_minimum_required(VERSION 3.16)
project(json_jsonl_parser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(-Wall -Wextra)

find_path(READLINE_INCLUDE_DIR
    NAMES readline/readline.h
    HINTS /opt/homebrew/opt/readline/include /usr/local/opt/readline/include
)

find_library(READLINE_LIBRARY
    NAMES readline
    HINTS /opt/homebrew/opt/readline/lib /usr/local/opt/readline/lib
)

set(CORE_SOURCES
    week2/scanner.cpp
    week2/parser.cpp
    week2/file-reader.cpp
    week2/integration.cpp
    week2/version.cpp
    week2/query-parser.cpp
)

add_executable(streamline ${CORE_SOURCES} week2/main.cpp)
target_include_directories(streamline PRIVATE week2)

if(READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
    target_include_directories(streamline PRIVATE ${READLINE_INCLUDE_DIR})
    target_link_libraries(streamline PRIVATE ${READLINE_LIBRARY})
    target_compile_definitions(streamline PRIVATE STREAMLINE_READLINE)
endif()

add_executable(tests ${CORE_SOURCES} tests/correctness/tests.cpp)
target_include_directories(tests PRIVATE week2)

add_executable(benchmark ${CORE_SOURCES} tests/performance/benchmark.cpp)
target_include_directories(benchmark PRIVATE week2)

set(WEEK1_SOURCES
    week1/scanner.cpp
    week1/parser.cpp
    week1/file-reader.cpp
)

add_executable(w1tests ${WEEK1_SOURCES} week1/main.cpp)
target_include_directories(w1tests PRIVATE week1)
