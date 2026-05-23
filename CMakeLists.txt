cmake_minimum_required(VERSION 3.20)

project(ChuvashProgrammingLanguage)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_COMPILER clang-20)
set(CMAKE_CXX_COMPILER clang++-20)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${LLVM_INCLUDE_DIRS}
)

add_definitions(${LLVM_DEFINITIONS})

# =========================================
# Subdirectories
# =========================================

add_subdirectory(source/token)
add_subdirectory(source/parser)
add_subdirectory(source/sema)
add_subdirectory(source/codegen)
add_subdirectory(source/driver)

# =========================================
# Main executable
# =========================================

add_executable(compiler.out
    source/main.cpp
)

target_link_libraries(compiler.out PRIVATE
    Driver
)