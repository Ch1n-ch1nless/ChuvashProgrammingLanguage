#include <cassert>
#include <expected>
#include <parser/interpreter.hpp>
#include <variant>
#include "parser/nodes.hpp"
#include "utils/overload.hpp"

namespace parser {
namespace visitor {

namespace runtime {

ValueInfo GetDefaultValue(const TypeVariant& type) {
  return std::visit(
      overloaded{
          [](const BuiltinType& type) {
            switch (type.kind) {
              case parser::BuiltinType::Kind::kInt:
                return ValueInfo{0, type};
              case parser::BuiltinType::Kind::kFloat:
                return ValueInfo{0.0, type};
              case parser::BuiltinType::Kind::kString:
                return ValueInfo{"", type};
              case parser::BuiltinType::Kind::kBool:
                return ValueInfo{false, type};
              case parser::BuiltinType::Kind::kUnit:
                return ValueInfo{Unit{}, type};
              default:
                assert(false && "Not implemented!");
            }
          },
          [](const UserType&) {
            assert(false && "Not implemented!");
            return ValueInfo{};
          }
      }, type
  );
}

} // namespace runtime

InterpretVisitor::Operation 
InterpretVisitor::convertTypeToOperation(const ExpressionVariant& op_type) {
  return std::visit(
      overloaded{
          [](const Addition&) {
            return Operation::kAddition;
          },
          [](const Subtraction&) {
            return Operation::kSubtraction;
          },
          [](const Multiplication&) {
            return Operation::kMultiplication;
          },
          [](const Division&) {
            return Operation::kDivision;
          },
          [](const Remainder&) {
            return Operation::kRemainder;
          },
          [](const Equal&) {
            return Operation::kEqual;
          },
          [](const NotEqual&) {
            return Operation::kNotEqual;
          },
          [](const LessThan&) {
            return Operation::kLessThan;
          },
          [](const LessEqual&) {
            return Operation::kLessEqual;
          },
          [](const GreaterThan&) {
            return Operation::kGreaterThan;
          },
          [](const GreaterEqual&) {
            return Operation::kGreaterEqual;
          },
          [](const And&) {
            return Operation::kAnd;
          },
          [](const Or&) {
            return Operation::kOr;
          },
          [](const Xor&) {
            return Operation::kXor;
          },
          [](const UnaryPlus&) {
            return Operation::kUnaryPlus;
          },
          [](const UnaryMinus&) {
            return Operation::kUnaryMinus;
          },
          [](const Not&) {
            return Operation::kNot;
          },
          [](const auto&) {
            assert(false && "Incorrect type for conversion!");
            return Operation::kPoisonOperation;
          },
      }, op_type
  );
}

runtime::ExpectedValue InterpretVisitor::applyUnaryOp(
    const runtime::Value& operand,
    Operation operation
) {
  switch (operation) {
    case Operation::kUnaryMinus:
      return std::visit(
        overloaded{
            [](const int& num) -> runtime::ExpectedValue {
              return -num;
            },
            [](const double& num) -> runtime::ExpectedValue {
              return -num;
            },
            [](const auto&) -> runtime::ExpectedValue {
              return std::unexpected{
                  "Unary minus is not defined for current type"
              };
            }
        },
        operand
      );
    case Operation::kUnaryPlus:
      return std::visit(
        overloaded{
            [](const int& num) -> runtime::ExpectedValue {
              return +num;
            },
            [](const double& num) -> runtime::ExpectedValue {
              return +num;
            },
            [](const auto&) -> runtime::ExpectedValue {
              return std::unexpected{
                  "Unary plus is not defined for current type"
              };
            }
        },
        operand
      );
    case Operation::kNot:
      return std::visit(
          overloaded{
              [](const bool& val) -> runtime::ExpectedValue {
                return !val;
              },
              [](const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"Not is not defined for current type"};
              }
          },
          operand
      );
    
    default:
      return std::unexpected{
          "Unexpected unary operation"
      };
  }
}

runtime::ExpectedValue InterpretVisitor::applyBinaryOp(
    const runtime::Value& left_value,
    const runtime::Value& right_value,
    Operation operation
) {
  switch (operation) {
    case Operation::kAddition:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs+rhs};
              },
              [](const double& lhs, const int& rhs) -> runtime::ExpectedValue {
                double new_val = static_cast<double>(rhs);
                new_val += lhs;
                return runtime::Value{new_val};
              },
              [](const int& lhs, const double& rhs) -> runtime::ExpectedValue {
                double new_val = static_cast<double>(lhs);
                new_val += rhs;
                return runtime::Value{new_val};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs+rhs};
              },
              [](const std::string& lhs, const std::string& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs+rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"Addition error"};
              },
          }, left_value, right_value
      );
    case Operation::kSubtraction:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs-rhs};
              },
              [](const double& lhs, const int& rhs) -> runtime::ExpectedValue {
                double new_val = static_cast<double>(rhs);
                new_val = lhs - new_val;
                return runtime::Value{new_val};
              },
              [](const int& lhs, const double& rhs) -> runtime::ExpectedValue {
                double new_val = static_cast<double>(lhs);
                new_val -= rhs;
                return runtime::Value{new_val};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs-rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"Subtraction error"};
              },
          }, left_value, right_value
      );
    case Operation::kMultiplication:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs*rhs};
              },
              [](const double& lhs, const int& rhs) -> runtime::ExpectedValue {
                double new_val = static_cast<double>(rhs);
                new_val *= lhs;
                return runtime::Value{new_val};
              },
              [](const int& lhs, const double& rhs) -> runtime::ExpectedValue {
                double new_val = static_cast<double>(lhs);
                new_val *= rhs;
                return runtime::Value{new_val};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs*rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"Multiplication error"};
              },
          }, left_value, right_value
      );
    case Operation::kDivision:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return (rhs == 0)
                    ? std::unexpected{"Division by zero"}
                    : runtime::ExpectedValue{lhs/rhs};
              },
              [](const double& lhs, const int& rhs) -> runtime::ExpectedValue {
                if (rhs == 0) {
                  return std::unexpected{"Division by zero"};
                }
                double new_val = static_cast<double>(rhs);
                new_val = lhs / new_val;
                return runtime::Value{new_val};
              },
              [](const int& lhs, const double& rhs) -> runtime::ExpectedValue {
                double new_val = static_cast<double>(lhs);
                if (rhs == 0.0) {
                  return std::unexpected{"Division by zero"};
                }
                new_val /= rhs;
                return runtime::Value{new_val};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return (rhs == 0.0)
                    ? std::unexpected{"Division by zero"}
                    : runtime::ExpectedValue{lhs/rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"Subtraction error"};
              },
          }, left_value, right_value
      );
    case Operation::kRemainder:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                if (rhs == 0) {
                  return std::unexpected{"Remainder by zero"};
                }
                return runtime::Value{lhs % rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"Remainder error"};
              },
          }, left_value, right_value
      );
    case Operation::kEqual:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs==rhs};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs==rhs};
              },
              [](const std::string& lhs, const std::string& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs==rhs};
              },
              [](const bool& lhs, const bool& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs==rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"Equal error"};
              },
          }, left_value, right_value
      );
    case Operation::kNotEqual:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs!=rhs};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs!=rhs};
              },
              [](const std::string& lhs, const std::string& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs!=rhs};
              },
              [](const bool& lhs, const bool& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs!=rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"NotEqual error"};
              },
          }, left_value, right_value
      );
    case Operation::kLessThan:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs<rhs};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs<rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"LessThan error"};
              },
          }, left_value, right_value
      );
    case Operation::kLessEqual:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs<=rhs};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs<=rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"LessEqual error"};
              },
          }, left_value, right_value
      );
    case Operation::kGreaterThan:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs>rhs};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs>rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"GreaterThan error"};
              },
          }, left_value, right_value
      );
    case Operation::kGreaterEqual:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs>=rhs};
              },
              [](const double& lhs, const double& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs>=rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"GreaterEqual error"};
              },
          }, left_value, right_value
      );
    // And and Or are not implemented, because needs to support lazy evaluation
    case Operation::kXor:
      return std::visit(
          overloaded{
              [](const int& lhs, const int& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs^rhs};
              },
              [](const bool& lhs, const bool& rhs) -> runtime::ExpectedValue {
                return runtime::Value{lhs!=rhs};
              },
              [](const auto&, const auto&) -> runtime::ExpectedValue {
                return std::unexpected{"Xor error"};
              },
          }, left_value, right_value
      );
    default: {
      return runtime::ExpectedValue{std::unexpected{"Not defined operation"}};
    }
  };
}

bool InterpretVisitor::isTrue(const runtime::ValueInfo& val) {
    return std::visit(overloaded{
        [](int v) { return v != 0; },
        [](double v) { return v != 0.0; },
        [](const std::string& v) { return !v.empty(); },
        [](bool v) { return v; },
        [](const runtime::Unit&) { return false; }
    }, val.value);
}

runtime::ExpectedValueInfo InterpretVisitor::callFunction(
    const Identificator& function_name,
    std::deque<runtime::ValueInfo> arguments
) {
  auto it = functions_.find(function_name.value);
  if (it == functions_.end()) {
    return std::unexpected{"Function not found: " + function_name.value};
  }
  const auto* func = it->second;

  if (func->parameters.size() != arguments.size()) {
    return std::unexpected{"Argument count mismatch in call to " + function_name.value};
  }

  auto old_return_value = std::move(return_value_);
  return_value_.reset();

  StackFrame frame(stack_);

  for (size_t i = 0; i < func->parameters.size(); ++i) {
    const auto& param = func->parameters[i];
    const auto& arg = arguments[i];

    auto default_param = runtime::GetDefaultValue(param.type);
    if (!arg.isConvertableTo(default_param)) {
      return_value_ = std::move(old_return_value);
      return std::unexpected{"Type mismatch for parameter '" + param.name + "'"};
    }

    auto declare_result = frame.declareVariable(param.name, arg);
    if (!declare_result) {
      return_value_ = std::move(old_return_value);
      return std::unexpected{"Failed to declare parameter: " + declare_result.error()};
    }
  }

  auto body_result = visit(func->body);
  if (!body_result) {
    return_value_ = std::move(old_return_value);
    return std::unexpected{"Error in function body: " + body_result.error()};
  }

  runtime::ExpectedValueInfo result;
  if (return_value_.has_value()) {
    result = *return_value_;
  } else {
    if (func->return_type.has_value()) {
      result = runtime::GetDefaultValue(*func->return_type);
    } else {
      result = runtime::GetDefaultValue(BuiltinType{BuiltinType::Kind::kUnit});
    }
  }

  return_value_ = std::move(old_return_value);
  return result;
}

} // namespace visitor
} // namespace parser
