#include <iostream>
#include <parser/parser.hpp>
#include <token/to_string.hpp>
#include <token/tokenizer.hpp>

int main() {
  std::string text = "func main() { ret 52 }";
  auto tokens = token::tokenize(text);
  if (tokens.has_value()) {
    for (const auto& token : *tokens) {
      std::cout << "Token at (" << token.beginPos.line << ":"
                << token.beginPos.column
                << ") = " << token::toString(token.tokenType) << "\n";
    }

    auto parsingResult = parser::parse(*tokens);
  } else {
    std::cerr << "Tokenization error: " << tokens.error() << std::endl;
    return 1;
  }
  return 0;
}