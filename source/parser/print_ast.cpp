#include <charconv>
#include <cstddef>
#include <iostream>
#include <parser/print_ast.hpp>
#include <string>
#include <variant>

#include "parser/nodes.hpp"
#include "token/tokens.hpp"
#include "utils/overload.hpp"
#include "utils/type_to_string.hpp"

namespace parser {

void printWithIndentation(const std::string& message, std::size_t indentation) {
  std::string tabs(indentation * 2, ' ');
  std::cout << tabs << message;
}

void printExpression(const ExpressionVariant& expression,
                     std::size_t indentation) {
  // Helper function:
  auto printBinaryOp = [&](const auto& op, const std::string& opName,
                           std::size_t indent) {
    printWithIndentation(opName + ":\n", indent);
    printWithIndentation("{\n", indent);
    {
      ++indent;
      printWithIndentation("Left Operand:\n", indent);
      printWithIndentation("{\n", indent);
      { printExpression(*op.left_operand, indent + 1); }
      printWithIndentation("}\n", indent);
      printWithIndentation("Right Operand:\n", indent);
      printWithIndentation("{\n", indent);
      { printExpression(*op.right_operand, indent + 1); }
      printWithIndentation("}\n", indent);
      --indent;
    }
    printWithIndentation("}\n", indent);
  };

  std::visit(
      overloaded{
          [&]<token::Literal LiteralT>(const LiteralT& literal) {
            printWithIndentation(toString<LiteralT>() + ":\n", indentation);
            printWithIndentation("{\n", indentation);
            {
              ++indentation;
              printWithIndentation(std::to_string(literal.value) + "\n",
                                   indentation);
              --indentation;
            }
            printWithIndentation("}\n", indentation);
          },
          [&](const StrLiteral& literal) {
            printWithIndentation(toString<StrLiteral>() + ":\n", indentation);
            printWithIndentation("{\n", indentation);
            {
              ++indentation;
              printWithIndentation(literal.value + "\n", indentation);
              --indentation;
            }
            printWithIndentation("}\n", indentation);
          },

          [&](const Identificator& id) {
            printWithIndentation(toString<Identificator>() + "\n", indentation);
            printWithIndentation("{\n", indentation);
            {
              ++indentation;
              printWithIndentation(id.value + "\n", indentation);
              --indentation;
            }
            printWithIndentation("}\n", indentation);
          },

          [&]<BinaryArithmetic ArithmeticOperationT>(
              const ArithmeticOperationT& op) {
            printBinaryOp(op, toString<ArithmeticOperationT>(), indentation);
          },

          [&]<BinaryLogical LogicalOperationT>(const LogicalOperationT& op) {
            printBinaryOp(op, toString<LogicalOperationT>(), indentation);
          },

          [&](const Assign& op) {
            printWithIndentation(toString<Assign>() + ":\n", indentation);
            printWithIndentation("{\n", indentation);
            {
              ++indentation;
              printWithIndentation("Left Value\n", indentation);
              printWithIndentation("{\n", indentation);
              { printExpression(*op.left_operand, indentation + 1); }
              printWithIndentation("}\n", indentation);
              printWithIndentation("Right Value:\n", indentation);
              printWithIndentation("{\n", indentation);
              { printExpression(*op.right_operand, indentation + 1); }
              printWithIndentation("}\n", indentation);
              --indentation;
            }
            printWithIndentation("}\n", indentation);
          },

          [&]<Unary UnaryOperationT>(const UnaryOperationT& op) {
            printWithIndentation(toString<UnaryOperationT>() + ":\n",
                                 indentation);
            printWithIndentation("{\n", indentation);
            {
              ++indentation;
              printWithIndentation("Operand:\n", indentation);
              printWithIndentation("{\n", indentation);
              { printExpression(*op.operand, indentation + 1); }
              printWithIndentation("}\n", indentation);
              --indentation;
            }
            printWithIndentation("}\n", indentation);
          },

          [&](const parser::Call& call) {
            printWithIndentation("Call:\n", indentation);
            printWithIndentation("{\n", indentation);
            {
              ++indentation;
              printWithIndentation("Callee:\n", indentation);
              printWithIndentation("(\n", indentation);
              {
                ++indentation;
                printExpression(*call.callee, indentation);
                --indentation;
              }
              printWithIndentation("}\n", indentation);

              printWithIndentation("Arguments:\n", indentation);
              printWithIndentation("(\n", indentation);
              {
                ++indentation;
                for (const auto& arg : call.arguments) {
                  printExpression(*arg, indentation);
                }
                --indentation;
              }
              printWithIndentation(")\n", indentation);
              --indentation;
            }
            printWithIndentation("}\n", indentation);
          }},
      expression);
}

void printStatement(const StatementVariant& statement,
                    std::size_t indentation) {
  std::visit(overloaded{
                 [&](const ScopeStatement& block) {
                   printWithIndentation("Scope:\n", indentation);
                   printWithIndentation("{\n", indentation);
                   for (const auto& stmt : block.statements) {
                     printStatement(*stmt, indentation + 1);
                   }
                   printWithIndentation("}\n", indentation);
                 },

                 [&](const ReturnStatement& ret) {
                   printWithIndentation("Return:\n", indentation);
                   printWithIndentation("{\n", indentation);
                   {
                     ++indentation;
                     if (ret.value.has_value()) {
                       printWithIndentation("Value:\n", indentation);
                       printWithIndentation("{\n", indentation);
                       {
                         ++indentation;
                         printExpression(**ret.value, indentation);
                         --indentation;
                       }
                       printWithIndentation("}\n", indentation);
                     } else {
                       printWithIndentation("Value: (none)\n", indentation);
                     }
                     --indentation;
                   }
                   printWithIndentation("}\n", indentation);
                 },

                 [&](const ExpressionStatement& expr) {
                   printWithIndentation("Expression:\n", indentation);
                   printWithIndentation("{\n", indentation);
                   {
                     ++indentation;
                     printExpression(*expr.expression, indentation);
                     --indentation;
                   }
                   printWithIndentation("}\n", indentation);
                 },

                 [&](const IfStatement& ifStmt) {
                   printWithIndentation("If\n", indentation);
                   printWithIndentation("{\n", indentation);
                   {
                     ++indentation;
                     printWithIndentation("Condition:\n", indentation);
                     printWithIndentation("{\n", indentation);
                     {
                       ++indentation;
                       printExpression(*ifStmt.condition, indentation);
                       --indentation;
                     }
                     printWithIndentation("}\n", indentation);

                     printWithIndentation("Then:\n", indentation);
                     printWithIndentation("{\n", indentation);
                     {
                       ++indentation;
                       printStatement(*ifStmt.then_branch, indentation);
                       --indentation;
                     }
                     printWithIndentation("}\n", indentation);

                     if (ifStmt.else_branch) {
                       printWithIndentation("Else:\n", indentation);
                       printWithIndentation("{\n", indentation);
                       {
                         ++indentation;
                         printStatement(**ifStmt.else_branch, indentation);
                         --indentation;
                       }
                       printWithIndentation("}\n", indentation);
                     }
                     --indentation;
                   }
                   printWithIndentation("}\n", indentation);
                 },

                 [&](const WhileStatement& whileStmt) {
                   printWithIndentation("While\n", indentation);
                   printWithIndentation("{\n", indentation);
                   {
                     ++indentation;
                     printWithIndentation("Condition:\n", indentation);
                     printWithIndentation("{\n", indentation);
                     {
                       ++indentation;
                       printExpression(*whileStmt.condition, indentation);
                       --indentation;
                     }
                     printWithIndentation("}\n", indentation);

                     printWithIndentation("Body:\n", indentation);
                     printWithIndentation("{\n", indentation);
                     {
                       ++indentation;
                       printStatement(*whileStmt.body, indentation);
                       --indentation;
                     }
                     printWithIndentation("}\n", indentation);
                     --indentation;
                   }
                   printWithIndentation("}\n", indentation);
                 },

                 [&](const VariableDeclaration& varDecl) {
                   printWithIndentation("Declaration:\n", indentation);
                   printWithIndentation("{\n", indentation);
                   {
                     ++indentation;
                     printWithIndentation("Variable:\n", indentation);
                     printWithIndentation("{\n", indentation);
                     {
                       ++indentation;
                       printWithIndentation(varDecl.name + "\n", indentation);
                       --indentation;
                     }
                     printWithIndentation("}\n", indentation);

                     printWithIndentation("Value:\n", indentation);
                     printWithIndentation("{\n", indentation);
                     {
                       ++indentation;
                       printExpression(*varDecl.value, indentation);
                       --indentation;
                     }
                     printWithIndentation("}\n", indentation);
                     --indentation;
                   }
                   printWithIndentation("}\n", indentation);
                 },
             },
             statement);
}

void printFunctionDeclaration(const FunctionDeclaration& function,
                              std::size_t indentation) {
  printWithIndentation("Function:\n", indentation);
  printWithIndentation("{\n", indentation);
  {
    ++indentation;
    printWithIndentation("Name:\n", indentation);
    printWithIndentation("{\n", indentation);
    {
      ++indentation;
      printWithIndentation(function.name + "\n", indentation);
      --indentation;
    }
    printWithIndentation("}\n", indentation);

    printWithIndentation("Parameters:\n", indentation);
    printWithIndentation("(\n", indentation);
    {
      ++indentation;
      for (const auto& param : function.parameters) {
        printWithIndentation(param + "\n", indentation);
      }
      --indentation;
    }
    printWithIndentation(")\n", indentation);

    printStatement(StatementVariant{function.body}, indentation);

    --indentation;
  }
  printWithIndentation("}\n", indentation);
}

void printAST(const std::pair<Program, Positions>& ast) {
  std::size_t indentation = 0u;

  printWithIndentation("Abstract Syntax Tree:\n", indentation);
  printWithIndentation("{\n", indentation);

  const auto& functions = ast.first.functions;
  for (const auto& function : functions) {
    ++indentation;
    printFunctionDeclaration(function, indentation);
    --indentation;
  }

  printWithIndentation("}\n", indentation);
}

}  // namespace parser