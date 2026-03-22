#pragma once

#include <expected>
#include <string>
#include <token/token_info.hpp>
#include <vector>

namespace token {

std::expected<std::vector<TokenInfo>, std::string> tokenize(
    const std::string& text);

}