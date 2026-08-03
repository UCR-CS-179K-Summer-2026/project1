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
    week1/scanner.cpp
    week1/parser.cpp
    week1/file-reader.cpp
)

add_executable(demo ${CORE_SOURCES} week1/demo.cpp)
target_include_directories(demo PRIVATE week1)

# demo.cpp opens "students.json" via a relative path, so copy the sample
# data next to the built binary — otherwise ./build/demo only works when
# run with week1/ as the working directory.
add_custom_command(TARGET demo POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/week1/students.json
        ${CMAKE_SOURCE_DIR}/week1/students.jsonl
        $<TARGET_FILE_DIR:demo>
)

add_executable(test_lookup ${CORE_SOURCES} week1/test_lookup.cpp)
target_include_directories(test_lookup PRIVATE week1)
