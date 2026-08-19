cmake_minimum_required(VERSION 3.16)
project(json_jsonl_parser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#We cannot use a debugger due to this.
set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)

if(MSVC)
    add_compile_options(/w)
else()
    add_compile_options(-w)
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

find_path(READLINE_INCLUDE_DIR
    NAMES readline/readline.h
    HINTS /opt/homebrew/opt/readline/include /usr/local/opt/readline/include
)

find_library(READLINE_LIBRARY
    NAMES readline
    HINTS /opt/homebrew/opt/readline/lib /usr/local/opt/readline/lib
)

set(CORE_SOURCES
    week4/file-scanner.cpp
    week4/file-parser.cpp
    week4/query-engine.cpp
    week4/version.cpp
    week4/query-scanner.cpp
    week4/query-parser.cpp
)

add_executable(streamline ${CORE_SOURCES} week4/main.cpp)
target_include_directories(streamline PRIVATE week4)
target_link_libraries(streamline PRIVATE Threads::Threads)

if(READLINE_INCLUDE_DIR AND READLINE_LIBRARY)
    target_include_directories(streamline PRIVATE ${READLINE_INCLUDE_DIR})
    target_link_libraries(streamline PRIVATE ${READLINE_LIBRARY})
    target_compile_definitions(streamline PRIVATE STREAMLINE_READLINE)
endif()

add_executable(tests ${CORE_SOURCES} tests/correctness/tests.cpp)
target_include_directories(tests PRIVATE week4)
target_link_libraries(tests PRIVATE Threads::Threads)

add_executable(benchmark ${CORE_SOURCES} tests/performance/benchmark.cpp)
target_include_directories(benchmark PRIVATE week4)
target_link_libraries(benchmark PRIVATE Threads::Threads)
target_compile_definitions(benchmark PRIVATE
    STREAMLINE_PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
    STREAMLINE_SYSTEM_NAME="${CMAKE_SYSTEM_NAME}"
    STREAMLINE_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}"
    STREAMLINE_COMPILER="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
    STREAMLINE_BUILD_TYPE="$<CONFIG>"
)
