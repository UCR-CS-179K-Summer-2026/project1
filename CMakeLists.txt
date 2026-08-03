cmake_minimum_required(VERSION 3.16)
project(sluice CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(week1)

add_executable(sluice week1/main.cpp week1/scanner.cpp week1/parser.cpp week1/file-reader.cpp)
add_executable(tests tests/tests.cpp week1/scanner.cpp week1/parser.cpp week1/file-reader.cpp)

enable_testing()
add_test(NAME unit COMMAND tests ${CMAKE_CURRENT_SOURCE_DIR}/tests/data)
