add_subdirectory(codegen)
add_subdirectory(token)
add_subdirectory(parser)
add_subdirectory(sema)
add_subdirectory(driver)

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
    Interpreter
    ObjectEmitter
    Driver

    ${LLVM_LIBS}
)