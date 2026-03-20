#pragma once

#include "nodes.hpp"
#include "token/token_info.hpp"
#include <token/tokens.hpp>

#include <utils/overload.hpp>
#include <utils/type_to_string.hpp>

#include <deque>
#include <expected>
#include <format>
#include <memory>
#include <optional>
#include <unordered_map>

namespace parser {

using Position = token::SymbolPosition;
using Positions = std::unordered_map<void *, Position>;
using ParseResult = std::expected<std::pair<Program, Positions>, std::string>;

namespace detail {

inline std::string makeSyntaxError(const Position &pos, std::string_view msg) {
  return std::format("Syntax error at {}: {}", pos.toString(), msg);
}

inline std::string makeUnexpectedEnd(std::string_view msg = "") {
  return msg.empty() ? "Unexpected end of input"
                     : std::format("Unexpected end of input: {}", msg);
}

template <typename Iterator>
std::expected<void, std::string> ensureTokens(Iterator it, Iterator end,
                                              std::string_view msg = "") {
  if (it == end)
    return std::unexpected(makeUnexpectedEnd(msg));
  return {};
}

template <typename Token, typename Iterator>
std::expected<Token, std::string> expectToken(Iterator &it, Iterator end) {
  if (auto err = ensureTokens(
          it, end, std::format("expected {}", toStringUnqualified<Token>()));
      !err)
    return std::unexpected(err.error());
  if (auto *t = std::get_if<Token>(&it->tokenType)) {
    auto result = *t;
    ++it;
    return result;
  }
  return std::unexpected(makeSyntaxError(
      it->beginPos,
      std::format("expected {}, got {}", toStringUnqualified<Token>(),
                  toString(it->tokenType))));
}

template <typename Iterator> 
const Position &currentPosition(Iterator it) {
  // TODO: Fix to concept!
  return it->beginPos;
}

///////////////////////////////////////////////////////////////////////////////

template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parsePrimary(Iterator &it, Iterator end, Positions &pos);

template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parsePostfix(Iterator &it, Iterator end, Positions &pos);

template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseUnary(Iterator &it, Iterator end, Positions &pos);

// factor ::= unary ( ('*'|'/'|'%') unary )*
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseFactor(Iterator &it, Iterator end, Positions &pos) {
  auto left = parseUnary(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end) {
    struct Op {
      int kind;
    };
    auto op = std::visit(
        overloaded{
            [](const token::Product &) -> std::optional<Op> { return Op{0}; },
            [](const token::Division &) -> std::optional<Op> { return Op{1}; },
            [](const token::Module &) -> std::optional<Op> { return Op{2}; },
            [](const auto &) -> std::optional<Op> { return std::nullopt; }},
        it->tokenType);
    if (!op)
      break;

    ++it;
    auto right = parseUnary(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    ExpressionVariant newNode;
    switch (op->kind) {
    case 0:
      newNode = Multiplication{
          std::make_shared<ExpressionVariant>(std::move(*left)),
          std::make_shared<ExpressionVariant>(std::move(*right))};
      break;

    case 1:
      newNode =
          Division{std::make_shared<ExpressionVariant>(std::move(*left)),
                   std::make_shared<ExpressionVariant>(std::move(*right))};
      break;

    case 2:
      newNode =
          Remainder{std::make_shared<ExpressionVariant>(std::move(*left)),
                    std::make_shared<ExpressionVariant>(std::move(*right))};
    }

    left = std::move(newNode);
  }
  return left;
}

// term ::= factor ( ('-'|'+') factor )*
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseTerm(Iterator &it, Iterator end, Positions &pos) {
  auto left = parseFactor(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end) {
    auto isPlus = std::visit(
        overloaded{
            [](const token::Plus &) -> std::optional<bool> { return true; },
            [](const token::Minus &) -> std::optional<bool> { return false; },
            [](const auto &) -> std::optional<bool> { return std::nullopt; }},
        it->tokenType);
    if (!isPlus.has_value()) {
      break;
    }

    ++it;
    auto right = parseFactor(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    if (isPlus.value()) {
      left = Addition{std::make_shared<ExpressionVariant>(std::move(*left)),
                      std::make_shared<ExpressionVariant>(std::move(*right))};
    } else {
      left =
          Subtraction{std::make_shared<ExpressionVariant>(std::move(*left)),
                      std::make_shared<ExpressionVariant>(std::move(*right))};
    }
  }
  return left;
}

// comparison ::= term ( ('<'|'>'|'<='|'>=') term )*
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseComparison(Iterator &it, Iterator end, Positions &pos) {
  auto left = parseTerm(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end) {
    struct Op {
      int kind;
    };
    auto op = std::visit(
        overloaded{
            [](const token::Less &) -> std::optional<Op> { return Op{0}; },
            [](const token::Greater &) -> std::optional<Op> { return Op{1}; },
            [](const token::LessEqual &) -> std::optional<Op> { return Op{2}; },
            [](const token::GreaterEqual &) -> std::optional<Op> {
              return Op{3};
            },
            [](const auto &) -> std::optional<Op> { return std::nullopt; }},
        it->tokenType);
    if (!op.has_value())
      break;

    ++it;
    auto right = parseTerm(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    ExpressionVariant newNode;
    switch (op->kind) {
    case 0:
      newNode =
          LessThan{std::make_shared<ExpressionVariant>(std::move(*left)),
                   std::make_shared<ExpressionVariant>(std::move(*right))};
      break;
    case 1:
      newNode =
          GreaterThan{std::make_shared<ExpressionVariant>(std::move(*left)),
                      std::make_shared<ExpressionVariant>(std::move(*right))};
      break;
    case 2:
      newNode =
          LessEqual{std::make_shared<ExpressionVariant>(std::move(*left)),
                    std::make_shared<ExpressionVariant>(std::move(*right))};
      break;
    case 3:
      newNode =
          GreaterEqual{std::make_shared<ExpressionVariant>(std::move(*left)),
                       std::make_shared<ExpressionVariant>(std::move(*right))};
      break;
    }
    left = std::move(newNode);
  }
  return left;
}

// equality ::= comparison ( ('=='|'!=') comparison )*
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseEquality(Iterator &it, Iterator end, Positions &pos) {
  auto left = parseComparison(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end) {
    auto isEqual = std::visit(
        overloaded{
            [](const token::Equal &) -> std::optional<bool> { return true; },
            [](const token::NotEqual &) -> std::optional<bool> { return false; },
            [](const auto &) -> std::optional<bool> { return std::nullopt; }},
        it->tokenType);
    if (!isEqual.has_value())
      break;

    ++it;
    auto right = parseComparison(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    if (isEqual.value())
      left = Equal{std::make_shared<ExpressionVariant>(std::move(*left)),
                   std::make_shared<ExpressionVariant>(std::move(*right))};
    else
      left = NotEqual{std::make_shared<ExpressionVariant>(std::move(*left)),
                      std::make_shared<ExpressionVariant>(std::move(*right))};
  }
  return left;
}

// logical_and ::= equality ( "and" equality )*
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseLogicalAnd(Iterator &it, Iterator end, Positions &pos) {
  auto left = parseEquality(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end && std::holds_alternative<token::And>(it->tokenType)) {
    ++it;
    auto right = parseEquality(it, end, pos);
    if (!right)
      return std::unexpected(right.error());
    left = And{std::make_shared<ExpressionVariant>(std::move(*left)),
               std::make_shared<ExpressionVariant>(std::move(*right))};
  }
  return left;
}

// logical_or ::= logical_and ( ("or"|"xor") logical_and )*
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseLogicalOr(Iterator &it, Iterator end, Positions &pos) {
  auto left = parseLogicalAnd(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end) {
    struct Op {
      int kind;
    };
    auto op = std::visit(
        overloaded{
            [](const token::Or &) -> std::optional<Op> { return Op{0}; },
            [](const token::Xor &) -> std::optional<Op> { return Op{1}; },
            [](const auto &) -> std::optional<Op> { return std::nullopt; }},
        it->tokenType);
    if (!op.has_value())
      break;

    ++it;
    auto right = parseLogicalAnd(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    ExpressionVariant newNode;
    if (op->kind == 0)
      newNode = Or{std::make_shared<ExpressionVariant>(std::move(*left)),
                   std::make_shared<ExpressionVariant>(std::move(*right))};
    else
      newNode = Xor{std::make_shared<ExpressionVariant>(std::move(*left)),
                    std::make_shared<ExpressionVariant>(std::move(*right))};
    left = std::move(newNode);
  }
  return left;
}

// assignment ::= logical_or ( "<-" assignment )?
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseAssignment(Iterator &it, Iterator end, Positions &pos) {
  auto left = parseLogicalOr(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  if (it != end && std::holds_alternative<token::Assign>(it->tokenType)) {
    ++it;
    auto right = parseAssignment(it, end, pos);
    if (!right)
      return std::unexpected(right.error());
    return Assign{std::make_shared<ExpressionVariant>(std::move(*left)),
                  std::make_shared<ExpressionVariant>(std::move(*right))};
  }
  return left;
}

// expression ::= assignment
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseExpression(Iterator &it, Iterator end, Positions &pos) {
  return parseAssignment(it, end, pos);
}

// primary ::= literal | identifier | '(' expression ')'
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parsePrimary(Iterator &it, Iterator end, Positions &pos) {
  if (auto err = ensureTokens(it, end, "primary expression"); !err)
    return std::unexpected(err.error());

  return std::visit(
      overloaded{
          [&](const token::IntLiteral &lit)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            return IntLiteral{lit.value};
          },
          [&](const token::FltLiteral &lit)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            return FltLiteral{lit.value};
          },
          [&](const token::StrLiteral &lit)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            return StrLiteral{lit.value};
          },
          [&](const token::Identificator &id)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            return Identificator{id.value};
          },
          [&](const token::LeftParenthesis &)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            auto inner = parseExpression(it, end, pos);
            if (!inner)
              return std::unexpected(inner.error());
            auto rparen = expectToken<token::RightParenthesis>(it, end);
            if (!rparen)
              return std::unexpected(rparen.error());
            return inner;
          },
          [&](const auto &) -> std::expected<ExpressionVariant, std::string> {
            return std::unexpected(
                makeSyntaxError(it->beginPos, "expected primary expression"));
          }},
      it->tokenType);
}

// postfix ::= primary ( call )*
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parsePostfix(Iterator &it, Iterator end, Positions &pos) {
  auto left = parsePrimary(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end &&
         std::holds_alternative<token::LeftParenthesis>(it->tokenType)) {
    ++it; // '('
    std::deque<ExpressionVariant> args;
    if (it != end &&
        !std::holds_alternative<token::RightParenthesis>(it->tokenType)) {
      while (true) {
        auto arg = parseExpression(it, end, pos);
        if (!arg)
          return std::unexpected(arg.error());
        args.push_back(std::move(*arg));
        if (it == end)
          return std::unexpected(makeUnexpectedEnd("in argument list"));
        bool more = std::visit(
            overloaded{[&it](const token::Comma &) {
                         ++it;
                         return true;
                       },
                       [](const token::RightParenthesis &) { return false; },
                       [&](const auto &) -> bool { return false; }},
            it->tokenType);
        if (!more)
          break;
      }
    }
    auto rparen = expectToken<token::RightParenthesis>(it, end);
    if (!rparen)
      return std::unexpected(rparen.error());
    left = Call{std::make_shared<ExpressionVariant>(std::move(*left)),
                std::move(args)};
  }
  return left;
}

// unary ::= ("not" | "+" | "-") unary | postfix
template <typename Iterator>
std::expected<ExpressionVariant, std::string>
parseUnary(Iterator &it, Iterator end, Positions &pos) {
  if (it == end)
    return parsePostfix(it, end, pos);

  return std::visit(
      overloaded{
          [&](const token::Not &)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            auto operand = parseUnary(it, end, pos);
            if (!operand)
              return std::unexpected(operand.error());
            return Not{
                std::make_shared<ExpressionVariant>(std::move(*operand))};
          },
          [&](const token::Plus &)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            auto operand = parseUnary(it, end, pos);
            if (!operand)
              return std::unexpected(operand.error());
            return UnaryPlus{
                std::make_shared<ExpressionVariant>(std::move(*operand))};
          },
          [&](const token::Minus &)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            auto operand = parseUnary(it, end, pos);
            if (!operand)
              return std::unexpected(operand.error());
            return UnaryMinus{
                std::make_shared<ExpressionVariant>(std::move(*operand))};
          },
          [&](const auto &) -> std::expected<ExpressionVariant, std::string> {
            return parsePostfix(it, end, pos);
          }},
      it->tokenType);
}

///////////////////////////////////////////////////////////////////////////////

template <typename Iterator>
std::expected<StatementVariant, std::string>
parseStatement(Iterator &it, Iterator end, Positions &pos);

template <typename Iterator>
std::expected<Block, std::string> 
parseBlock(Iterator &it, Iterator end, Positions &pos) {
  auto lbrace = expectToken<token::LeftBrace>(it, end);
  if (!lbrace)
    return std::unexpected(lbrace.error());

  std::deque<StatementVariant> stmts;
  while (it != end && !std::holds_alternative<token::RightBrace>(it->tokenType)) {
    auto stmt = parseStatement(it, end, pos);
    if (!stmt)
      return std::unexpected(stmt.error());
    stmts.push_back(std::move(*stmt));
  }
  auto rbrace = expectToken<token::RightBrace>(it, end);
  if (!rbrace)
    return std::unexpected(rbrace.error());
  return Block{std::move(stmts)};
}

template <typename Iterator>
std::expected<StatementVariant, std::string>
parseReturnStatement(Iterator &it, Iterator end, Positions &pos) {
  ++it; // 'ret'
  std::optional<ExpressionVariant> value;
  if (it != end && !std::holds_alternative<token::RightBrace>(it->tokenType)) {
    auto expr = parseExpression(it, end, pos);
    if (!expr)
      return std::unexpected(expr.error());
    value = std::move(*expr);
  }
  return StatementVariant{ReturnStatement{std::move(value)}};
}

template <typename Iterator>
std::expected<StatementVariant, std::string>
parseIfStatement(Iterator &it, Iterator end, Positions &pos) {
  ++it; // 'if'
  auto lparen = expectToken<token::LeftParenthesis>(it, end);
  if (!lparen)
    return std::unexpected(lparen.error());
  auto cond = parseExpression(it, end, pos);
  if (!cond)
    return std::unexpected(cond.error());
  auto rparen = expectToken<token::RightParenthesis>(it, end);
  if (!rparen)
    return std::unexpected(rparen.error());
  auto thenBlock = parseBlock(it, end, pos);
  if (!thenBlock)
    return std::unexpected(thenBlock.error());

  std::optional<std::shared_ptr<Block>> elseBlock;
  if (it != end && std::holds_alternative<token::Else>(it->tokenType)) {
    ++it;
    auto elseBlk = parseBlock(it, end, pos);
    if (!elseBlk)
      return std::unexpected(elseBlk.error());
    elseBlock = std::make_shared<Block>(std::move(*elseBlk));
  }
  return StatementVariant{
      IfStatement{std::move(*cond),
                  std::make_shared<Block>(std::move(*thenBlock)), elseBlock}};
}

template <typename Iterator>
std::expected<StatementVariant, std::string>
parseWhileStatement(Iterator &it, Iterator end, Positions &pos) {
  ++it; // 'while'
  auto lparen = expectToken<token::LeftParenthesis>(it, end);
  if (!lparen)
    return std::unexpected(lparen.error());
  auto cond = parseExpression(it, end, pos);
  if (!cond)
    return std::unexpected(cond.error());
  auto rparen = expectToken<token::RightParenthesis>(it, end);
  if (!rparen)
    return std::unexpected(rparen.error());
  auto body = parseBlock(it, end, pos);
  if (!body)
    return std::unexpected(body.error());
  return StatementVariant{WhileStatement{
      std::move(*cond), std::make_shared<Block>(std::move(*body))}};
}

template <typename Iterator>
std::expected<StatementVariant, std::string>
parseVariableDeclaration(Iterator &it, Iterator end, Positions &pos) {
  auto name = std::get<token::Identificator>(it->tokenType);
  ++it; // identifier
  ++it; // '<-'
  auto value = parseExpression(it, end, pos);
  if (!value)
    return std::unexpected(value.error());
  return StatementVariant{VariableDeclaration{name.value, std::move(*value)}};
}

template <typename Iterator>
std::expected<StatementVariant, std::string>
parseExpressionStatement(Iterator &it, Iterator end, Positions &pos) {
  auto expr = parseExpression(it, end, pos);
  if (!expr)
    return std::unexpected(expr.error());
  return StatementVariant{ExpressionStatement{std::move(*expr)}};
}

template <typename Iterator>
std::expected<StatementVariant, std::string>
parseStatement(Iterator &it, Iterator end, Positions &pos) {
  if (auto err = ensureTokens(it, end, "statement"); !err)
    return std::unexpected(err.error());

  if (std::holds_alternative<token::Return>(it->tokenType))
    return parseReturnStatement(it, end, pos);
  else if (std::holds_alternative<token::If>(it->tokenType))
    return parseIfStatement(it, end, pos);
  else if (std::holds_alternative<token::Cycle>(it->tokenType))
    return parseWhileStatement(it, end, pos);
  else if (std::holds_alternative<token::LeftBrace>(it->tokenType)) {
    auto block = parseBlock(it, end, pos);
    if (!block)
      return std::unexpected(block.error());
    return StatementVariant{std::move(*block)};
  } else if (std::holds_alternative<token::Identificator>(it->tokenType)) {
    auto next = it;
    ++next;
    if (next != end && std::holds_alternative<token::Assign>(next->tokenType))
      return parseVariableDeclaration(it, end, pos);
    else
      return parseExpressionStatement(it, end, pos);
  } else {
    return parseExpressionStatement(it, end, pos);
  }
}

///////////////////////////////////////////////////////////////////////////////

template <typename Iterator>
std::expected<FunctionDeclaration, std::string>
parseFunctionDeclaration(Iterator &it, Iterator end, Positions &pos) {
  auto func = expectToken<token::Function>(it, end);
  if (!func)
    return std::unexpected(func.error());

  auto name = expectToken<token::Identificator>(it, end);
  if (!name)
    return std::unexpected(name.error());

  auto lparen = expectToken<token::LeftParenthesis>(it, end);
  if (!lparen)
    return std::unexpected(lparen.error());

  std::vector<std::string> params;
  if (it != end &&
      !std::holds_alternative<token::RightParenthesis>(it->tokenType)) {
    while (true) {
      auto param = expectToken<token::Identificator>(it, end);
      if (!param)
        return std::unexpected(param.error());
      params.push_back(param->value);
      if (it == end)
        return std::unexpected(makeUnexpectedEnd("in parameter list"));
      bool more = std::visit(
          overloaded{[&it](const token::Comma &) {
                       ++it;
                       return true;
                     },
                     [](const token::RightParenthesis &) { return false; },
                     [&](const auto &) -> bool { return false; }},
          it->tokenType);
      if (!more)
        break;
    }
  }
  auto rparen = expectToken<token::RightParenthesis>(it, end);
  if (!rparen)
    return std::unexpected(rparen.error());

  auto body = parseBlock(it, end, pos);
  if (!body)
    return std::unexpected(body.error());

  return FunctionDeclaration{name->value, std::move(params), std::move(*body)};
}

} // namespace detail

///////////////////////////////////////////////////////////////////////////////

template <typename Iterator> ParseResult parse(Iterator &begin, Iterator end) {
  Program program;
  Positions positions;

  while (begin != end) {
    auto pos = detail::currentPosition(begin);
    auto func = detail::parseFunctionDeclaration(begin, end, positions);
    if (!func)
      return std::unexpected(func.error());
    program.functions.push_back(std::move(*func));
    positions[&program.functions.back()] = pos;
  }
  return std::make_pair(std::move(program), std::move(positions));
}

template <typename Range> ParseResult parse(const Range &tokens) {
  auto begin = tokens.begin();
  return parse(begin, tokens.end());
}

} // namespace parser