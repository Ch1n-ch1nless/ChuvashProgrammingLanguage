#include <parser/parser.hpp>
#include "parser/nodes.hpp"
#include "token/tokens.hpp"

namespace parser {

namespace detail {

inline std::string makeSyntaxError(const Position &pos, std::string_view msg) {
  return std::format("Syntax error at {}: {}", pos.toString(), msg);
}

inline std::string makeUnexpectedEnd(std::string_view msg = "") {
  return msg.empty() ? "Unexpected end of input"
                     : std::format("Unexpected end of input: {}", msg);
}

std::expected<void, std::string> ensureTokens(TokenIterator it, TokenIterator end,
                                              std::string_view msg = "") {
  if (it == end) {
    return std::unexpected(makeUnexpectedEnd(msg));
  }
  return {};
}

template <typename Token>
std::expected<Token, std::string> expectToken(TokenIterator &it, TokenIterator end) {
  if (auto err = ensureTokens(
          it, end, std::format("expected {}", toStringUnqualified<Token>()));
      !err) {
    return std::unexpected(err.error());
  }
  if (auto *t = std::get_if<Token>(&it->token)) {
    auto result = *t;
    ++it;
    return result;
  }
  return std::unexpected(makeSyntaxError(
      it->beginPos,
      std::format("expected {}, got {}", toStringUnqualified<Token>(),
                  token::toString(it->token))));
}

const Position &currentPosition(TokenIterator it) {
  return it->beginPos;
}

///////////////////////////////////////////////////////////////////////////////

// TODO: Come up with a Rust-like wrapper for errors on macroses

std::expected<ExpressionVariant, std::string>
parsePrimary(TokenIterator &it, TokenIterator end, Positions &pos);

std::expected<ExpressionVariant, std::string>
parsePostfix(TokenIterator &it, TokenIterator end, Positions &pos);

std::expected<ExpressionVariant, std::string>
parseUnary(TokenIterator &it, TokenIterator end, Positions &pos);

// factor ::= unary ( ('*'|'/'|'%') unary )*
std::expected<ExpressionVariant, std::string>
parseFactor(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto left = parseUnary(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end) {
    struct Op {
      enum : int {
        kProduct,
        kDivision,
        kModule
      } kind;
    };
    auto op = std::visit(
        overloaded{
            [](const token::Product &) -> std::optional<Op> { return Op{Op::kProduct}; },
            [](const token::Division &) -> std::optional<Op> { return Op{Op::kDivision}; },
            [](const token::Module &) -> std::optional<Op> { return Op{Op::kModule}; },
            [](const auto &) -> std::optional<Op> { return std::nullopt; }},
        it->token);
    if (!op)
      break;

    ++it;
    auto right = parseUnary(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    ExpressionVariant newNode;
    switch (op->kind) {
    case Op::kProduct:
      newNode = Multiplication{
          std::make_unique<ExpressionVariant>(std::move(*left)),
          std::make_unique<ExpressionVariant>(std::move(*right))};
      break;

    case Op::kDivision:
      newNode =
          Division{std::make_unique<ExpressionVariant>(std::move(*left)),
                   std::make_unique<ExpressionVariant>(std::move(*right))};
      break;

    case Op::kModule:
      newNode =
          Remainder{std::make_unique<ExpressionVariant>(std::move(*left)),
                    std::make_unique<ExpressionVariant>(std::move(*right))};
    }

    left = std::move(newNode);
  }
  return left;
}

// term ::= factor ( ('-'|'+') factor )*
std::expected<ExpressionVariant, std::string>
parseTerm(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto left = parseFactor(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end) {
    auto isPlus = std::visit(
        overloaded{
            [](const token::Plus &) -> std::optional<bool> { return true; },
            [](const token::Minus &) -> std::optional<bool> { return false; },
            [](const auto &) -> std::optional<bool> { return std::nullopt; }},
        it->token);
    if (!isPlus.has_value()) {
      break;
    }

    ++it;
    auto right = parseFactor(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    if (isPlus.value()) {
      left = Addition{std::make_unique<ExpressionVariant>(std::move(*left)),
                      std::make_unique<ExpressionVariant>(std::move(*right))};
    } else {
      left =
          Subtraction{std::make_unique<ExpressionVariant>(std::move(*left)),
                      std::make_unique<ExpressionVariant>(std::move(*right))};
    }
  }
  return left;
}

// comparison ::= term ( ('<'|'>'|'<='|'>=') term )*
std::expected<ExpressionVariant, std::string>
parseComparison(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto left = parseTerm(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end) {
    struct Op {
      enum : int {
        kLessThan,
        kLessEqual,
        kGreaterThan,
        kGreaterEqual
      } kind;
    };
    auto op = std::visit(
        overloaded{
            [](const token::Less &) -> std::optional<Op> {
              return Op{Op::kLessThan};
            },
            [](const token::LessEqual &) -> std::optional<Op> {
              return Op{Op::kLessEqual};
            },
            [](const token::Greater &) -> std::optional<Op> {
              return Op{Op::kGreaterThan};
            },
            [](const token::GreaterEqual &) -> std::optional<Op> {
              return Op{Op::kGreaterEqual};
            },
            [](const auto &) -> std::optional<Op> {
              return std::nullopt;
            }
        },
        it->token);
    if (!op.has_value())
      break;

    ++it;
    auto right = parseTerm(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    ExpressionVariant newNode;
    switch (op->kind) {
    case Op::kLessThan:
      newNode =
          LessThan{std::make_unique<ExpressionVariant>(std::move(*left)),
                   std::make_unique<ExpressionVariant>(std::move(*right))};
      break;
    case Op::kLessEqual:
      newNode =
          LessEqual{std::make_unique<ExpressionVariant>(std::move(*left)),
                      std::make_unique<ExpressionVariant>(std::move(*right))};
      break;
    case Op::kGreaterThan:
      newNode =
          GreaterThan{std::make_unique<ExpressionVariant>(std::move(*left)),
                    std::make_unique<ExpressionVariant>(std::move(*right))};
      break;
    case Op::kGreaterEqual:
      newNode =
          GreaterEqual{std::make_unique<ExpressionVariant>(std::move(*left)),
                       std::make_unique<ExpressionVariant>(std::move(*right))};
      break;
    }
    left = std::move(newNode);
  }
  return left;
}

// equality ::= comparison ( ('=='|'!=') comparison )*
std::expected<ExpressionVariant, std::string>
parseEquality(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto left = parseComparison(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end) {
    auto isEqual = std::visit(
        overloaded{
            [](const token::Equal &) -> std::optional<bool> { return true; },
            [](const token::NotEqual &) -> std::optional<bool> { return false; },
            [](const auto &) -> std::optional<bool> { return std::nullopt; }},
        it->token);
    if (!isEqual.has_value())
      break;

    ++it;
    auto right = parseComparison(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    if (isEqual.value())
      left = Equal{std::make_unique<ExpressionVariant>(std::move(*left)),
                   std::make_unique<ExpressionVariant>(std::move(*right))};
    else
      left = NotEqual{std::make_unique<ExpressionVariant>(std::move(*left)),
                      std::make_unique<ExpressionVariant>(std::move(*right))};
  }
  return left;
}

// logical_and ::= equality ( "and" equality )*
std::expected<ExpressionVariant, std::string>
parseLogicalAnd(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto left = parseEquality(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end && std::holds_alternative<token::And>(it->token)) {
    ++it;
    auto right = parseEquality(it, end, pos);
    if (!right)
      return std::unexpected(right.error());
    left = And{std::make_unique<ExpressionVariant>(std::move(*left)),
               std::make_unique<ExpressionVariant>(std::move(*right))};
  }
  return left;
}

// logical_or ::= logical_and ( ("or"|"xor") logical_and )*
std::expected<ExpressionVariant, std::string>
parseLogicalOr(TokenIterator &it, TokenIterator end, Positions &pos) {
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
        it->token);
    if (!op.has_value())
      break;

    ++it;
    auto right = parseLogicalAnd(it, end, pos);
    if (!right)
      return std::unexpected(right.error());

    ExpressionVariant newNode;
    if (op->kind == 0)
      newNode = Or{std::make_unique<ExpressionVariant>(std::move(*left)),
                   std::make_unique<ExpressionVariant>(std::move(*right))};
    else
      newNode = Xor{std::make_unique<ExpressionVariant>(std::move(*left)),
                    std::make_unique<ExpressionVariant>(std::move(*right))};
    left = std::move(newNode);
  }
  return left;
}

// assignment ::= logical_or ( "<-" assignment )?
std::expected<ExpressionVariant, std::string>
parseAssignment(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto left = parseLogicalOr(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  if (it != end && std::holds_alternative<token::Assign>(it->token)) {
    ++it;
    auto right = parseAssignment(it, end, pos);
    if (!right)
      return std::unexpected(right.error());
    return Assign{std::make_unique<ExpressionVariant>(std::move(*left)),
                  std::make_unique<ExpressionVariant>(std::move(*right))};
  }
  return left;
}

// expression ::= assignment
std::expected<ExpressionVariant, std::string>
parseExpression(TokenIterator &it, TokenIterator end, Positions &pos) {
  return parseAssignment(it, end, pos);
}

// primary ::= literal | identifier | '(' expression ')'
std::expected<ExpressionVariant, std::string>
parsePrimary(TokenIterator &it, TokenIterator end, Positions &pos) {
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
      it->token);
}

// postfix ::= primary ( call )*
std::expected<ExpressionVariant, std::string>
parsePostfix(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto left = parsePrimary(it, end, pos);
  if (!left)
    return std::unexpected(left.error());

  while (it != end &&
         std::holds_alternative<token::LeftParenthesis>(it->token)) {
    ++it; // '('
    std::deque<ExpressionVariant> args;
    if (it != end &&
        !std::holds_alternative<token::RightParenthesis>(it->token)) {
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
            it->token);
        if (!more)
          break;
      }
    }
    auto rparen = expectToken<token::RightParenthesis>(it, end);
    if (!rparen)
      return std::unexpected(rparen.error());
    left = Call{std::make_unique<ExpressionVariant>(std::move(*left)),
                std::move(args)};
  }
  return left;
}

// unary ::= ("not" | "+" | "-") unary | postfix
std::expected<ExpressionVariant, std::string>
parseUnary(TokenIterator &it, TokenIterator end, Positions &pos) {
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
                std::make_unique<ExpressionVariant>(std::move(*operand))};
          },
          [&](const token::Plus &)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            auto operand = parseUnary(it, end, pos);
            if (!operand)
              return std::unexpected(operand.error());
            return UnaryPlus{
                std::make_unique<ExpressionVariant>(std::move(*operand))};
          },
          [&](const token::Minus &)
              -> std::expected<ExpressionVariant, std::string> {
            ++it;
            auto operand = parseUnary(it, end, pos);
            if (!operand)
              return std::unexpected(operand.error());
            return UnaryMinus{
                std::make_unique<ExpressionVariant>(std::move(*operand))};
          },
          [&](const auto &) -> std::expected<ExpressionVariant, std::string> {
            return parsePostfix(it, end, pos);
          }},
      it->token);
}

///////////////////////////////////////////////////////////////////////////////

std::expected<StatementVariant, std::string>
parseStatement(TokenIterator &it, TokenIterator end, Positions &pos);

template <typename TokenIterator>
std::expected<Block, std::string> 
parseBlock(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto lbrace = expectToken<token::LeftBrace>(it, end);
  if (!lbrace)
    return std::unexpected(lbrace.error());

  std::deque<StatementVariant> stmts;
  while (it != end && !std::holds_alternative<token::RightBrace>(it->token)) {
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

std::expected<StatementVariant, std::string>
parseReturnStatement(TokenIterator &it, TokenIterator end, Positions &pos) {
  ++it; // 'ret'
  std::optional<ExpressionVariant> value;
  if (it != end && !std::holds_alternative<token::RightBrace>(it->token)) {
    auto expr = parseExpression(it, end, pos);
    if (!expr)
      return std::unexpected(expr.error());
    value = std::move(*expr);
  }
  return StatementVariant{ReturnStatement{std::move(value)}};
}

std::expected<StatementVariant, std::string>
parseIfStatement(TokenIterator &it, TokenIterator end, Positions &pos) {
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

  std::optional<std::unique_ptr<Block>> elseBlock;
  if (it != end && std::holds_alternative<token::Else>(it->token)) {
    ++it;
    auto elseBlk = parseBlock(it, end, pos);
    if (!elseBlk)
      return std::unexpected(elseBlk.error());
    elseBlock = std::make_unique<Block>(std::move(*elseBlk));
  }
  return StatementVariant{
      IfStatement{std::move(*cond),
                  std::make_unique<Block>(std::move(*thenBlock)), 
                  std::move(elseBlock)}};
}

std::expected<StatementVariant, std::string>
parseWhileStatement(TokenIterator &it, TokenIterator end, Positions &pos) {
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
      std::move(*cond), std::make_unique<Block>(std::move(*body))}};
}

std::expected<StatementVariant, std::string>
parseVariableDeclaration(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto name = std::get<token::Identificator>(it->token);
  ++it; // identifier
  ++it; // '<-'
  auto value = parseExpression(it, end, pos);
  if (!value)
    return std::unexpected(value.error());
  return StatementVariant{VariableDeclaration{name.value, std::move(*value)}};
}

std::expected<StatementVariant, std::string>
parseExpressionStatement(TokenIterator &it, TokenIterator end, Positions &pos) {
  auto expr = parseExpression(it, end, pos);
  if (!expr)
    return std::unexpected(expr.error());
  return StatementVariant{ExpressionStatement{std::move(*expr)}};
}

std::expected<StatementVariant, std::string>
parseStatement(TokenIterator &it, TokenIterator end, Positions &pos) {
  if (auto err = ensureTokens(it, end, "statement"); !err)
    return std::unexpected(err.error());

  if (std::holds_alternative<token::Return>(it->token))
    // ++it add there
    return parseReturnStatement(it, end, pos);
  else if (std::holds_alternative<token::If>(it->token))
    return parseIfStatement(it, end, pos);
  else if (std::holds_alternative<token::Cycle>(it->token))
    return parseWhileStatement(it, end, pos);
  else if (std::holds_alternative<token::LeftBrace>(it->token)) {
    auto block = parseBlock(it, end, pos);
    if (!block)
      return std::unexpected(block.error());
    return StatementVariant{std::move(*block)};
  } else if (std::holds_alternative<token::Identificator>(it->token)) {
    auto next = it;
    ++next;
    if (next != end && std::holds_alternative<token::Assign>(next->token))
      return parseVariableDeclaration(it, end, pos);
    else
      return parseExpressionStatement(it, end, pos);
  } else {
    return parseExpressionStatement(it, end, pos);
  }
}

///////////////////////////////////////////////////////////////////////////////

std::expected<FunctionDeclaration, std::string>
parseFunctionDeclaration(TokenIterator &it, TokenIterator end, Positions &pos) {
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
      !std::holds_alternative<token::RightParenthesis>(it->token)) {
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
          it->token);
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

ParseResult parse(const TokenRange &tokens) {
  auto begin = tokens.begin();
  auto end = tokens.end();

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

} // namespace parser