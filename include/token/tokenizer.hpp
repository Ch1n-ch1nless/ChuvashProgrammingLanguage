#pragma once

#include <vector>

#include "tokens.hpp"
#include "token_info.hpp"

namespace token {

std::vector<TokenInfo> tokenize(const std::string& text);

}
