add_library(Parser STATIC parser.cpp)
add_library(PrintAST STATIC print_ast.cpp)

target_link_libraries(Parser PrintAST)