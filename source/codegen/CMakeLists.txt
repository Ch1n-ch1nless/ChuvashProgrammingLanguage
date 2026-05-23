add_library(Codegen STATIC
    llvm_ir_builder.cpp
    interpreter.cpp
)

target_include_directories(Codegen PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(Codegen PUBLIC
    Parser
    Sema

    LLVMCore
    LLVMSupport
)

# =========================================
# ObjectEmitter
# =========================================

add_library(ObjectEmitter STATIC
    object_emitter.cpp
)

target_include_directories(ObjectEmitter PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(ObjectEmitter PUBLIC

    LLVMCore
    LLVMSupport

    LLVMTarget
    LLVMMC
    LLVMCodeGen

    LLVMAsmPrinter
    LLVMAsmParser

    LLVMX86CodeGen
    LLVMX86AsmParser
    LLVMX86Desc
    LLVMX86Info
)