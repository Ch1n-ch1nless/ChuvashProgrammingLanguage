#include <exception>
#include <iostream>

#include <parser/nodes.hpp>
#include <parser/interpreter.hpp>
#include <utils/overload.hpp>
#include <variant>

namespace parser {
namespace visitor {

namespace runtime {

// ---------- Environment ----------
void Environment::define(const std::string& name, Value value) {
  map_[name] = std::move(value);
}
void Environment::assign(const std::string& name, Value value) {
  auto it = map_.find(name);
  if (it == map_.end()) throw Error("Undefined variable: " + name);
  it->second = std::move(value);
}
Value Environment::get(const std::string& name) const {
  auto it = map_.find(name);
  if (it == map_.end()) throw Error("Undefined variable: " + name);
  return it->second;
}
bool Environment::has(const std::string& name) const {
  return map_.find(name) != map_.end();
}

}  // namespace runtime::

// ---------- InterpretVisitor ----------

// --------------------------------- Literals ---------------------------------
void InterpretVisitor::visitImpl(const IntLiteral& lit) {
  lastResult_ = lit.value;
}
void InterpretVisitor::visitImpl(const FltLiteral& lit) {
  lastResult_ = lit.value;
}
void InterpretVisitor::visitImpl(const StrLiteral& lit) {
  lastResult_ = lit.value;
}

// ------------------------------ Identificator -------------------------------
void InterpretVisitor::visitImpl(const Identificator& id) {
  for (auto it = envStack_.rbegin(); it != envStack_.rend(); ++it) {
    if (it->has(id.value)) {
      lastResult_ = it->get(id.value);
      return;
    }
  }
  lastResult_ = globalEnv_.get(id.value);
}

// ----------------------------- Binary operations ----------------------------
void InterpretVisitor::visitImpl(const Addition& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kAddition);
}

void InterpretVisitor::visitImpl(const Subtraction& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kSubtraction);
}

void InterpretVisitor::visitImpl(const Multiplication& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kMultiplication);
}

void InterpretVisitor::visitImpl(const Division& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kDivision);
}

void InterpretVisitor::visitImpl(const Remainder& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kRemainder);
}

void InterpretVisitor::visitImpl(const Equal& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kEqual);
}

void InterpretVisitor::visitImpl(const NotEqual& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kNotEqual);
}

void InterpretVisitor::visitImpl(const LessThan& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kLessThan);
}

void InterpretVisitor::visitImpl(const LessEqual& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kLessEqual);
}

void InterpretVisitor::visitImpl(const GreaterThan& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kGreaterThan);
}

void InterpretVisitor::visitImpl(const GreaterEqual& op) {
  visit(*op.left_operand);
  runtime::Value left = lastResult_;
  visit(*op.right_operand);
  runtime::Value right = lastResult_;
  applyBinaryOp(left, right, Operation::kGreaterEqual);
}

void InterpretVisitor::visitImpl(const And& op) {
  visit(*op.left_operand);
  bool left = isTrue(lastResult_);
  if (!left) {
    lastResult_ = false;
    return;
  }
  visit(*op.right_operand);
  lastResult_ = isTrue(lastResult_);
}

void InterpretVisitor::visitImpl(const Or& op) {
  visit(*op.left_operand);
  bool left = isTrue(lastResult_);
  if (left) {
    lastResult_ = true;
    return;
  }
  visit(*op.right_operand);
  lastResult_ = isTrue(lastResult_);
}

void InterpretVisitor::visitImpl(const Xor& op) {
  visit(*op.left_operand);
  bool left = isTrue(lastResult_);
  visit(*op.right_operand);
  bool right = isTrue(lastResult_);
  lastResult_ = (left != right);
}

void InterpretVisitor::visitImpl(const Assign& op) {
  if (const auto* id = std::get_if<Identificator>(&*op.left_operand)) {
    visit(*op.right_operand);
    runtime::Value val = lastResult_;
    for (auto it = envStack_.rbegin(); it != envStack_.rend(); ++it) {
      if (it->has(id->value)) {
        it->assign(id->value, val);
        lastResult_ = val;
        return;
      }
    }
    globalEnv_.assign(id->value, val);
    lastResult_ = val;
    return;
  }
  throw runtime::Error("Left side of assignment must be an identifier");
}

// ------------------------------- Unary operations ---------------------------
void InterpretVisitor::visitImpl(const UnaryPlus& op) {
  visit(*op.operand);
  applyUnaryOp(lastResult_, Operation::kUnaryPlus);
}

void InterpretVisitor::visitImpl(const UnaryMinus& op) {
  visit(*op.operand);
  applyUnaryOp(lastResult_, Operation::kUnaryMinus);
}

void InterpretVisitor::visitImpl(const Not& op) {
  visit(*op.operand);
  lastResult_ = !isTrue(lastResult_);
}

// ----------------------------------- Call -----------------------------------
void InterpretVisitor::visitImpl(const Call& call) {
  if (const auto* id = std::get_if<Identificator>(&*call.callee)) {
    std::vector<runtime::Value> args;
    for (const auto& arg : call.arguments) {
      visit(*arg);
      args.push_back(lastResult_);
    }
    callFunction(id->value, args);
    return;
  }
  throw runtime::Error("Can only call functions by name");
}

// ------------------------------- Statements ---------------------------------
void InterpretVisitor::visitImpl(const ReturnStatement& ret) {
  returnFlag_ = true;
  if (ret.value) {
    visit(**ret.value);
    returnValue_ = lastResult_;
  } else {
    returnValue_ = runtime::Unit{};
  }
}

void InterpretVisitor::visitImpl(const ExpressionStatement& exprStmt) {
  visit(*exprStmt.expression);
}

void InterpretVisitor::visitImpl(const IfStatement& ifStmt) {
  visit(*ifStmt.condition);
  if (isTrue(lastResult_)) {
    visit(*ifStmt.then_branch);
  } else if (ifStmt.else_branch) {
    visit(**ifStmt.else_branch);
  }
}

void InterpretVisitor::visitImpl(const WhileStatement& whileStmt) {
  while (true) {
    visit(*whileStmt.condition);
    if (!isTrue(lastResult_)) break;
    visit(*whileStmt.body);
    if (returnFlag_) return;
  }
}

void InterpretVisitor::visitImpl(const VariableDeclaration& varDecl) {
  visit(*varDecl.value);
  if (envStack_.empty())
    globalEnv_.define(varDecl.name, lastResult_);
  else
    envStack_.back().define(varDecl.name, lastResult_);
}

void InterpretVisitor::visitImpl(const ScopeStatement& scope) {
  envStack_.emplace_back();
  for (const auto& stmt : scope.statements) {
    visit(*stmt);
    if (returnFlag_) break;
  }
  envStack_.pop_back();
}

// ------------------------------ Definitions ---------------------------------
void InterpretVisitor::visitImpl([[maybe_unused]]const FunctionDeclaration& func) {
  //TODO: release logic for inner function declaration!
}

// ---------- Helper methods ----------
bool InterpretVisitor::isTrue(const runtime::Value& val) const {
  return std::visit(
      overloaded{[](int v) -> bool { return v != 0; },
                 [](double v) -> bool { return v != 0.0; },
                 [](const std::string& v) -> bool { return !v.empty(); },
                 [](bool v) -> bool { return v; },
                 [](runtime::Unit) -> bool { return false; }},
      val);
}

void InterpretVisitor::applyBinaryOp(const runtime::Value& left,
                                     const runtime::Value& right,
                                     Operation op) {
  lastResult_ = std::visit(
      overloaded{
          [&](int a, int b) -> runtime::Value {
            switch (op) {
              case Operation::kAddition:
                return a + b;
              case Operation::kSubtraction:
                return a - b;
              case Operation::kMultiplication:
                return a * b;
              case Operation::kDivision: {
                if (b == 0) {
                  throw runtime::Error("Division by zero");
                }
                return a / b;
              }
              case Operation::kRemainder:
                return a % b;
              case Operation::kEqual:
                return a == b;
              case Operation::kNotEqual:
                return a != b;
              case Operation::kLessThan:
                return a < b;
              case Operation::kLessEqual:
                return a <= b;
              case Operation::kGreaterThan:
                return a > b;
              case Operation::kGreaterEqual:
                return a >= b;
              default:
                throw runtime::Error("Unsupported operation on Integers");
            }
          },
          [&](double a, double b) -> runtime::Value {
            switch (op) {
              case Operation::kAddition:
                return a + b;
              case Operation::kSubtraction:
                return a - b;
              case Operation::kMultiplication:
                return a * b;
              case Operation::kDivision: {
                if (b == 0) {
                  throw runtime::Error("Division by zero");
                }
                return a / b;
              }
              case Operation::kEqual:
                return a == b;
              case Operation::kNotEqual:
                return a != b;
              case Operation::kLessThan:
                return a < b;
              case Operation::kLessEqual:
                return a <= b;
              case Operation::kGreaterThan:
                return a > b;
              case Operation::kGreaterEqual:
                return a >= b;
              default:
                throw runtime::Error("Unsupported operation on Doubles");
            }
          },
          [&](const std::string& a, const std::string& b) -> runtime::Value {
            switch (op) {
              case Operation::kAddition:
                return a + b;
              case Operation::kEqual:
                return a == b;
              case Operation::kNotEqual:
                return a != b;
              case Operation::kLessThan:
                return a < b;
              case Operation::kLessEqual:
                return a <= b;
              case Operation::kGreaterThan:
                return a > b;
              case Operation::kGreaterEqual:
                return a >= b;
              default:
                throw runtime::Error("Unsupported operation on Strings");
            }
          },
          [&](int a, double b) -> runtime::Value {
            applyBinaryOp(static_cast<double>(a), b, op);
            return lastResult_;
          },
          [&](double a, int b) -> runtime::Value {
            applyBinaryOp(a, static_cast<double>(b), op);
            return lastResult_;
          },
          [&](bool a, bool b) -> runtime::Value {
            switch (op) {
              case Operation::kEqual:
                return a == b;
              case Operation::kNotEqual:
                return a != b;
              default:
                throw runtime::Error("Unsupported operation on Booleans");
            }
          },
          [&](auto, auto) -> runtime::Value {
            throw runtime::Error("Type mismatch in binary operation");
          }},
      left, right);
}

void InterpretVisitor::applyUnaryOp(const runtime::Value& operand,
                                    Operation op) {
  lastResult_ = std::visit(
      overloaded{
          [&](int x) -> runtime::Value {
            switch (op) {
              case Operation::kUnaryPlus:
                return +x;
              case Operation::kUnaryMinus:
                return -x;
              default:
                throw runtime::Error("Unsupported unary op on Integer");
            }
          },
          [&](double x) -> runtime::Value {
            switch (op) {
              case Operation::kUnaryPlus:
                return +x;
              case Operation::kUnaryMinus:
                return -x;
              default:
                throw runtime::Error("Unsupported unary op on Double");
            }
          },
          [&](auto) -> runtime::Value {
            throw runtime::Error("Unary operation only for numeric types");
          }},
      operand);
}

void InterpretVisitor::callFunction(const std::string& name,
                                    const std::vector<runtime::Value>& args) {
  auto it = functions_.find(name);
  if (it == functions_.end())
    throw runtime::Error("Undefined function: " + name);
  const auto& func = it->second;
  if (func.parameters.size() != args.size())
    throw runtime::Error("Argument count mismatch for " + name);

  envStack_.emplace_back();
  for (size_t i = 0; i < args.size(); ++i) {
    envStack_.back().define(func.parameters[i], args[i]);
  }
  returnFlag_ = false;
  visit(StatementVariant{func.body});
  envStack_.pop_back();

  if (returnFlag_)
    lastResult_ = returnValue_;
  else
    lastResult_ = runtime::Unit{};
}

runtime::Value InterpretVisitor::run(const Program& program) {
  for (const auto& func : program.functions) {
      functions_[func.name] = func;
  }
  if (functions_.find("main") == functions_.end())
      throw runtime::Error("No 'main' function defined");
  callFunction("main", {});
  return lastResult_;
}

}  // namespace visitor

ExitCodeT InterpretProgram(const std::pair<Program, Positions>& ast) {
  visitor::InterpretVisitor visitor;
  try {
    visitor::runtime::Value last_result = visitor.run(ast.first);
    std::visit(
      overloaded{
        [](int result) {
          return result;
        },
        [](auto) {
          std::cerr << "Unexpected value!\n";
          return -2;
        }
      },
      last_result
    );
  } catch (const visitor::runtime::Error& error) {
    std::cerr << "Runtime error: " << error.what() << "\n";
    return -1;
  } catch (const std::exception& excertion) {
    std::cerr << "Unexpected error: " << excertion.what() << "\n";
    return -2;
  }
  return 0;
}

}  // namespace parser