#pragma once

#include <cstddef>

#include <token/tokens.hpp>

namespace token {

struct SymbolPosition {
  std::size_t line;
  std::size_t column;
};

struct TokenInfo {
  SymbolPosition beginPos;
  TokenVariant tokenType;
};

}