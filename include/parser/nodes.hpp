#pragma once

#include <deque>
#include <optional>
#include <vector>

#include <parser/types.hpp>
#include <token/tokens.hpp>
#include <utils/boxed.hpp>
#include <utils/type_tuple.hpp>
#include <parser/nodes.hpp>
#include "sema/symbols.hpp"

namespace parser {

struct ExpressionVariant;
using ExprPtr = Boxed<ExpressionVariant>;

// Load literals and identificators as expressions
using token::FltLiteral;
using token::Identificator;
using token::IntLiteral;
using token::Literals;
using token::StrLiteral;

///////////////////////////////////////////////////////////////////////////////

#define EQUAL_OPERATOR(type) \
  friend bool operator==(const type& left, const type& right) = default;

struct BinaryOperationNode {
  ExprPtr left_operand;
  ExprPtr right_operand;
  EQUAL_OPERATOR(BinaryOperationNode)
};

//  ---------------------------< Binary Operators >----------------------------
#define BINARY_OPERATOR(type)         \
  struct type : BinaryOperationNode { \
    EQUAL_OPERATOR(type)              \
  };

BINARY_OPERATOR(Addition)
BINARY_OPERATOR(Subtraction)
BINARY_OPERATOR(Multiplication)
BINARY_OPERATOR(Division)
BINARY_OPERATOR(Remainder)
BINARY_OPERATOR(Assign)

// clang-format off
using BinaryArithmeticOperations =
  TTuple
  < Addition
  , Subtraction
  , Multiplication
  , Division
  , Remainder
  >;
// clang-format on

BINARY_OPERATOR(Equal)
BINARY_OPERATOR(NotEqual)
BINARY_OPERATOR(LessThan)
BINARY_OPERATOR(LessEqual)
BINARY_OPERATOR(GreaterThan)
BINARY_OPERATOR(GreaterEqual)
BINARY_OPERATOR(And)
BINARY_OPERATOR(Or)
BINARY_OPERATOR(Xor)

// clang-format off
using BinaryLogicalOperations = 
  TTuple
  < Equal
  , NotEqual
  , LessThan
  , LessEqual
  , GreaterThan
  , GreaterEqual
  , And
  , Xor
  , Or
  >;

using BinaryOperations =
  Concat
  < TTuple
    < Assign
    >
  , Concat
    < BinaryArithmeticOperations
    , BinaryLogicalOperations
    >::Result
  >::Result;
// clang-format on

#undef BINARY_OPERATOR

//  ----------------------------< Unary Operators >----------------------------
struct UnaryOperationNode {
  ExprPtr operand;
  EQUAL_OPERATOR(UnaryOperationNode)
};

#define UNARY_OPERATOR(type)         \
  struct type : UnaryOperationNode { \
    EQUAL_OPERATOR(type)             \
  };

UNARY_OPERATOR(UnaryPlus)
UNARY_OPERATOR(UnaryMinus)
UNARY_OPERATOR(Not)

using UnaryOperations = TTuple<Not, UnaryMinus, UnaryPlus>;

#undef UNARY_OPERATOR

// ----------------------------------------------------------------------------
struct Call {
  ExprPtr callee;
  std::deque<ExprPtr> arguments;
  EQUAL_OPERATOR(Call)
};

// ---------------------------- < Expressions > -------------------------------
// clang-format off
using ExpressionTypes = 
  Concat
  < Literals
  , Concat
    < TTuple
      < Identificator
      >
    , Concat
      < BinaryOperations
      , Concat
        < UnaryOperations
        , TTuple
          < Call
          >
        >::Result
      >::Result
    >::Result
  >::Result;
//clang-format on

struct ExpressionVariant : TupleToVariant<ExpressionTypes>::Result {
  using Base = TupleToVariant<ExpressionTypes>::Result;
  using Base::Base;
};

// -------------------------------< Statements >-------------------------------
struct StatementVariant;
using StatePtr = Boxed<StatementVariant>;

struct ScopeStatement {
  std::deque<StatePtr> statements;
  EQUAL_OPERATOR(ScopeStatement)
};

struct ReturnStatement {
  std::optional<ExprPtr> value;
  EQUAL_OPERATOR(ReturnStatement)
};

struct ExpressionStatement {
  ExprPtr expression;
  EQUAL_OPERATOR(ExpressionStatement)
};

struct IfStatement {
  ExprPtr condition;
  StatePtr then_branch;
  std::optional<StatePtr> else_branch;
  EQUAL_OPERATOR(IfStatement)
};

struct WhileStatement {
  ExprPtr condition;
  StatePtr body;
  EQUAL_OPERATOR(WhileStatement)
};

struct Parameter {
  std::string name;
  TypeVariant type;
  bool operator==(const Parameter&) const = default;
};

struct VariableDeclaration {
  std::string name;
  TypeVariant type;
  EQUAL_OPERATOR(VariableDeclaration)
};

// clang-format off
using StatementTypes = 
  TTuple
  < ReturnStatement
  , ExpressionStatement
  , IfStatement
  , WhileStatement
  , VariableDeclaration
  , Parameter
  , ScopeStatement
  >;
// clang-format on

struct StatementVariant : TupleToVariant<StatementTypes>::Result {
  using Base = TupleToVariant<StatementTypes>::Result;
  using Base::Base;
};

// ------------------------------< Declarations >------------------------------
struct FunctionDeclaration {
  std::string name;
  std::vector<Parameter> parameters;
  std::optional<TypeVariant> return_type;
  ScopeStatement body;
  EQUAL_OPERATOR(FunctionDeclaration)
};

using DefinitionTypes = TTuple<FunctionDeclaration>;

struct DefinitionVariant : TupleToVariant<DefinitionTypes>::Result {
  using Base = TupleToVariant<DefinitionTypes>::Result;
  using Base::Base;
};

struct Program {
  std::deque<FunctionDeclaration> functions;
  EQUAL_OPERATOR(Program)
};

#undef EQUAL_OPERATOR

///////////////////////////////////////////////////////////////////////////////

namespace concepts {

template <typename T>
concept IsExpression = Contains<ExpressionTypes, T>::value;

template <typename T>
concept IsStatement = Contains<StatementTypes, T>::value;

template <typename T>
concept IsDefinition = Contains<DefinitionTypes, T>::value;

template <typename T>
concept IsBinaryArithmetic =
    Contains<BinaryArithmeticOperations, T>::value && IsExpression<T>;

template <typename T>
concept IsBinaryLogical =
    Contains<BinaryLogicalOperations, T>::value && IsExpression<T>;

template <typename T>
concept IsBinaryOperation = IsBinaryArithmetic<T> || IsBinaryLogical<T>;

template <typename T>
concept IsUnary = Contains<UnaryOperations, T>::value && IsExpression<T>;

} // namespace concepts

}  // namespace parser