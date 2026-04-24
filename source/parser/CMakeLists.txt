add_library(Parser STATIC parser.cpp)
add_library(PrintAST STATIC print_ast.cpp)
add_library(DumpAST STATIC graphviz_ast.cpp)
add_library(InterpretAST STATIC interpreter.cpp)

target_link_libraries(Parser PrintAST)
target_link_libraries(Parser DumpAST)
target_link_libraries(Parser InterpretAST)