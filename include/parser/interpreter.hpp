// interpreter.hpp
#pragma once

#include <cstddef>
#include <unordered_map>
#include <vector>
#include <string>
#include <variant>
#include <stdexcept>

#include <parser/parser.hpp>
#include <parser/nodes.hpp>
#include <parser/visitor.hpp>

namespace parser {

using ExitCodeT = int;
ExitCodeT InterpretProgram(const std::pair<Program, Positions>& ast);

namespace visitor {

namespace runtime {

struct Unit {};
using Value = std::variant<int, double, std::string, bool, Unit>;

class Error : public std::runtime_error {
 public:
  explicit Error(const std::string& msg) : std::runtime_error(msg) {}
};

class Environment {
 public:
  void define(const std::string& name, Value value);
  void assign(const std::string& name, Value value);
  Value get(const std::string& name) const;
  bool has(const std::string& name) const;
 private:
  std::unordered_map<std::string, Value> map_;
};

} // namespace runtime

class InterpretVisitor : public BaseVariantVisitor<InterpretVisitor> {
  friend class BaseExpressionVisitor<InterpretVisitor>;
  friend class BaseStatementVisitor<InterpretVisitor>;
  friend class BaseDefinitionVisitor<InterpretVisitor>;
  friend ExitCodeT InterpretProgram(const std::pair<Program, Positions>& ast);

 private:
  enum class Operation : size_t {
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
    kOp,
    kXor,
    kAssign,
    kUnaryPlus,
    kUnaryMinus,
    kNot,
  };

 public:
  explicit InterpretVisitor() = default;

  using BaseExpressionVisitor<InterpretVisitor>::visit;
  using BaseStatementVisitor<InterpretVisitor>::visit;
  using BaseDefinitionVisitor<InterpretVisitor>::visit;

 protected:
  // -------------------------------- Literals --------------------------------
  void visitImpl(const IntLiteral& lit);
  void visitImpl(const FltLiteral& lit);
  void visitImpl(const StrLiteral& lit);

  // ----------------------------- Identificator ------------------------------
  void visitImpl(const Identificator& id);

  // ---------------------------- Binary operations ---------------------------
  void visitImpl(const Addition& op);
  void visitImpl(const Subtraction& op);
  void visitImpl(const Multiplication& op);
  void visitImpl(const Division& op);
  void visitImpl(const Remainder& op);
  void visitImpl(const Equal& op);
  void visitImpl(const NotEqual& op);
  void visitImpl(const LessThan& op);
  void visitImpl(const LessEqual& op);
  void visitImpl(const GreaterThan& op);
  void visitImpl(const GreaterEqual& op);
  void visitImpl(const And& op);
  void visitImpl(const Or& op);
  void visitImpl(const Xor& op);
  void visitImpl(const Assign& op);

  // ------------------------------ Unary operations --------------------------
  void visitImpl(const UnaryPlus& op);
  void visitImpl(const UnaryMinus& op);
  void visitImpl(const Not& op);

  // ---------------------------------- Call ----------------------------------
  void visitImpl(const Call& call);

  // ------------------------------ Statements --------------------------------
  void visitImpl(const ReturnStatement& ret);
  void visitImpl(const ExpressionStatement& exprStmt);
  void visitImpl(const IfStatement& ifStmt);
  void visitImpl(const WhileStatement& whileStmt);
  void visitImpl(const VariableDeclaration& varDecl);
  void visitImpl(const ScopeStatement& scope);

  // ----------------------------- Definitions --------------------------------
  void visitImpl(const FunctionDeclaration& func);

private:
  runtime::Environment globalEnv_;
  std::vector<runtime::Environment> envStack_;
  std::unordered_map<std::string, FunctionDeclaration> functions_;
  bool returnFlag_{false};
  runtime::Value returnValue_;
  runtime::Value lastResult_;

  bool isTrue(const runtime::Value& val) const;
  void applyBinaryOp(const runtime::Value& left, const runtime::Value& right, Operation op);
  void applyUnaryOp(const runtime::Value& operand, Operation op);
  void callFunction(const std::string& name, const std::vector<runtime::Value>& args);

 public:
  runtime::Value run(const Program& program);
};

} // namespace visitor

} // namespace parser
