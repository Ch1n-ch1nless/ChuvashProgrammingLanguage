#include "driver/compiler_driver.hpp"

#include "driver/linker_driver.hpp"

#include <codegen/llvm_ir_builder.hpp>
#include <codegen/object_emitter.hpp>

#include <parser/parser.hpp>

#include <sema/symbol_tree_builder.hpp>

#include <token/tokenizer.hpp>

#include <iostream>

namespace driver {

bool CompilerDriver::compile(
    const std::string& source,
    const std::string& output_name
) {
    // =========================================
    // Tokenization
    // =========================================

    auto tokens = token::tokenize(source);

    if (!tokens.has_value()) {
        std::cerr << tokens.error() << "\n";
        return false;
    }

    // =========================================
    // Parsing
    // =========================================

    auto parsing = parser::parse(*tokens);

    if (!parsing.has_value()) {
        std::cerr << parsing.error() << "\n";
        return false;
    }

    auto& program = parsing->first;

    // =========================================
    // Sema
    // =========================================

    parser::sema::SymbolTreeBuilder sema;

    try {
        sema.build(program);
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n";
        return false;
    }

    // =========================================
    // LLVM IR
    // =========================================

    codegen::IRBuilderVisitor builder;

    llvm::Module* module = builder.generate(program);

    if (!module) {
        std::cerr << "IR generation failed\n";
        return false;
    }

    // =========================================
    // Object emission
    // =========================================

    constexpr auto object_name = "output.o";

    if (!codegen::emitObjectFile(
            module,
            object_name
        )) {
        std::cerr << "Object emission failed\n";
        return false;
    }

    // =========================================
    // Linking
    // =========================================

    if (!LinkerDriver::linkExecutable(
            object_name,
            output_name
        )) {
        std::cerr << "Linking failed\n";
        return false;
    }

    return true;
}

}