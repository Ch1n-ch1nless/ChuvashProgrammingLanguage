#include <format>
#include <string>
#include <token/to_string.hpp>
#include <utils/overload.hpp>
#include <utils/type_to_string.hpp>
#include <variant>

#include "token/tokens.hpp"

namespace token {

std::string toString(const TokenVariant& token) {
  return std::visit(
      overloaded{
          []<concepts::IsLogicalOperator Type>(
              [[maybe_unused]] const Type& op) {
            return std::format("LogicalOperator: {}",
                               ::toStringUnqualified<Type>());
          },
          []<concepts::IsOperator Type>([[maybe_unused]] const Type& op) {
            return std::format("Operator: {}",
                               ::toStringUnqualified<Type>());
          },
          [](const StrLiteral& token) {
            return std::format("Literal {}", token.value);
          },
          []<concepts::IsLiteral Type>([[maybe_unused]] const Type& literal) {
            return std::format("Literal: {}", std::to_string(literal.value));
          },
          []<concepts::IsKeyword Type>([[maybe_unused]] const Type& token) {
            return std::format("Keyword: {}",
                               ::toStringUnqualified<Type>());
          },
          []<concepts::IsBuildInType Type>(
              [[maybe_unused]] const Type& token) {
            return std::format("BuildInType: {}",
                               ::toStringUnqualified<Type>());
          },
          [](const Identificator& token) {
            return std::format("Identificator: \"{}\"", token.value);
          },

// Overloads for grammar symbols:
#define EMPTY_TOKEN(type, token_name)                                       \
  []([[maybe_unused]] const type& grammarToken) {                           \
    return std::format("GrammarSymbol: {}", ::toStringUnqualified<type>()); \
  },

#define EMPTY_TOKEN_LAST(type, token_name)                                  \
  []([[maybe_unused]] const type& grammarToken) {                           \
    return std::format("GrammarSymbol: {}", ::toStringUnqualified<type>()); \
  },

          GRAMMAR_TOKENS

#undef EMPTY_TOKEN
#undef EMPTY_TOKEN_LAST
      },
      token);
}

}  // namespace token