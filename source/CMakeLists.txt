add_subdirectory(token)

add_executable(compiler main.cpp)
target_link_libraries(compiler Token)