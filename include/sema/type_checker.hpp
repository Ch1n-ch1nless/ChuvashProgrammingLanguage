// type_checker.hpp
#pragma once

#include <expected>
#include <parser/nodes.hpp>
#include <parser/visitor.hpp>
#include "sema/symbols.hpp"

namespace parser::sema {

using ErrorMessageT = std::string;
using ExpectedType = std::expected<parser::TypeVariant, ErrorMessageT>;

class TypeChecker : public visitor::BaseVariantVisitor<ExpectedType, TypeChecker> {
private:
  friend class visitor::BaseTypeVisitor<ExpectedType, TypeChecker>;
  friend class visitor::BaseExpressionVisitor<ExpectedType, TypeChecker>;
  friend class visitor::BaseStatementVisitor<ExpectedType, TypeChecker>;
  friend class visitor::BaseDefinitionVisitor<ExpectedType, TypeChecker>;

public:
  TypeChecker(symbols::Scope* global_scope) : current_scope_(global_scope) {}

  using visitor::BaseTypeVisitor<ExpectedType, TypeChecker>::visit;
  using visitor::BaseExpressionVisitor<ExpectedType, TypeChecker>::visit;
  using visitor::BaseStatementVisitor<ExpectedType, TypeChecker>::visit;
  using visitor::BaseDefinitionVisitor<ExpectedType, TypeChecker>::visit;

protected:
  // Types
  ExpectedType visitImpl(const BuiltinType&);
  ExpectedType visitImpl(const UserType&);

  // Literals
  ExpectedType visitImpl(const IntLiteral&);
  ExpectedType visitImpl(const FltLiteral&);
  ExpectedType visitImpl(const StrLiteral&);

  // Identificator
  ExpectedType visitImpl(const Identificator&);

  // Binary operations
  template <parser::concepts::IsBinaryOperation BinaryOpT>
  ExpectedType visitImpl(const BinaryOpT& op) {
      auto left_type = visit(*op.left_operand);
      if (!left_type) return std::unexpected(left_type.error());
      auto right_type = visit(*op.right_operand);
      if (!right_type) return std::unexpected(right_type.error());
      auto op_kind = convertTypeToOperation(op);
      if (type_context_.areBinaryOperandsCompatible(*left_type, *right_type, op_kind)) {
          return type_context_.getBinaryOperationResultType(*left_type, *right_type, op_kind);
      }
      return std::unexpected("Incompatible types for binary operation");
  }

  ExpectedType visitImpl(const Assign& op);

  // Unary operations
  template <parser::concepts::IsUnary UnaryOpT>
  ExpectedType visitImpl(const UnaryOpT& op) {
      auto operand_type = visit(*op.operand);
      if (!operand_type) return std::unexpected(operand_type.error());
      auto op_kind = convertTypeToOperation(op);
      switch (op_kind) {
          case Operation::kUnaryPlus:
          case Operation::kUnaryMinus:
              if (type_context_.isArithmetic(*operand_type))
                  return *operand_type;
              else
                  return std::unexpected("Unary arithmetic operator requires numeric operand");
          case Operation::kNot:
              if (type_context_.isConvertibleToBool(*operand_type))
                  return BuiltinType{BuiltinType::Kind::kBool};
              else
                  return std::unexpected("Logical not requires operand convertible to bool");
          default:
              return std::unexpected("Unknown unary operation");
      }
  }

  // Call
  ExpectedType visitImpl(const Call& call);

  // Statements
  ExpectedType visitImpl(const ReturnStatement& ret);
  ExpectedType visitImpl(const ExpressionStatement& stmt);
  ExpectedType visitImpl(const IfStatement& stmt);
  ExpectedType visitImpl(const WhileStatement& stmt);
  ExpectedType visitImpl(const VariableDeclaration& decl);
  ExpectedType visitImpl(const ScopeStatement& stmt);
  ExpectedType visitImpl(const Parameter& param);

  // Function definition
  ExpectedType visitImpl(const FunctionDeclaration& func_decl);

private:
  enum class Operation {
    kAddition,
    kSubtraction,
    kMultiplication,
    kDivision,
    kRemainder,
    kEqual,
    kNotEqual,
    kLessThan,
    kLessEqual,
    kGreaterThan,
    kGreaterEqual,
    kAnd,
    kOr,
    kXor,
    kAssign,
    kUnaryPlus,
    kUnaryMinus,
    kNot,
  };

  Operation convertTypeToOperation(const parser::ExpressionVariant& op_type);

  class TypeContext {
  public:
    bool isConvertableTo(const TypeVariant& lhs, const TypeVariant& rhs) const;
    bool areBinaryOperandsCompatible(const TypeVariant& left, const TypeVariant& right, Operation op) const;
    TypeVariant getBinaryOperationResultType(const TypeVariant& left, const TypeVariant& right, Operation op) const;
    bool isArithmetic(const TypeVariant& t) const;
    bool isFloat(const TypeVariant& t) const;
    bool isInteger(const TypeVariant& t) const;
    bool isBool(const TypeVariant& t) const;
    bool isConvertibleToBool(const TypeVariant& t) const;
    bool isComparable(const TypeVariant& a, const TypeVariant& b) const;
  };

  symbols::Scope* current_scope_ = nullptr;
  TypeContext type_context_;
};

} // namespace parser::sema