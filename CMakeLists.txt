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

# LLVM part in CMakeLists.txt
add_compile_options(-fno-rtti)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(SYSTEM
    ${LLVM_INCLUDE_DIRS}
)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(LLVM_LIBS
    Core
    Support
    IRReader
    Analysis
)

add_subdirectory(source)