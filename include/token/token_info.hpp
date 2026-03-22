#pragma once

#include <cstddef>
#include <format>
#include <token/tokens.hpp>

namespace token {

struct SymbolPosition {
  std::size_t line;
  std::size_t column;

  std::string toString() const { return std::format("({}, {})", line, column); }
};

struct TokenInfo {
  SymbolPosition beginPos;
  TokenVariant token;
};

}  // namespace token