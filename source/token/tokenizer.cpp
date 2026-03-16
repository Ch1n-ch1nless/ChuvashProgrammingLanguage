#include <token/tokenizer.hpp>

#include <cctype>
#include <charconv>
#include <expected>
#include <map>
#include <string>

namespace token {

// TODO: Rewrite to UTF-8 format
bool isDigit(const char& c) {
    return std::isdigit(c);
}

// TODO: Rewrite to UTF-8 format
bool isLetter(const char& c) {
    return std::isalpha(c);
}

// TODO: Rewrite to UTF-8 format
bool isIdentifierStart(const char& c) {
    return isLetter(c) || c == '_';
}

// TODO: Rewrite to UTF-8 format
bool isIdentifierPart(const char& c) {
    return isIdentifierStart(c) || isDigit(c);
}

// TODO: Rewrite to UTF-8 format
bool isOperatorChar(const char& c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '%' ||
           c == '=' || c == '!' || c == '<' || c == '>' || c == '(' ||
           c == ')' || c == '&' || c == '|' || c == '^' || c == '~' ||
           c == '{' || c == '}' ;
}

///////////////////////////////////////////////////////////////////////////////

#define EMPTY_TOKEN(type, token_name) {token_name, type{}},
#define EMPTY_TOKEN_LAST(type, token_name) {token_name, type{}},

static const std::map<std::string, TokenVariant> kStringToToken = {
LOGICAL_OPERATORS
ARITHMETIC_OPERATORS
KEYWORDS
GRAMMAR_TOKENS
};

#undef EMPTY_TOKEN
#undef EMPTY_TOKEN_LAST

///////////////////////////////////////////////////////////////////////////////

void processSpaceSymbols(
    std::string::const_iterator& currentSymbol,
    const std::string::const_iterator& endSymbol,
    SymbolPosition& currentPosition
) {
  while (currentSymbol != endSymbol) {
    if (*currentSymbol == '\n') {
      ++currentPosition.line;
      currentPosition.column = 0;
      ++currentSymbol;
    } else if (std::isspace(static_cast<unsigned char>(*currentSymbol))) {
      ++currentPosition.column;
      ++currentSymbol;
    } else {
      break;
    }
  }
}

///////////////////////////////////////////////////////////////////////////////

std::expected<TokenInfo, std::string> readNumericalLiteral(
    std::string::const_iterator& currentSymbol,
    const std::string::const_iterator& endSymbol,
    SymbolPosition& currentPosition
) {
  if (!isDigit(*currentSymbol) && *currentSymbol != '.') {
    return std::unexpected("Not a digit");
  }

  auto beginPos = currentPosition;
  auto start = currentSymbol;
  bool isFloat = false;

  while (currentSymbol != endSymbol && isDigit(*currentSymbol)) {
    ++currentSymbol;
    ++currentPosition.column;
  }

  if (currentSymbol != endSymbol && *currentSymbol == '.') {
    isFloat = true;
    ++currentSymbol;
    ++currentPosition.column;

    if (currentSymbol == endSymbol || !isDigit(*currentSymbol)) {
      return std::unexpected("Expected digits after decimal point at " +
                              std::to_string(currentPosition.line) + ":" +
                              std::to_string(currentPosition.column));
    }
    while (currentSymbol != endSymbol && isDigit(*currentSymbol)) {
      ++currentSymbol;
      ++currentPosition.column;
    }
  }

  std::string numberStr(start, currentSymbol);
  if (isFloat) {
    float value = 0.f;
    std::from_chars(&(*start), &(*currentSymbol), value);
    return TokenInfo{beginPos, FltLiteral{value}};
  } else {
    int value = 0;
    std::from_chars(&(*start), &(*currentSymbol), value);
    return TokenInfo{beginPos, IntLiteral{value}};
  }
}

///////////////////////////////////////////////////////////////////////////////

std::expected<TokenInfo, std::string> readIdentifierOrKeyword(
    std::string::const_iterator& currentSymbol,
    const std::string::const_iterator& endSymbol,
    SymbolPosition& currentPosition
) {
  if (!isIdentifierStart(*currentSymbol)) {
    return std::unexpected("Not an identifier start");
  }

  auto beginPos = currentPosition;
  auto start = currentSymbol;

  while (currentSymbol != endSymbol && isIdentifierPart(*currentSymbol)) {
    ++currentSymbol;
    ++currentPosition.column;
  }

  std::string ident(start, currentSymbol);

  auto it = kStringToToken.find(ident);
  if (it != kStringToToken.end()) {
    return TokenInfo{beginPos, it->second};
  } else {
    return TokenInfo{beginPos, Identificator{ident}};
  }
}

///////////////////////////////////////////////////////////////////////////////

std::expected<TokenInfo, std::string> readOperator(
  std::string::const_iterator& currentSymbol,
  const std::string::const_iterator& endSymbol,
  SymbolPosition& currentPosition
) {
  if (!isOperatorChar(*currentSymbol)) {
    return std::unexpected("Not an operator character");
  }

  auto beginPos = currentPosition;
  auto start = currentSymbol;

  while (currentSymbol != endSymbol && isOperatorChar(*currentSymbol)) {
    ++currentSymbol;
    ++currentPosition.column;
  }

  std::string opStr(start, currentSymbol);
  while (!opStr.empty()) {
    auto it = kStringToToken.find(opStr);
    if (it != kStringToToken.end()) {
      return TokenInfo{beginPos, it->second};
    }
    opStr.pop_back();
    --currentPosition.column;
    --currentSymbol;
  }

  return std::unexpected("Unknown operator at " +
                          std::to_string(beginPos.line) + ":" +
                          std::to_string(beginPos.column));
}

///////////////////////////////////////////////////////////////////////////////

std::expected<std::vector<TokenInfo>, std::string> tokenize(const std::string& text) {
  std::vector<TokenInfo> result;
  auto currentSymbol = text.begin();
  const auto endSymbol = text.end();
  SymbolPosition currentPosition{1, 0};

  while (currentSymbol != endSymbol) {
    processSpaceSymbols(currentSymbol, endSymbol, currentPosition);
    if (currentSymbol == endSymbol) break;

    if (auto numToken = readNumericalLiteral(currentSymbol, endSymbol, currentPosition)) {
      result.push_back(*numToken);
      continue;
    }

    if (auto idToken = readIdentifierOrKeyword(currentSymbol, endSymbol, currentPosition)) {
      result.push_back(*idToken);
      continue;
    }

    if (auto opToken = readOperator(currentSymbol, endSymbol, currentPosition)) {
      result.push_back(*opToken);
      continue;
    }

    return std::unexpected("Unexpected character '" +
                            std::string(1, *currentSymbol) + "' at " +
                            std::to_string(currentPosition.line) + ":" +
                            std::to_string(currentPosition.column));
  }

  return result;
}

}