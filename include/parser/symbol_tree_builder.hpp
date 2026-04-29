// symbol_tree_builder.hpp
#pragma once

#include <parser/symbols.hpp>
#include <parser/nodes.hpp>
#include <parser/visitor.hpp>
#include <stdexcept>

namespace parser::visitor {

class SymbolTreeBuilder : public BaseVariantVisitor<void, SymbolTreeBuilder> {
 private:
  friend class BaseTypeVisitor<void, SymbolTreeBuilder>;
  friend class BaseExpressionVisitor<void, SymbolTreeBuilder>;
  friend class BaseStatementVisitor<void, SymbolTreeBuilder>;
  friend class BaseDefinitionVisitor<void, SymbolTreeBuilder>;

 public:
  SymbolTreeBuilder() = default;

  using BaseTypeVisitor<void, SymbolTreeBuilder>::visit;
  using BaseExpressionVisitor<void, SymbolTreeBuilder>::visit;
  using BaseStatementVisitor<void, SymbolTreeBuilder>::visit;
  using BaseDefinitionVisitor<void, SymbolTreeBuilder>::visit;

  void build(const Program& program) {
    global_scope_ = std::make_unique<symbols::Scope>();
    current_scope_ = global_scope_.get();

    addBuiltinTypes();

    for (const auto& func : program.functions) {
      visit(func);
    }

    for (const auto& func : program.functions) {
      ScopeGuard scope_guard(&current_scope_);
      for (const auto& param : func.parameters) {
        visit(param);
      }
      visit(func.body);
    }
  }

  symbols::Scope* getGlobalScope() const { return global_scope_.get(); }

 protected:
  // ---------------------------------- Types -----------------------------------
  void visitImpl(const BuiltinType&) {
    // Built-in types are already registered
  }

  void visitImpl(const UserType& type) {
    ensureTypeExists(type);
  }

  // -------------------------------- Literals ---------------------------------
  void visitImpl(const IntLiteral&) {}
  void visitImpl(const FltLiteral&) {}
  void visitImpl(const StrLiteral&) {}

  // ----------------------------- Identificator ------------------------------
  void visitImpl(const Identificator& id) {
    if (!current_scope_->Lookup(id.value)) {
      throw std::runtime_error("Undefined identifier: " + id.value);
    }
  }

  // ---------------------------- Binary operations ---------------------------
  template <parser::concepts::IsBinaryOperation BinaryOpT>
  void visitImpl(const BinaryOpT& op) {
    visit(*op.left_operand);
    visit(*op.right_operand);
  }

  void visitImpl(const Assign& op) {
    visit(*op.left_operand);
    visit(*op.right_operand);
  }

  // ------------------------------ Unary operations --------------------------
  template <parser::concepts::IsUnary UnaryOpT>
  void visitImpl(const UnaryOpT& op) {
    visit(*op.operand);
  }

  // ---------------------------------- Call ----------------------------------
  void visitImpl(const Call& call) {
    visit(*call.callee);
    for (const auto& arg : call.arguments) {
      visit(*arg);
    }
  }

  // ------------------------------ Statements --------------------------------
  void visitImpl(const ReturnStatement& ret) {
    if (ret.value) visit(**ret.value);
  }

  void visitImpl(const ExpressionStatement& stmt) {
    visit(*stmt.expression);
  }

  void visitImpl(const IfStatement& stmt) {
    visit(*stmt.condition);
    visit(*stmt.then_branch);
    if (stmt.else_branch) visit(**stmt.else_branch);
  }

  void visitImpl(const WhileStatement& stmt) {
    visit(*stmt.condition);
    visit(*stmt.body);
  }

  void visitImpl(const VariableDeclaration& decl) {
    ensureTypeExists(decl.type);
    if (!current_scope_->Insert(decl.name, symbols::SymbolInfo::SymbolKind::kVariable, decl.type)) {
      throw std::runtime_error("Duplicate variable: " + decl.name);
    }
  }

  void visitImpl(const Parameter& param) {
    if (!current_scope_->Insert(param.name, symbols::SymbolInfo::SymbolKind::kVariable, param.type)) {
      throw std::runtime_error("Duplicate parameter: " + param.name);
    }
    ensureTypeExists(param.type);
  }

  void visitImpl(const ScopeStatement& scope) {
    ScopeGuard scope_guard(&current_scope_);
    for (const auto& stmt : scope.statements) {
      visit(*stmt);
    }
  }

  // ----------------------------- Definitions --------------------------------
  void visitImpl(const FunctionDeclaration& func) {
    auto retType = func.return_type.value_or(BuiltinType{BuiltinType::Kind::kUnit});
    if (!current_scope_->Insert(
        func.name,
        symbols::SymbolInfo::SymbolKind::kFunction,
        retType
    )) {
      throw std::runtime_error("Duplicate function: " + func.name);
    }
  }

 private:
  // -------------------------- Symbol table management --------------------------
  std::unique_ptr<symbols::Scope> global_scope_;
  symbols::Scope* current_scope_ = nullptr;

  class ScopeGuard {
   public:
    explicit ScopeGuard(symbols::Scope** scope) : scope_(scope) {
      *scope_ = (*scope_)->AddChild();
    }

    ~ScopeGuard() {
      *scope_ = (*scope_)->parent;
    }

   private:
    symbols::Scope** scope_;
  };

  void addBuiltinTypes() {
    auto add = [this](const std::string& name, BuiltinType::Kind kind) {
      current_scope_->Insert(name, symbols::SymbolInfo::SymbolKind::kType, BuiltinType{kind});
    };
    add("int", BuiltinType::Kind::kInt);
    add("float", BuiltinType::Kind::kFloat);
    add("string", BuiltinType::Kind::kString);
    add("bool", BuiltinType::Kind::kBool);
    add("unit", BuiltinType::Kind::kUnit);
  }

  void ensureTypeExists(const TypeVariant& type) {
    std::visit([this](const auto& t) {
      using T = std::decay_t<decltype(t)>;
      if constexpr (std::is_same_v<T, UserType>) {
        if (!current_scope_->Lookup(t.name)) {
          throw std::runtime_error("Undefined type: " + t.name);
        }
        auto* sym = current_scope_->Lookup(t.name);
        if (sym->kind != symbols::SymbolInfo::SymbolKind::kType) {
          throw std::runtime_error("'" + t.name + "' is not a type");
        }
      }
    }, type);
  }
};

} // namespace parser::visitor