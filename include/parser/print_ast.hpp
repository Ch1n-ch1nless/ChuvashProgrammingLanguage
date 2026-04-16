// print_ast.hpp
#pragma once

#include <cstddef>
#include <iostream>
#include <parser/parser.hpp>
#include <string>
#include <utility>

#include "parser/nodes.hpp"
#include "parser/visitor.hpp"
#include "token/tokens.hpp"
#include "utils/type_to_string.hpp"

namespace parser {

namespace visitor {

class PrintASTVisitor : public BaseVariantVisitor<PrintASTVisitor> {
 private:
  friend class BaseExpressionVisitor<PrintASTVisitor>;
  friend class BaseStatementVisitor<PrintASTVisitor>;
  friend class BaseDefinitionVisitor<PrintASTVisitor>;

 private:
  class IndentationGuard {
   public:
    explicit IndentationGuard(PrintASTVisitor& visitor) : visitor_(visitor) {
      visitor_.printMessage("{\n");
      visitor_.increaseIndent();
    }

    ~IndentationGuard() {
      visitor_.decreaseIndent();
      visitor_.printMessage("}\n");
    }

   private:
    PrintASTVisitor& visitor_;
  };

 public:
  explicit PrintASTVisitor(std::ostream& os, int indentSize = 2,
                           char indentChar = ' ')
      : os_(os),
        indentSize_(indentSize),
        indentChar_(indentChar),
        indentLevel_(0) {}

  using BaseExpressionVisitor<PrintASTVisitor>::visit;
  using BaseStatementVisitor<PrintASTVisitor>::visit;
  using BaseDefinitionVisitor<PrintASTVisitor>::visit;

 protected:
  // -------------------------------- Literals --------------------------------
  template <token::Literal LiteralT>
  void visitImpl(const LiteralT& literal) {
    printMessage(toStringUnqualified<LiteralT>() + ":\n");
    {
      IndentationGuard scope_for_literal_value(*this);
      printMessage(std::to_string(literal.value) + "\n");
    }
  }

  void visitImpl(const StrLiteral& literal) {
    printMessage(toStringUnqualified<StrLiteral>() + ":\n");
    {
      IndentationGuard scope_for_literal_value(*this);
      printMessage(literal.value + "\n");
    }
  }

  // ----------------------------- Identificator ------------------------------
  void visitImpl(const Identificator& id) {
    printMessage(toStringUnqualified<Identificator>() + ":\n");
    {
      IndentationGuard scope_for_identificator_value(*this);
      printMessage(id.value + "\n");
    }
  }

  // ---------------------------- Binary operations ---------------------------
  template <BinaryArithmetic ArithmeticOperationT>
  void visitImpl(const ArithmeticOperationT& op) {
    printMessage(toStringUnqualified<ArithmeticOperationT>() + ":\n");
    IndentationGuard scope_for_operands(*this);

    printMessage("Left Operand:\n");
    {
      IndentationGuard scope_for_left_operand(*this);
      visit(*op.left_operand);
    }

    printMessage("Right Operand:\n");
    {
      IndentationGuard scope_for_right_operand(*this);
      visit(*op.right_operand);
    }
  }

  template <BinaryLogical LogicalOperationT>
  void visitImpl(const LogicalOperationT& op) {
    printMessage(toStringUnqualified<LogicalOperationT>() + ":\n");
    {
      IndentationGuard scope_for_operands(*this);

      printMessage("Left Operand:\n");
      {
        IndentationGuard scope_for_left_operand(*this);
        visit(*op.left_operand);
      }

      printMessage("Right Operand:\n");
      {
        IndentationGuard scope_for_right_operand(*this);
        visit(*op.right_operand);
      }
    }
  }

  void visitImpl(const Assign& op) {
    printMessage(toStringUnqualified<Assign>() + ":\n");
    {
      IndentationGuard scope_for_arguments(*this);

      printMessage("Left Value:\n");
      {
        IndentationGuard scope_for_lvalue(*this);
        visit(*op.left_operand);
      }

      printMessage("Right Value:\n");
      {
        IndentationGuard scope_for_rvalue(*this);
        visit(*op.right_operand);
      }
    }
  }

  // ------------------------------ Unary operations --------------------------
  template <Unary UnaryOperationT>
  void visitImpl(const UnaryOperationT& op) {
    printMessage(toStringUnqualified<UnaryOperationT>() + ":\n");
    {
      IndentationGuard scope_for_operand(*this);

      printMessage("Operand:\n");
      {
        IndentationGuard scope_for_expression(*this);
        visit(*op.operand);
      }
    }
  }

  // ---------------------------------- Call ----------------------------------
  void visitImpl(const Call& call) {
    printMessage("Call:\n");
    {
      IndentationGuard _(*this);

      printMessage("Callee:\n");
      {
        IndentationGuard scope_for_callee(*this);
        visit(*call.callee);
      }

      printMessage("Arguments:\n");
      printMessage("(\n");
      {
        // scope for arguments of call:
        increaseIndent();
        for (const auto& arg : call.arguments) {
          visit(*arg);
        }
        decreaseIndent();
      }
      printMessage(")\n");
    }
  }

  // ------------------------------ Statements --------------------------------
  void visitImpl(const ReturnStatement& ret) {
    printMessage("Return:\n");
    {
      IndentationGuard scope_for_value(*this);
      if (ret.value) {
        printMessage("Value:\n");
        {
          IndentationGuard scope_for_internal_expression(*this);
          visit(**ret.value);
        }
      } else {
        printMessage("Value: (none)\n");
      }
    }
  }

  void visitImpl(const ExpressionStatement& exprStmt) {
    printMessage("Expression:\n");
    {
      IndentationGuard guard(*this);
      visit(*exprStmt.expression);
    }
  }

  void visitImpl(const IfStatement& ifStmt) {
    printMessage("If\n");
    {
      IndentationGuard guard(*this);

      printMessage("Condition:\n");
      {
        IndentationGuard scope_for_condition(*this);
        visit(*ifStmt.condition);
      }

      printMessage("Then:\n");
      {
        IndentationGuard scope_for_then(*this);
        visit(*ifStmt.then_branch);
      }

      if (ifStmt.else_branch) {
        printMessage("Else:\n");
        {
          IndentationGuard scope_for_else(*this);
          visit(**ifStmt.else_branch);
        }
      }
    }
  }

  void visitImpl(const WhileStatement& whileStmt) {
    printMessage("While\n");
    {
      IndentationGuard guard(*this);

      printMessage("Condition:\n");
      {
        IndentationGuard scope_for_condition(*this);
        visit(*whileStmt.condition);
      }

      printMessage("Body:\n");
      {
        IndentationGuard scope_for_body(*this);
        visit(*whileStmt.body);
      }
    }
  }

  void visitImpl(const VariableDeclaration& varDecl) {
    printMessage("Declaration:\n");
    {
      IndentationGuard guard(*this);

      printMessage("Variable:\n");
      {
        IndentationGuard scope_for_variable_name(*this);
        printMessage(varDecl.name + "\n");
      }

      printMessage("Value:\n");
      {
        IndentationGuard scope_for_variable_value(*this);
        visit(*varDecl.value);
      }
    }
  }

  void visitImpl(const ScopeStatement& scope) {
    printMessage("Scope:\n");
    {
      IndentationGuard guard(*this);
      for (const auto& stmt : scope.statements) {
        visit(*stmt);
      }
    }
  }

  // ----------------------------- Definitions --------------------------------
  void visitImpl(const FunctionDeclaration& func) {
    printMessage("Function:\n");
    {
      IndentationGuard guard(*this);

      printMessage("Name:\n");
      {
        IndentationGuard scope_for_function_name(*this);
        printMessage(func.name + "\n");
      }

      printMessage("Parameters:\n");
      printMessage("(\n");
      {
        // scope for parameters:
        increaseIndent();
        for (const auto& param : func.parameters) {
          printMessage(param + "\n");
        }
        decreaseIndent();
      }
      printMessage(")\n");

      printMessage("Body:\n");
      {
        IndentationGuard scope_for_function_body(*this);
        visit(func.body);
      }
    }
  }

 private:
  std::ostream& os_;
  int indentSize_;
  char indentChar_;
  size_t indentLevel_;

  void printMessage(const std::string& message) {
    os_ << std::string(indentLevel_ * indentSize_, indentChar_) << message;
  }

  void increaseIndent() { ++indentLevel_; }
  void decreaseIndent() { --indentLevel_; }
};

}  // namespace visitor

void printAST(const std::pair<Program, Positions>& ast);

}  // namespace parser