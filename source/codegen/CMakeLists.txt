add_library(Codegen STATIC llvm_ir_builder.cpp)
add_library(Interpreter STATIC interpreter.cpp)

target_link_libraries(Codegen Interpreter)