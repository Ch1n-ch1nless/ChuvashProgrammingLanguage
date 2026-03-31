#include <cstddef>
#include <cstdlib>
#include <format>
#include <fstream>
#include <parser/graphviz_ast.hpp>
#include <sstream>
#include <string>
#include <utility>

#include "parser/nodes.hpp"
#include "parser/parser.hpp"

namespace parser {

namespace details {

static size_t ast_dump_call_counter = 0u;

struct DotBuilder {
 public:
  std::string createNewNode(const std::string& label) {
    std::string id = "node" + std::to_string(counter_++);
    dot_stream_ << id << " [label=\"" << label << "\"];\n";
    return id;
  }

  void addEdge(const std::string& from, const std::string& to) {
    dot_stream_ << from << " -> " << to << ";\n";
  }

  std::string show() const { return dot_stream_.str(); }

 private:
  std::size_t counter_ = 0;
  std::ostringstream dot_stream_;
};

}  // namespace details

// -----------------------------< Dump functions >-----------------------------

std::string dumpExpression(const ExpressionVariant& expr, details::DotBuilder& builder) {
  return std::visit(
      overloaded{

          [&]<token::Literal LiteralT>(const LiteralT& literal) {
            return builder.createNewNode(toStringUnqualified<LiteralT>() + "\\n" +
                                   std::to_string(literal.value));
          },

          [&](const StrLiteral& literal) {
            return builder.createNewNode("String\\n" + literal.value);
          },

          [&](const Identificator& id) {
            return builder.createNewNode("Id\\n" + id.value);
          },

          [&]<BinaryArithmetic T>(const T& op) {
            auto root = builder.createNewNode(toStringUnqualified<T>());

            auto left = dumpExpression(*op.left_operand, builder);
            auto right = dumpExpression(*op.right_operand, builder);

            builder.addEdge(root, left);
            builder.addEdge(root, right);

            return root;
          },

          [&]<BinaryLogical T>(const T& op) {
            auto root = builder.createNewNode(toStringUnqualified<T>());

            auto left = dumpExpression(*op.left_operand, builder);
            auto right = dumpExpression(*op.right_operand, builder);

            builder.addEdge(root, left);
            builder.addEdge(root, right);

            return root;
          },

          [&](const Assign& op) {
            auto root = builder.createNewNode("Assign");

            auto left = dumpExpression(*op.left_operand, builder);
            auto right = dumpExpression(*op.right_operand, builder);

            builder.addEdge(root, left);
            builder.addEdge(root, right);

            return root;
          },

          [&]<Unary T>(const T& op) {
            auto root = builder.createNewNode(toStringUnqualified<T>());

            auto child = dumpExpression(*op.operand, builder);
            builder.addEdge(root, child);

            return root;
          },

          [&](const Call& call) {
            auto root = builder.createNewNode("Call");

            auto callee = dumpExpression(*call.callee, builder);
            builder.addEdge(root, callee);

            for (const auto& arg : call.arguments) {
              auto argNode = dumpExpression(*arg, builder);
              builder.addEdge(root, argNode);
            }

            return root;
          }
      },
      expr);
}

std::string dumpStatement(const StatementVariant& stmt,
                          details::DotBuilder& builder) {
  return std::visit(
      overloaded{

          [&](const ScopeStatement& scope) {
            auto root = builder.createNewNode("Scope");

            for (const auto& s : scope.statements) {
              auto child = dumpStatement(*s, builder);
              builder.addEdge(root, child);
            }

            return root;
          },

          [&](const ReturnStatement& ret) {
            auto root = builder.createNewNode("Return");

            if (ret.value) {
              auto val = dumpExpression(**ret.value, builder);
              builder.addEdge(root, val);
            }

            return root;
          },

          [&](const ExpressionStatement& expr) {
            auto root = builder.createNewNode("ExprStmt");
            auto child = dumpExpression(*expr.expression, builder);
            builder.addEdge(root, child);
            return root;
          },

          [&](const IfStatement& ifStmt) {
            auto root = builder.createNewNode("If");

            auto cond = dumpExpression(*ifStmt.condition, builder);
            builder.addEdge(root, cond);

            auto thenNode = dumpStatement(*ifStmt.then_branch, builder);
            builder.addEdge(root, thenNode);

            if (ifStmt.else_branch) {
              auto elseNode = dumpStatement(**ifStmt.else_branch, builder);
              builder.addEdge(root, elseNode);
            }

            return root;
          },

          [&](const WhileStatement& whileStmt) {
            auto root = builder.createNewNode("While");

            auto cond = dumpExpression(*whileStmt.condition, builder);
            auto body = dumpStatement(*whileStmt.body, builder);

            builder.addEdge(root, cond);
            builder.addEdge(root, body);

            return root;
          },

          [&](const VariableDeclaration& var) {
            auto root = builder.createNewNode("VarDecl\\n" + var.name);

            auto val = dumpExpression(*var.value, builder);
            builder.addEdge(root, val);

            return root;
          }

      },
      stmt);
}

void dumpFunctionDeclaration(const FunctionDeclaration& function,
                             details::DotBuilder& builder) {
  auto root = builder.createNewNode("Function\\n" + function.name);

  for (const auto& param : function.parameters) {
    auto p = builder.createNewNode("Param\\n" + param);
    builder.addEdge(root, p);
  }

  auto body = dumpStatement(StatementVariant{function.body}, builder);
  builder.addEdge(root, body);
}

// -------------------------< Methods implementation >-------------------------

ASTGraphVizDumper::ASTGraphVizDumper(std::string output_dir_name)
    : output_dir_name_(std::move(output_dir_name)) {}

void ASTGraphVizDumper::dumpToDotFile(const std::pair<Program, Positions>& ast,
                                      const std::string& dot_file_name) {
  details::DotBuilder builder;

  const auto& functions = ast.first.functions;

  for (const auto& function : functions) {
    dumpFunctionDeclaration(function, builder);
  }

  auto full_path_to_dot =
      std::format("{}/{}.dot", output_dir_name_, dot_file_name);

  std::ofstream file(full_path_to_dot);

  file << "digraph AST {\n";
  file << builder.show();
  file << "}\n";
}

std::string ASTGraphVizDumper::generateFileName() const {
  return std::format("ast{}.dot", details::ast_dump_call_counter);
}

void ASTGraphVizDumper::dumpToDotFile(
    const std::pair<Program, Positions>& ast) {
  dumpToDotFile(ast,
                std::format("ast{}.dot", ++details::ast_dump_call_counter));
}

void ASTGraphVizDumper::dumpToPng(const std::pair<Program, Positions>& ast) {
  dumpToDotFile(ast);

  auto full_path_to_dot = std::format("{}/ast{}.dot", output_dir_name_,
                                      details::ast_dump_call_counter);
  auto full_path_to_png = std::format("{}/ast{}.png", output_dir_name_,
                                      details::ast_dump_call_counter);

  auto command =
      std::format("dot -Tpng {} -o {}", full_path_to_dot, full_path_to_png);

  std::system(command.c_str());
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