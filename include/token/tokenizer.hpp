#pragma once

#include <expected>
#include <vector>
#include <string>

#include "tokens.hpp"
#include "token_info.hpp"

namespace token {

std::expected<std::vector<TokenInfo>, std::string> tokenize(const std::string& text);

}