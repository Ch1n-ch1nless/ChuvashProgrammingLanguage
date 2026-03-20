#pragma once

#include <vector>
#include <deque>
#include <memory>
#include <optional>
#include <variant>  // ← добавлено для std::variant

#include <token/tokens.hpp>
#include <utils/type_tuple.hpp>

namespace parser {

struct ExpressionVariant; // ← предварительное объявление

// Load literals and identificators as expressions
using token::IntLiteral;
using token::FltLiteral;
using token::StrLiteral;
using token::Identificator;

using token::Literals;

///////////////////////////////////////////////////////////////////////////////

#define EQUAL_OPERATOR(type) \
friend bool operator==(const type& left, const type& right) = default;

struct BinaryOperationNode {
  std::shared_ptr<ExpressionVariant> left_operand;
  std::shared_ptr<ExpressionVariant> right_operand;

  EQUAL_OPERATOR(BinaryOperationNode)
};

//  ---------------------------< Binary Operators >----------------------------
#define BINARY_OPERATOR(type)\
struct type : BinaryOperationNode {\
  EQUAL_OPERATOR(type) \
};

BINARY_OPERATOR(Addition)       // Plus
BINARY_OPERATOR(Subtraction)    // Minus
BINARY_OPERATOR(Multiplication) // Product
BINARY_OPERATOR(Division)       // Division
BINARY_OPERATOR(Remainder)      // Module
BINARY_OPERATOR(Assign)         // Assign

using BinaryArithmeticOperations = TTuple<Addition, Subtraction, Multiplication, Division, Remainder>;

BINARY_OPERATOR(Equal)        // Equal
BINARY_OPERATOR(NotEqual)     // NotEqual
BINARY_OPERATOR(LessThan)     // Less
BINARY_OPERATOR(LessEqual)    // LessEqual
BINARY_OPERATOR(GreaterThan)  // Greater
BINARY_OPERATOR(GreaterEqual) // GreaterEqual
BINARY_OPERATOR(And)          // And
BINARY_OPERATOR(Or)           // Or
BINARY_OPERATOR(Xor)          // Xor

using BinaryLogicalOperations = TTuple<Equal, NotEqual, LessThan, LessEqual, 
                                       GreaterThan, GreaterEqual, And,
                                       Xor, Or>;

using BinaryOperations = Concat
  < TTuple<Assign>
  , Concat
    < BinaryArithmeticOperations
    , BinaryLogicalOperations
  >::Result
>::Result;

#undef BINARY_OPERATOR

//  ----------------------------< Unary Operators >----------------------------
struct UnaryOperationNode {
  std::shared_ptr<ExpressionVariant> operand;

  EQUAL_OPERATOR(UnaryOperationNode)
};

#define UNARY_OPERATOR(type)\
struct type : UnaryOperationNode {\
  EQUAL_OPERATOR(type) \
};

UNARY_OPERATOR(UnaryPlus);  // Plus
UNARY_OPERATOR(UnaryMinus); // Minus
UNARY_OPERATOR(Not);        // Not


using UnaryOperations = TTuple<Not, UnaryMinus, UnaryPlus>;

#undef UNARY_OPERATOR

// ----------------------------------------------------------------------------
struct Call {
    std::shared_ptr<ExpressionVariant> callee;
    std::deque<ExpressionVariant> arguments;
    EQUAL_OPERATOR(Call)
};

// ---------------------------- < Expressions > -------------------------------
using ExpressionTypes = Concat<
  Literals,
    Concat<
        TTuple<Identificator>,
        Concat<
            BinaryOperations,
            Concat<
                UnaryOperations,
                TTuple<Call>
            >::Result
        >::Result
    >::Result
>::Result;

// Изменено: убран шаблонный конструктор, используется наследование конструкторов
struct ExpressionVariant : TupleToVariant<ExpressionTypes>::Result {
    using Base = TupleToVariant<ExpressionTypes>::Result;
    using Base::Base;
};

// ----------------------------------------------------------------------------
struct Block;
struct StatementVariant;

struct ReturnStatement {
    std::optional<ExpressionVariant> value;
    EQUAL_OPERATOR(ReturnStatement)
};

struct ExpressionStatement {
    ExpressionVariant expression;
    EQUAL_OPERATOR(ExpressionStatement)
};

struct IfStatement {
    ExpressionVariant condition;
    std::shared_ptr<Block> then_block;
    std::optional<std::shared_ptr<Block>> else_block;
    EQUAL_OPERATOR(IfStatement)
};

struct WhileStatement {
    ExpressionVariant condition;
    std::shared_ptr<Block> body;
    EQUAL_OPERATOR(WhileStatement)
};

struct VariableDeclaration {
    std::string name;
    ExpressionVariant value;
    EQUAL_OPERATOR(VariableDeclaration)
};

struct Block {
    std::deque<StatementVariant> statements;
    EQUAL_OPERATOR(Block)
};

using StatementTypes = TTuple<
    ReturnStatement,
    ExpressionStatement,
    IfStatement,
    WhileStatement,
    VariableDeclaration,
    Block
>;

// Изменено: убран шаблонный конструктор, используется наследование конструкторов
struct StatementVariant : std::variant<
    ReturnStatement,
    ExpressionStatement,
    IfStatement,
    WhileStatement,
    VariableDeclaration,
    Block
> {
    using Base = std::variant<
        ReturnStatement,
        ExpressionStatement,
        IfStatement,
        WhileStatement,
        VariableDeclaration,
        Block
    >;
    using Base::Base;
};

// ----------------------------------------------------------------------------
struct FunctionDeclaration {
    std::string name;
    std::vector<std::string> parameters;
    Block body;
    EQUAL_OPERATOR(FunctionDeclaration)
};

using DefinitionTypes = TTuple<FunctionDeclaration>;

// Аналогично для DefinitionVariant
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

template<typename T>
concept Expression = Contains<ExpressionTypes, T>::value;

template<typename T>
concept Statement = Contains<StatementTypes, T>::value;

template<typename T>
concept Definition = Contains<DefinitionTypes, T>::value;

template<typename T>
concept BinaryArithmetic = Contains<BinaryArithmeticOperations, T>::value && Expression<T>;

template<typename T>
concept BinaryLogical = Contains<BinaryLogicalOperations, T>::value && Expression<T>;

template<typename T>
concept Unary = Contains<UnaryOperations, T>::value && Expression<T>;

} // namespace parser