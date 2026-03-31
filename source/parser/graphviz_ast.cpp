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
#include "token/tokens.hpp"
#include "utils/type_to_string.hpp"

namespace parser {

namespace details {

static size_t ast_dump_call_counter = 0u;

struct DotBuilder {
 public:
  std::string createNewNode(const std::string& label,
                        const std::string& color = "white") {
    std::string id = "node" + std::to_string(counter_++);
    dot_stream_ << id << " [label=\"" << label
                << "\", style=filled, fillcolor=" << color << "];\n";
    return id;
  }

  void addEdge(const std::string& from,
                const std::string& to,
                const std::string& label = "") {
    dot_stream_ << from << " -> " << to;
    if (!label.empty()) {
        dot_stream_ << " [label=\"" << label << "\"]";
    }
    dot_stream_ << ";\n";
  }

  std::string show() const { return dot_stream_.str(); }

 private:
  std::size_t counter_ = 0;
  std::ostringstream dot_stream_;
};

}  // namespace details

// -----------------------------< Dump functions >-----------------------------

namespace colors {

enum class Color : size_t {
  kLiteral = 0u,
  kIdentitificator,
  kBinaryOp,
  kUnaryOp,
  kAssign,
  kCall,
  kIfStmt,
  kWhileStmt,
  kVarDecl,
  kScope,
  kFuncDecl,
};

constexpr std::string kColorNames[] {
  "lightgreen",
  "yellow",
  "pink",
  "violet",
  "red",
  "green",
  "lightblue",
  "blue",
  "orange",
  "lightgray",
  "white",
};

constexpr std::string getColorName(Color color) {
  return kColorNames[static_cast<size_t>(color)];
}

} // namespace colors

namespace labels {

enum class Label : size_t {
  kLeftOperand = 0u,
  kRightOperand,
  kLeftValue,
  kRightValue,
  kCondition,
  kThen,
  kElse,
  kBody,
};

constexpr std::string kLabelNames[] {
  "L",
  "R",
  "lvalue",
  "rvalue",
  "cond",
  "then",
  "else",
  "body",
};

constexpr std::string getLabelName(Label label) {
  return kLabelNames[static_cast<size_t>(label)];
}

} // namespace labels

std::string dumpExpression(const ExpressionVariant& expr, details::DotBuilder& builder) {
  using Color = colors::Color;
  using Label = labels::Label;

  return std::visit(
      overloaded{

          [&]<token::Literal LiteralT>(const LiteralT& literal) {
            auto label = std::format("{}\\n[{}]",
                toStringUnqualified<LiteralT>(),
                std::to_string(literal.value)
            );
            return builder.createNewNode(label, colors::getColorName(Color::kLiteral));
          },

          [&](const StrLiteral& literal) {
            auto label = std::format("{}\\n[{}]",
                toStringUnqualified<StrLiteral>(),
                literal.value
            );
            return builder.createNewNode(label, colors::getColorName(Color::kLiteral));
          },

          [&](const Identificator& id) {
            auto label = std::format("{}\\n[{}]",
                toStringUnqualified<Identificator>(),
                id.value
            );
            return builder.createNewNode(label, colors::getColorName(Color::kIdentitificator));
          },

          [&]<BinaryArithmetic ArithmeticOperationT>(const ArithmeticOperationT& op) {
            auto root = builder.createNewNode(
                toStringUnqualified<ArithmeticOperationT>(),
                colors::getColorName(Color::kBinaryOp)
            );
            auto left = dumpExpression(*op.left_operand, builder);
            auto right = dumpExpression(*op.right_operand, builder);

            builder.addEdge(root, left, labels::getLabelName(Label::kLeftOperand));
            builder.addEdge(root, right, labels::getLabelName(Label::kRightOperand));

            return root;
          },

          [&]<BinaryLogical LogicalOperationT>(const LogicalOperationT& op) {
            auto root = builder.createNewNode(
                toStringUnqualified<LogicalOperationT>(),
                colors::getColorName(Color::kBinaryOp)
            );
            auto left = dumpExpression(*op.left_operand, builder);
            auto right = dumpExpression(*op.right_operand, builder);

            builder.addEdge(root, left, labels::getLabelName(Label::kLeftOperand));
            builder.addEdge(root, right, labels::getLabelName(Label::kRightOperand));

            return root;
          },

          [&](const Assign& op) {
            auto root = builder.createNewNode(
                toStringUnqualified<Assign>(),
                colors::getColorName(Color::kAssign)
            );
            auto left = dumpExpression(*op.left_operand, builder);
            auto right = dumpExpression(*op.right_operand, builder);

            builder.addEdge(root, left, labels::getLabelName(Label::kLeftValue));
            builder.addEdge(root, right, labels::getLabelName(Label::kRightValue));

            return root;
          },

          [&]<Unary UnaryOperationT>(const UnaryOperationT& op) {
            auto root = builder.createNewNode(toStringUnqualified<UnaryOperationT>());

            auto child = dumpExpression(*op.operand, builder);
            builder.addEdge(root, child);

            return root;
          },

          [&](const Call& call) {
            auto root = builder.createNewNode(toStringUnqualified<Call>());

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
  using Color = colors::Color;
  using Label = labels::Label;

  return std::visit(
      overloaded{

          [&](const ScopeStatement& scope) {
            auto root = builder.createNewNode(
                "Scope",
              colors::getColorName(Color::kScope)
            );

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
            auto root = builder.createNewNode(
                "If",
              colors::getColorName(Color::kIfStmt)
            );
            auto cond = dumpExpression(*ifStmt.condition, builder);
            builder.addEdge(root, cond, labels::getLabelName(Label::kCondition));

            auto thenNode = dumpStatement(*ifStmt.then_branch, builder);
            builder.addEdge(root, thenNode, labels::getLabelName(Label::kThen));

            if (ifStmt.else_branch) {
              auto elseNode = dumpStatement(**ifStmt.else_branch, builder);
              builder.addEdge(root, elseNode, labels::getLabelName(Label::kElse));
            }

            return root;
          },

          [&](const WhileStatement& whileStmt) {
            auto root = builder.createNewNode(
                "While",
                colors::getColorName(Color::kWhileStmt)
              );

            auto cond = dumpExpression(*whileStmt.condition, builder);
            auto body = dumpStatement(*whileStmt.body, builder);

            builder.addEdge(root, cond, labels::getLabelName(Label::kCondition));
            builder.addEdge(root, body, labels::getLabelName(Label::kBody));

            return root;
          },

          [&](const VariableDeclaration& var) { 
            auto label = std::format("{}\\n[{}]",
                toStringUnqualified<VariableDeclaration>(),
                var.name
            );
            auto root = builder.createNewNode(
                label,
                colors::getColorName(Color::kVarDecl)
            );

            auto val = dumpExpression(*var.value, builder);
            builder.addEdge(root, val);

            return root;
          }

      },
      stmt);
}

void dumpFunctionDeclaration(const FunctionDeclaration& function,
                             details::DotBuilder& builder) {
  auto label = std::format("{}\\n[{}]",
      "Function",
      function.name
  );

  auto root = builder.createNewNode(
      label,
      colors::getColorName(colors::Color::kFuncDecl)
  );

  for (const auto& param : function.parameters) {
    auto p = builder.createNewNode("Param\\n" + param);
    builder.addEdge(root, p);
  }

  auto body = dumpStatement(StatementVariant{function.body}, builder);
  builder.addEdge(root, body, labels::getLabelName(labels::Label::kBody));
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
  return std::format("ast{}", ++details::ast_dump_call_counter);
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