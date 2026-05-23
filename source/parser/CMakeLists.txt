add_library(Parser STATIC
    parser.cpp
    print_ast.cpp
    graphviz_ast.cpp
)

target_include_directories(Parser PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Parser PUBLIC
    Token
    ToString
)