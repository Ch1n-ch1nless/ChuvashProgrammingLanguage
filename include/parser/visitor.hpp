#pragma once

#include "parser/nodes.hpp"
namespace parser::visitor {
// clang-format off

// Class visitor for types.
template <typename Derived>
class BaseTypeVisitor {
 public:
  void visit(const TypeVariant& type) {
    std::visit([this](const auto& node) {
      derived().visitImpl(node);
    }, type);
  }

  template<typename T>
  void dispatch(const T& node) {
    derived().visitImpl(node);
  }

 protected:
  void visitImpl(const BuiltinType&) {}
  void visitImpl(const UserType&) {}

 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

// Class visitor for expressions.
template <typename Derived>
class BaseExpressionVisitor {
 public:
  void visit(const ExpressionVariant& expr) {
    std::visit([this](const auto& node) {
      derived().visitImpl(node);
    }, expr);
  }

 protected:
  template<parser::concepts::IsExpression ExpressionT> 
  void visitImpl(const ExpressionT&) {}
 
 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

// Class visitor for statements.
template <typename Derived>
class BaseStatementVisitor {
 public:
  void visit(const StatementVariant& expr) {
    std::visit([this](const auto& node) {
      derived().visitImpl(node);
    }, expr);
  }

 protected:
  void visitImpl(const ReturnStatement&) {}
  void visitImpl(const ExpressionStatement&) {}
  void visitImpl(const IfStatement&) {}
  void visitImpl(const WhileStatement&) {}
  void visitImpl(const VariableDeclaration&) {}
  void visitImpl(const Parameter&) {}
  void visitImpl(const ScopeStatement&) {}

 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

// Class visitor for definitions.
template <typename Derived>
class BaseDefinitionVisitor {
 public:
  void visit(const DefinitionVariant& def) {
    std::visit([this](const auto& node) {
      derived().visitImpl(node);
    }, def);
  }

 protected:
  void visitImpl(const FunctionDeclaration&) {}

 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

// Class visitor for expressions, statements and definitions.
template <typename Derived>
class BaseVariantVisitor
    : public BaseTypeVisitor<Derived>
    , public BaseExpressionVisitor<Derived>
    , public BaseStatementVisitor<Derived>
    , public BaseDefinitionVisitor<Derived> {
};

// clang-format on
}  // namespace parser::visitor