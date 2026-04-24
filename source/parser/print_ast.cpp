#include <iostream>
#include <parser/print_ast.hpp>

namespace parser {

void printAST(const std::pair<Program, Positions>& ast) {
  visitor::PrintASTVisitor visitor(std::cout);
  std::cout << "Abstract Syntax Tree:\n" << "{\n";

  const auto& functions = ast.first.functions;
  for (const auto& function : functions) {
    visitor.visit(function);
  }
  
  std::cout << "}\n";
}

}  // namespace parser