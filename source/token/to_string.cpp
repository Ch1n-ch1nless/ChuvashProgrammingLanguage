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
          []<LogicalOperator LogicalOperatorT>(
              [[maybe_unused]] const LogicalOperatorT& op) {
            return std::format("LogicalOperator: {}",
                               ::toStringUnqualified<LogicalOperatorT>());
          },
          []<Operator OperatorT>([[maybe_unused]] const OperatorT& op) {
            return std::format("Operator: {}",
                               ::toStringUnqualified<OperatorT>());
          },
          [](const StrLiteral& token) {
            return std::format("Literal {}", token.value);
          },
          []<Literal LiteralT>([[maybe_unused]] const LiteralT& literal) {
            return std::format("Literal: {}", std::to_string(literal.value));
          },
          []<Keyword KeywordT>([[maybe_unused]] const KeywordT& token) {
            return std::format("Keyword: {}",
                               ::toStringUnqualified<KeywordT>());
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