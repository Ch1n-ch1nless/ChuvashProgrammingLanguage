#include <cstddef>
#include <fstream>

#include <parser/graphviz_ast.hpp>

namespace parser {

static size_t ast_dump_call_counter = 0u;

ASTGraphVizDumper::ASTGraphVizDumper(std::string output_dir_name)
    : output_dir_name_(std::move(output_dir_name)) {}

void ASTGraphVizDumper::dumpToDotFile(const std::pair<Program, Positions>& ast,
                                      const std::string& dot_file_name) {
  visitor::graphviz::DotBuilder builder;
  visitor::GraphvizASTVisitor visitor(builder);

  const auto& functions = ast.first.functions;

  for (const auto& function : functions) {
    visitor.visit(function);
  }

  auto full_path_to_dot =
      std::format("{}/{}.dot", output_dir_name_, dot_file_name);

  std::ofstream file(full_path_to_dot);

  file << "digraph AST {\n";
  file << builder.show();
  file << "}\n";
}

std::string ASTGraphVizDumper::generateFileName() const {
  return std::format("ast{}", ++ast_dump_call_counter);
}

void ASTGraphVizDumper::dumpToDotFile(
    const std::pair<Program, Positions>& ast) {
  dumpToDotFile(ast, generateFileName());
}

void ASTGraphVizDumper::dumpToPng(const std::pair<Program, Positions>& ast) {
  dumpToPng(ast, generateFileName());
}

void ASTGraphVizDumper::dumpToPng(const std::pair<Program, Positions>& ast,
                                  const std::string& png_file_name) {
  dumpToDotFile(ast, png_file_name);

  auto full_path_to_dot =
      std::format("{}/{}.dot", output_dir_name_, png_file_name);
  auto full_path_to_png =
      std::format("{}/{}.png", output_dir_name_, png_file_name);

  auto command =
      std::format("dot -Tpng {} -o {}", full_path_to_dot, full_path_to_png);

  std::system(command.c_str());
}

}  // namespace parser
