#include <iostream>
#include <parser/parser.hpp>
#include <parser/print_ast.hpp>
#include <parser/graphviz_ast.hpp>
#include <token/to_string.hpp>
#include <token/tokenizer.hpp>

int main() {
  // Current simple programm
  std::string text = R"(
  func main() : int {
    var c : int
    var x : int
    var y : int
    c <- 3 * 2
    x <- factorial(c)
    y <- factorial(5)
    ret x + y
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
  /*if (parsingResult.has_value()) {
    auto result = parser::InterpretProgram(*parsingResult);
    std::cout << "Result of interpretation:" << result << "\n";
  } else {
    std::cerr << "Interpretation is failed!\n";
  }*/

  return 0;
}