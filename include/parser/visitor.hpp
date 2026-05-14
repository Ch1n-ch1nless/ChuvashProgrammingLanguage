#pragma once

#include <cassert>
#include "parser/nodes.hpp"
namespace parser::visitor {
// clang-format off

// Class visitor for types.
// TODO: Add concept for Derived
template <typename RetType, typename Derived>
class BaseTypeVisitor {
 public:
  RetType visit(const TypeVariant& type) {
    return std::visit([this](const auto& node) -> RetType {
      return derived().visitImpl(node);
    }, type);
  }

 protected:
  template <parser::concepts::IsType TypeNodeT>
  RetType visitImpl(const TypeNodeT&) {
    assert(false && "Not implemented method!");
  }

 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

template<typename Derived>
class BaseTypeVisitor<void, Derived> {
 public:
  void visit(const TypeVariant& type) {
    std::visit([this](const auto& node) {
      derived().visitImpl(node);
    }, type);
  }

 protected:
  template <parser::concepts::IsType TypeNodeT>
  void visitImpl(const TypeNodeT&) {
    assert(false && "Not implemented method!");
  }

 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

// Class visitor for expressions.
template <typename RetType, typename Derived>
class BaseExpressionVisitor {
 public:
  RetType visit(const ExpressionVariant& expr) {
    return std::visit([this](const auto& node) -> RetType {
      return derived().visitImpl(node);
    }, expr);
  }

 protected:
  template<parser::concepts::IsExpression ExpressionT> 
  RetType visitImpl(const ExpressionT&) {
    assert(false && "Not implemented method!");
  }
 
 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

template <typename Derived>
class BaseExpressionVisitor<void, Derived> {
 public:
  void visit(const ExpressionVariant& expr) {
    std::visit([this](const auto& node) {
      derived().visitImpl(node);
    }, expr);
  }

 protected:
  template<parser::concepts::IsExpression ExpressionT> 
  void visitImpl(const ExpressionT&) {
    assert(false && "Not implemented method!");
  }
 
 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

// Class visitor for statements.
template <typename RetType, typename Derived>
class BaseStatementVisitor {
 public:
  RetType visit(const StatementVariant& expr) {
    return std::visit([this](const auto& node) {
      return derived().visitImpl(node);
    }, expr);
  }

 protected:
  template <parser::concepts::IsStatement StatementT>
  RetType visitImpl(const StatementT&) {
    assert(false && "Not implemented method!");
  }

 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

template <typename Derived>
class BaseStatementVisitor<void, Derived> {
 public:
  void visit(const StatementVariant& expr) {
    std::visit([this](const auto& node) {
      derived().visitImpl(node);
    }, expr);
  }

 protected:
  template <parser::concepts::IsStatement StatementT>
  void visitImpl(const StatementT&) {
    assert(false && "Not implemented method!");
  }

 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

// Class visitor for definitions.
template <typename RetType, typename Derived>
class BaseDefinitionVisitor {
 public:
  RetType visit(const DefinitionVariant& def) {
    return std::visit([this](const auto& node) {
      return derived().visitImpl(node);
    }, def);
  }

 protected:
  template <parser::concepts::IsDefinition DefinitionT>
  RetType visitImpl(const DefinitionT&) {
    assert(false && "Not implemented method!");
  }

 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

template <typename Derived>
class BaseDefinitionVisitor<void, Derived> {
 public:
  void visit(const DefinitionVariant& def) {
    std::visit([this](const auto& node) {
      derived().visitImpl(node);
    }, def);
  }

 protected:
  template <parser::concepts::IsDefinition DefinitionT>
  void visitImpl(const DefinitionT&) {
    assert(false && "Not implemented method!");
  }

 private:
  Derived& derived() { return static_cast<Derived&>(*this); }
};

// Class visitor for expressions, statements and definitions.
template <typename RetType, typename Derived>
class BaseVariantVisitor
    : public BaseTypeVisitor<RetType, Derived>
    , public BaseExpressionVisitor<RetType, Derived>
    , public BaseStatementVisitor<RetType, Derived>
    , public BaseDefinitionVisitor<RetType, Derived> {
};

// clang-format on
}  // namespace parser::visitor