cmake_minimum_required(VERSION 3.14)
project(ChuvashProgrammingLanguage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXECUTABLE_SUFFIX ".out")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

set(CMAKE_STATIC_LIBRARY_PREFIX "")
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_CXX_FLAGS "-std=c++23 -Wall -Wextra -Wpedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-std=c++23 -fsanitize=undefined -fsanitize=address -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(source)