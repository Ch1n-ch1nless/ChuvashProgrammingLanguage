#pragma once

#include <expected>
#include <token/to_string.hpp>
#include <token/tokens.hpp>
#include <unordered_map>
#include <utils/overload.hpp>
#include <utils/type_to_string.hpp>
#include <vector>

#include "nodes.hpp"
#include "token/token_info.hpp"

namespace parser {

using Position = token::SymbolPosition;
using Positions = std::unordered_map<void *, Position>;
using ParseResult = std::expected<std::pair<Program, Positions>, std::string>;

using TokenRange = std::vector<token::TokenInfo>;
using TokenIterator = std::vector<token::TokenInfo>::const_iterator;

ParseResult parse(const TokenRange &tokens);

}  // namespace parser