#include "../../include/token/tokenizer.hpp"

#include <cassert>
#include <charconv>
#include <map>
#include <optional>
using namespace token;

// TODO: Rewrite to UTF-8 format
bool isDigit(char c) { 
  return isdigit(c); 
}

// TODO: Rewrite to UTF-8 format
bool isLetter(char c) {
  return std::isalpha(c); 
}

// TODO: Rewrite to UTF-8 format
bool isIdentifierStart(char c) {
  return isLetter(c) || c == '_';
}

// TODO: Rewrite to UTF-8 format
bool isIdentifierPart(char c) {
  return isIdentifierStart(c) || isDigit(c);
}

// TODO: Refact! Now it doesn't have any connection with `token_list`
const std::map<std::string, TokenVariant> kStringToToken = {
  {"func", Function{}},
  {"ret", Return{}},
  {"if", If{}},
  {"else", Else{}},
  {"while", Cycle{}},
  {"not", Not{}},
  {"and", And{}},
  {"or", Or{}},
  {"xor", Xor{}},
};

void processSpaceSymbols(
  std::string::const_iterator& currentSymbol,
  SymbolPosition& currentPosition
) {
  if (*currentSymbol == '\n') {
    currentPosition.line++;
    currentPosition.column = 0;
    currentSymbol++;
  } else if (std::isspace(static_cast<unsigned char>(*currentSymbol))) {
    currentPosition.column++;
    currentSymbol++;
  }
}

std::optional<TokenInfo> readNumericalLiteral(
  std::string::const_iterator& currentSymbol,
  const std::string::const_iterator& endSymbol,
  SymbolPosition& currentPosition
) {
  // Check correct input of our function
  if (!isDigit(*currentSymbol)) {
    return std::nullopt;
  }

  bool isFloat = false;
  auto beginPosition = currentPosition;
  auto numberBegin = currentSymbol;

  while (currentSymbol != endSymbol) {
    if (isDigit(*currentSymbol)) {
      currentSymbol++;
      currentPosition.column++;
    } else if (*currentSymbol == '.') {
      return readFloatLiteral(...);
    } else {
      return std::nullopt;
    }
  }

  auto numberEnd = currentSymbol;

  int newValue = 0;
  std::from_chars(&(*numberBegin), &(*numberEnd), newValue);
  TokenInfo newToken{beginPosition, IntLiteral{newValue}};

  return {newToken};
}

// TODO: Global refact to UTF-8 format for supporting chuvash and russian symbols:
std::vector<TokenInfo> tokenize(const std::string& text) {
  // ...
  std::vector<TokenInfo> result;
  auto currentSymbol = text.begin();
  const auto endSymbol = text.end();
  SymbolPosition currentPosition{0, 0};

  while (currentSymbol != endSymbol) {
    processSpaceSymbols(currentSymbol, currentPosition);

    if (isDigit(*currentSymbol)) {
      auto numLiteral = readNumericalLiteral(currentSymbol, endSymbol, currentPosition);
      if (!numLiteral.has_value()) {
        // TODO: CHANGE CRINGE ASSERT!
        assert(false && "Error! Incorrect format of numerical literal");
      }
      result.push_back(*numLiteral);
    }

  }

  return result;
}