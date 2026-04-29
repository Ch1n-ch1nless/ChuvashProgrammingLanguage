#include <iostream>
#include <parser/parser.hpp>
#include <parser/print_ast.hpp>
#include <parser/graphviz_ast.hpp>
#include <parser/interpreter.hpp>
#include <string>
#include <token/to_string.hpp>
#include <token/tokenizer.hpp>
#include "parser/interpreter.hpp"
#include "utils/overload.hpp"

int main() {
  // Current simple programm
  std::string text = R"(
  func main() : int {
    ret factorial(6) + fibonacci(6)
  }

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
)";

  // Execute tokenization stage
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
  if (parsingResult.has_value()) {
    parser::printAST(*parsingResult);
    parser::ASTGraphVizDumper dumper("../img");
    dumper.dumpToPng(*parsingResult);
  } else {
    std::cout << parsingResult.error();
  }
  std::cout << "==============================\n\n";

  // Interpret program:
  if (parsingResult.has_value()) {
    parser::visitor::InterpretVisitor interpreter;
    auto result = interpreter.interpret(parsingResult->first);
    if (result.has_value()) {
      std::cout << "Interpretation result: " << std::visit(
        overloaded{
          [](const auto& val) {
            return std::to_string(val);
          },
          [](const std::string& val) {
            return val;
          },
          [](const parser::visitor::runtime::Unit&) {
            return std::string("Unit");
          }
        }, result->value) << "\n";
    } else {
      std::cerr << "Interpretation error: " << result.error() << "\n";
    }
  } else {
    std::cerr << "Interpretation is failed!\n";
  }

  return 0;
}