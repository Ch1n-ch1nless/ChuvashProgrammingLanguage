add_subdirectory(token)
add_subdirectory(parser)

add_executable(compiler main.cpp)
target_link_libraries(compiler Token)
target_link_libraries(compiler Parser)