#include <iostream>
#include <parser/print_ast.hpp>

namespace parser {

void printAST(const std::pair<Program, Positions>& ast) {
  /*std::size_t indentation = 0u;

  printWithIndentation("Abstract Syntax Tree:\n", indentation);
  printWithIndentation("{\n", indentation);

  const auto& functions = ast.first.functions;
  for (const auto& function : functions) {
    ++indentation;
    printFunctionDeclaration(function, indentation);
    --indentation;
  }

  printWithIndentation("}\n", indentation);*/

  visitor::PrintASTVisitor visitor(std::cout);
  std::cout << "Abstract Syntax Tree:\n" << "{\n";

  const auto& functions = ast.first.functions;
  for (const auto& function : functions) {
    visitor.visit(function);
  }
  
  std::cout << "}\n";
}

}  // namespace parser