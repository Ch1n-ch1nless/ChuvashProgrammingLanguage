#include <iostream>
#include <parser/parser.hpp>
#include <parser/print_ast.hpp>
#include <token/to_string.hpp>
#include <token/tokenizer.hpp>

int main() {
  // Current simple programm
  std::string text = "func main() { ret 52 }";

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
  } else {
    std::cout << parsingResult.error();
  }
  std::cout << "==============================\n\n";

  return 0;
}