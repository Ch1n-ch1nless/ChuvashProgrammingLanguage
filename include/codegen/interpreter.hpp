// interpreter.hpp
#pragma once

#include <cassert>
#include <cstddef>
#include <deque>
#include <expected>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>

#include <parser/parser.hpp>
#include "parser/nodes.hpp"
#include "parser/visitor.hpp"
#include "token/tokens.hpp"
#include "utils/overload.hpp"

namespace codegen::interpreter {

// Runtime structs
namespace runtime {

struct Unit {};
// TODO: Needs to support the user-defined types.
using Value = std::variant<int, double, std::string, bool, Unit>;

struct ValueInfo {
  Value value;
  parser::TypeVariant type;

  bool isConvertableTo(const ValueInfo& other) const {
    return std::visit(
        overloaded{
            [](const parser::BuiltinType& left, const parser::BuiltinType& right) {
              return left.kind == right.kind;
            },
            [](auto, auto) {
              // TODO: Add support of user-defined structures
              return false;
            }
        }, type, other.type
    );
  }
};

ValueInfo GetDefaultValue(const parser::TypeVariant& type);

struct VariableInfo {
  std::string name;
  ValueInfo value_info;
};

using ErrorMessage = std::string;
using ExpectedValueInfo = std::expected<ValueInfo, ErrorMessage>;
using ExpectedValue = std::expected<Value, ErrorMessage>;

} // namespace runtime

class InterpretVisitor
    : public parser::visitor::BaseTypeVisitor<runtime::ExpectedValueInfo, InterpretVisitor>
    , public parser::visitor::BaseExpressionVisitor<runtime::ExpectedValueInfo, InterpretVisitor>
    , public parser::visitor::BaseStatementVisitor<runtime::ExpectedValueInfo, InterpretVisitor>
    , public parser::visitor::BaseDefinitionVisitor<void, InterpretVisitor> {

  // Friends declarations:
  friend parser::visitor::BaseTypeVisitor<runtime::ExpectedValueInfo, InterpretVisitor>;
  friend parser::visitor::BaseExpressionVisitor<runtime::ExpectedValueInfo, InterpretVisitor>;
  friend parser::visitor::BaseStatementVisitor<runtime::ExpectedValueInfo, InterpretVisitor>;
  friend parser::visitor::BaseDefinitionVisitor<void, InterpretVisitor>;

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
    kOr,
    kXor,
    kAssign,
    kUnaryPlus,
    kUnaryMinus,
    kNot,

    kPoisonOperation
  };

 public:
  explicit InterpretVisitor() = default;

  using parser::visitor::BaseTypeVisitor<runtime::ExpectedValueInfo, InterpretVisitor>::visit;
  using parser::visitor::BaseExpressionVisitor<runtime::ExpectedValueInfo, InterpretVisitor>::visit;
  using parser::visitor::BaseStatementVisitor<runtime::ExpectedValueInfo, InterpretVisitor>::visit;
  using parser::visitor::BaseDefinitionVisitor<void, InterpretVisitor>::visit;

 public:
  // Main method to start interpretation
  runtime::ExpectedValueInfo interpret(const parser::Program& program) {
    for (const auto& func : program.functions) {
      functions_[func.name] = std::addressof(func);
    }
    
    auto main_it = functions_.find("main");
    if (main_it == functions_.end()) {
      return std::unexpected{"No 'main' function found"};
    }
    
    return callFunction(parser::Identificator{"main"}, {});
  }

 protected:
  // --------------------------------- Types ----------------------------------
  runtime::ExpectedValueInfo visitImpl(const parser::BuiltinType& type_node) {
    return runtime::GetDefaultValue(type_node);
  }

  runtime::ExpectedValueInfo visitImpl(const parser::UserType&) {
    return std::unexpected{"Not support!"};
  }

  // ------------------------------- Literals --------------------------------
  runtime::ExpectedValueInfo visitImpl(const token::IntLiteral literal) {
    return runtime::ValueInfo{
        literal.value,
        parser::BuiltinType{parser::BuiltinType::Kind::kInt}
    };
  }

  runtime::ExpectedValueInfo visitImpl(const token::FltLiteral literal) {
    return runtime::ValueInfo{
        literal.value,
        parser::BuiltinType{parser::BuiltinType::Kind::kFloat}
    };
  }

  runtime::ExpectedValueInfo visitImpl(const token::StrLiteral literal) {
    return runtime::ValueInfo{
        literal.value,
        parser::BuiltinType{parser::BuiltinType::Kind::kString}
    };
  }

  // ----------------------------- Identificator ------------------------------
  runtime::ExpectedValueInfo visitImpl(const parser::Identificator& id) {
    StackFrame* current_frame = getCurrentFrame();
    return current_frame->getVariable(id.value);
  }
  
  // ---------------------------- BinaryOperations ----------------------------
  template <parser::concepts::IsBinaryOperation BinaryOperationT>
  runtime::ExpectedValueInfo visitImpl(const BinaryOperationT& op) {
    auto left_operand = visit(*op.left_operand);
    if (left_operand.has_value() == false) {
      return std::unexpected{"Left operand error: " + left_operand.error()};
    }

    auto right_operand = visit(*op.right_operand);
    if (right_operand.has_value() == false) {
      return std::unexpected{"Right operand error: " + right_operand.error()};
    }

    if (left_operand->isConvertableTo(*right_operand) == false &&
        right_operand->isConvertableTo(*left_operand) == false ) {
      return std::unexpected{"Type incompatibility"};
    }

    parser::TypeVariant new_type;
    auto op_kind = convertTypeToOperation(op);
    bool is_relational = (op_kind >= Operation::kEqual && op_kind <= Operation::kGreaterEqual);
    bool is_logical = (op_kind == Operation::kAnd || op_kind == Operation::kOr || op_kind == Operation::kXor);
    if (is_relational || is_logical) {
        new_type = parser::BuiltinType{parser::BuiltinType::Kind::kBool};
    } else {
        new_type = left_operand->isConvertableTo(*right_operand) ? right_operand->type : left_operand->type;
    }
    
    auto new_value = applyBinaryOp(
        left_operand->value,
        right_operand->value,
        convertTypeToOperation(op)
    );

    if (new_value.has_value() == false) {
      return std::unexpected{new_value.error()};
    }

    return runtime::ValueInfo{*new_value, new_type};
  }

  runtime::ExpectedValueInfo visitImpl(const parser::And& op) {
    auto left_operand = visit(*op.left_operand);
    if (left_operand.has_value() == false) {
      return std::unexpected{"Left operand error: " + left_operand.error()};
    }

    if (isConvertableToBool(*left_operand) == false) {
      return std::unexpected{"Left operand is not convertable to bool"};
    }

    if (!isTrue(*left_operand)) {
      return runtime::ValueInfo{false, parser::BuiltinType{parser::BuiltinType::Kind::kBool}};
    }

    auto right_operand = visit(*op.right_operand);
    if (right_operand.has_value() == false) {
      return std::unexpected{"Right operand error: " + right_operand.error()};
    }

    if (isConvertableToBool(*right_operand) == false) {
      return std::unexpected{"Right operand is not convertable to bool"};
    }

    return runtime::ValueInfo{isTrue(*right_operand), parser::BuiltinType{parser::BuiltinType::Kind::kBool}};
  }

  runtime::ExpectedValueInfo visitImpl(const parser::Or& op) {
    auto left_operand = visit(*op.left_operand);
    if (left_operand.has_value() == false) {
      return std::unexpected{"Left operand error: " + left_operand.error()};
    }

    if (isConvertableToBool(*left_operand) == false) {
      return std::unexpected{"Left operand is not convertable to bool"};
    }

    if (isTrue(*left_operand)) {
      return runtime::ValueInfo{true, parser::BuiltinType{parser::BuiltinType::Kind::kBool}};
    }

    auto right_operand = visit(*op.right_operand);
    if (right_operand.has_value() == false) {
      return std::unexpected{"Right operand error: " + right_operand.error()};
    }

    if (isConvertableToBool(*right_operand) == false) {
      return std::unexpected{"Right operand is not convertable to bool"};
    }

    return runtime::ValueInfo{isTrue(*right_operand), parser::BuiltinType{parser::BuiltinType::Kind::kBool}};
  }

  // ------------------------------- Assignment -------------------------------
  runtime::ExpectedValueInfo visitImpl(const parser::Assign& op) {
    if (const auto& id = std::get_if<parser::Identificator>(&(*op.left_operand))) {
      auto right_operand = visit(*op.right_operand);
      if (right_operand.has_value() == false) {
        return std::unexpected{"Right operand error: " + right_operand.error()};
      }

      StackFrame* current_frame = getCurrentFrame();
      auto set_result = current_frame->setVariable(id->value, *right_operand);
      if (set_result.has_value() == false) {
        return std::unexpected{"Assignment error: " + set_result.error()};
      }
      return *set_result;
    }
    return std::unexpected{"Left operand of assignment must be an identificator!"};
  }

  // ---------------------------- UnaryOperations -----------------------------
  template <parser::concepts::IsUnary UnaryOperationT>
  runtime::ExpectedValueInfo visitImpl(const UnaryOperationT& op) {
    auto operand = visit(*op.operand);
    if (operand.has_value() == false) {
      return std::unexpected{"Operand error: " + operand.error()};
    }

    auto operation = convertTypeToOperation(op);

    auto new_value = applyUnaryOp(operand->value, operation);
    if (new_value.has_value() == false) {
      return std::unexpected{new_value.error()};
    }

    return runtime::ValueInfo{*new_value, operand->type};
  }

  // ---------------------------------- Call ----------------------------------
  runtime::ExpectedValueInfo visitImpl(const parser::Call& call) {
    if (const auto* id = 
        std::get_if<parser::Identificator>(&(*call.callee))) {

      std::deque<runtime::ValueInfo> arguments;

      for (auto& arg_ptr : call.arguments) {
        auto arg_value = visit(*arg_ptr);
        if (arg_value.has_value() == false) {
          return std::unexpected{
              "Error in call argument: " + arg_value.error()
          };
        }
        arguments.push_back(std::move(*arg_value));
      }

      return callFunction(*id, arguments);
    }
    return std::unexpected{"Callee is not Identificator!"};
  }

  // ---------------------------- Statements and Definitions ----------------------------
  runtime::ExpectedValueInfo visitImpl(const parser::Parameter&) {
    return std::unexpected{"Not support!"};
  }


  runtime::ExpectedValueInfo visitImpl(const parser::ScopeStatement& scope) {
    StackFrame scope_frame(stack_);
    for (const auto& stmt : scope.statements) {
      auto res = visit(*stmt);
      if (!res) return std::unexpected(res.error());
      if (return_value_.has_value()) return *return_value_;
    }
    return runtime::GetDefaultValue(parser::BuiltinType{parser::BuiltinType::Kind::kUnit});
  }

  runtime::ExpectedValueInfo visitImpl(const parser::ReturnStatement& return_stmt) {
    if (return_stmt.value.has_value()) {
      auto val = visit(**return_stmt.value);
      if (!val) return std::unexpected(val.error());
      return_value_ = *val;
    } else {
      return_value_ = runtime::GetDefaultValue(parser::BuiltinType{parser::BuiltinType::Kind::kUnit});
    }
    return *return_value_;
}

  runtime::ExpectedValueInfo visitImpl(const parser::ExpressionStatement& expr_stmt) {
    return visit(*expr_stmt.expression);
  }

  runtime::ExpectedValueInfo visitImpl(const parser::IfStatement& if_stmt) {
    auto cond = visit(*if_stmt.condition);
    if (!cond) return std::unexpected(cond.error());
    if (!isConvertableToBool(*cond)) return std::unexpected("condition not bool");
    if (isTrue(*cond)) {
      auto res = visit(*if_stmt.then_branch);
      if (return_value_.has_value()) return *return_value_;
      return res;
    } else if (if_stmt.else_branch.has_value()) {
      auto res = visit(**if_stmt.else_branch);
      if (return_value_.has_value()) return *return_value_;
      return res;
    }
    return runtime::GetDefaultValue(parser::BuiltinType{parser::BuiltinType::Kind::kUnit});
  }

  runtime::ExpectedValueInfo visitImpl(const parser::WhileStatement& while_stmt) {
    while (true) {
      auto cond = visit(*while_stmt.condition);
      if (!cond) return std::unexpected(cond.error());
      if (!isConvertableToBool(*cond)) return std::unexpected("condition not bool");
      if (!isTrue(*cond)) break;
      auto res = visit(*while_stmt.body);
      if (return_value_.has_value()) return *return_value_;
    }
    return runtime::GetDefaultValue(parser::BuiltinType{parser::BuiltinType::Kind::kUnit});
  }

  runtime::ExpectedValueInfo visitImpl(const parser::VariableDeclaration& var_decl) {
    StackFrame* current_frame = getCurrentFrame();
    auto default_value = runtime::GetDefaultValue(var_decl.type);
    auto declare_result = current_frame->declareVariable(var_decl.name, default_value);
    if (declare_result.has_value() == false) {
      return std::unexpected{"Variable declaration error: " + declare_result.error()};
    }
    return *declare_result;
  }

  // --------------------------- Function Definition --------------------------
  void visitImpl(const parser::FunctionDeclaration&) {
    // TODO: Implement function definition handling
  }

 private:
  // Internal methods of InterpretVisitor:
  runtime::ExpectedValue applyBinaryOp(
      const runtime::Value& left_value,
      const runtime::Value& right_value,
      Operation operation
  );

  runtime::ExpectedValue applyUnaryOp(
      const runtime::Value& operand,
      Operation operation
  );

  runtime::ExpectedValueInfo callFunction(
      const parser::Identificator& function_name,
      std::deque<runtime::ValueInfo> arguments
  );

  Operation convertTypeToOperation(const parser::ExpressionVariant& op_type);

  bool isConvertableToBool(const runtime::ValueInfo& value_info) {
    return value_info.isConvertableTo(
        runtime::GetDefaultValue(parser::BuiltinType{parser::BuiltinType::Kind::kBool})
    );
  }
  
  bool isTrue(const runtime::ValueInfo& value_info);

 private:

  // ==========================================================================
  // Stack for interpretation
  // ==========================================================================
  class StackFrame {
   public:
    explicit StackFrame(std::deque<StackFrame*>& stack) : stack_(stack) {
      stack_.push_back(this);
    }

    ~StackFrame() {
      assert(!stack_.empty() && "The data was stolen from the stack frame!");
      stack_.pop_back();
    }

    runtime::ExpectedValueInfo getVariable(const std::string& name) {
      for (auto frame_ptr = stack_.rbegin(); frame_ptr != stack_.rend(); ++frame_ptr) {
        auto& variables = (*frame_ptr)->variables_;
        auto var_it = variables.find(name);
        if (var_it != variables.end()) {
          return var_it->second;
        }
      }
      return std::unexpected{"Undefined variable: " + name};
    }

    runtime::ExpectedValueInfo setVariable(const std::string& name, const runtime::ValueInfo& value_info) {
      for (auto frame_ptr = stack_.rbegin(); frame_ptr != stack_.rend(); ++frame_ptr) {
        auto& variables = (*frame_ptr)->variables_;
        auto var_it = variables.find(name);
        if (var_it != variables.end()) {
          var_it->second = value_info;
          return value_info;
        }
      }
      return std::unexpected{"Undefined variable: " + name};
    }

    runtime::ExpectedValueInfo declareVariable(const std::string& name, const runtime::ValueInfo& value_info) {
      if (variables_.find(name) != variables_.end()) {
        return std::unexpected{"Variable already declared: " + name};
      }
      variables_[name] = value_info;
      return value_info;
    }

   private:
    std::unordered_map<std::string, runtime::ValueInfo> variables_;
    std::deque<StackFrame*>& stack_;
  };

  std::deque<StackFrame*> stack_;

  StackFrame* getCurrentFrame() {
    assert(!stack_.empty() && "Stack is empty!");
    return stack_.back();
  };

  // ==========================================================================
  // Function registry
  // ==========================================================================
  std::unordered_map<std::string, const parser::FunctionDeclaration*> functions_;
  std::optional<runtime::ValueInfo> return_value_;
  
};

} // namespace codegen::interpreter