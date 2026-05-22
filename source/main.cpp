#include <iostream>
#include <parser/parser.hpp>
#include <parser/print_ast.hpp>
#include <parser/graphviz_ast.hpp>
#include <codegen/llvm_ir_builder.hpp>
#include <sema/symbol_tree_builder.hpp>
#include <string>
#include <token/to_string.hpp>
#include <token/tokenizer.hpp>

int main() {
  // Пример программы (та же, что и раньше)
  std::string text = R"(
  func fibonacci(n : int) : int {
    var a : int
    a <- 0
    var b : int
    b <- 1
    
    while (n > 0) {
      var temp : int
      temp <- a + b
      a <- b
      b <- temp
      n <- n - 1
    }
    ret a
  }

  func factorial(n : int) : int {
    if (n == 0) {
      ret 1
    } else {
      ret n * factorial(n - 1)
    }
  }

  func main() : int {
    var c : int
    c <- 3 * 2
    var b : int
    b <- factorial(c)
    var a : int
    a <- fibonacci(c)
    ret a + b
  }
)";

  std::cout << "Result of tokenization:\n";
  std::cout << "==============================\n";

  auto tokens = token::tokenize(text);
  if (tokens.has_value()) {
    for (const auto& token : *tokens) {
      std::cout << "Token at (" << token.beginPos.line << ":"
                << token.beginPos.column
                << ") = " << token::toString(token.token) << "\n";
    }
  } else {
    std::cerr << "Tokenization error: " << tokens.error() << std::endl;
    return 1;
  }
  std::cout << "==============================\n\n";

  // Execute parsing stage:
  std::cout << "Result of parsing\n";
  std::cout << "==============================\n";

  auto parsingResult = parser::parse(*tokens);
  if (!parsingResult.has_value()) {
    std::cerr << parsingResult.error() << std::endl;
    return 1;
  }

  const auto& program = parsingResult->first;
  parser::ASTGraphVizDumper dumper("../img");
  dumper.dumpToPng(*parsingResult);
  std::cout << "==============================\n\n";

  parser::sema::SymbolTreeBuilder symbol_builder;
  try {
    symbol_builder.build(program);
    std::cout << "Symbol tree built successfully!\n";
  } catch (const std::exception& ex) {
    std::cerr << "Symbol tree building error: " << ex.what() << "\n";
  }

  std::cout << "\nGenerating LLVM IR...\n";
  std::cout << "==============================\n";

  codegen::IRBuilderVisitor irBuilder;
  llvm::Module* module = irBuilder.generate(program);
  if (module) {
    std::cout << "LLVM IR generated successfully:\n\n";
    irBuilder.dump();
  } else {
    std::cerr << "Failed to generate LLVM IR.\n";
    return 1;
  }

  std::error_code EC;
  llvm::raw_fd_ostream outFile("output.ll", EC);
  if (!EC) {
    module->print(outFile, nullptr);
    std::cout << "\nIR also saved to 'output.ll'\n";
  }

  return 0;
}