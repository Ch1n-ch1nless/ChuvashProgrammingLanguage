add_subdirectory(codegen)
add_subdirectory(token)
add_subdirectory(parser)
add_subdirectory(sema)

add_executable(compiler
    main.cpp
)

target_link_libraries(compiler
    PRIVATE
    Token
    Parser
    Sema
    Codegen
    ToString
    PrintAST
    DumpAST
    ${LLVM_LIBS}
)