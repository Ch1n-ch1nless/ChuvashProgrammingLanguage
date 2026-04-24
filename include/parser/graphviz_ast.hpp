#include <parser/parser.hpp>
#include <sstream>
#include <string>
#include <utility>

#include "parser/nodes.hpp"
#include "parser/visitor.hpp"
#include "token/tokens.hpp"

namespace parser {

namespace visitor {

namespace graphviz {

namespace colors {

enum class Color : size_t {
  kDefault = 0u,
  kLiteral,
  kIdentitificator,
  kExpression,
  kUnary,
  kAssign,
  kCall,
  kControlFlow,
  kVarDecl,
  kScope,
  kFuncDecl,
  kType,
};

struct ColorStyle {
  const char* fill;
  const char* border;
};

constexpr ColorStyle kStyles[] {
  {"#FFFFFF", "#000000"}, // default

  {"#A5D6A7", "#2E7D32"}, // literal (green)
  {"#CE93D8", "#6A1B9A"}, // identitificator (purple)

  {"#90CAF9", "#1565C0"}, // expression (blue)
  {"#B39DDB", "#512DA8"}, // unary

  {"#EF9A9A", "#C62828"}, // assign (red-ish)
  {"#80CBC4", "#00695C"}, // call (teal)

  {"#FFCC80", "#EF6C00"}, // control flow (orange)

  {"#FFE082", "#F9A825"}, // var decl (yellow)
  {"#CFD8DC", "#37474F"}, // scope (gray)

  {"#FFD54F", "#F57F17"}, // function (strong yellow)
  {"#B0BEC5", "#37474F"}, // type (neutral gray-blue)
};

constexpr ColorStyle get(Color c) {
  return kStyles[static_cast<size_t>(c)];
}

} // namespace colors

namespace labels {

enum class Label : size_t {
  kNone = 0u,
  kLeftOperand,
  kRightOperand,
  kLeftValue,
  kRightValue,
  kCondition,
  kThen,
  kElse,
  kBody,
};

constexpr std::string kLabelNames[] {
  "",
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

struct DotBuilder {
 public:
  std::string createNewNode(const std::string& label,
                            colors::Color color = colors::Color::kDefault) {
    const auto style = colors::get(color);

    std::string id = "node" + std::to_string(counter_++);
    dot_stream_ << id
                << " [label=\"" << label << "\""
                << ", shape=box"
                << ", style=\"rounded,filled\""
                << ", fillcolor=\"" << style.fill << "\""
                << ", color=\"" << style.border << "\""
                << ", fontname=\"JetBrains Mono\""
                << "];\n";
    return id;
  }

  void addEdge(const std::string& from,
               const std::string& to,
               labels::Label label = labels::Label::kNone) {
    dot_stream_ << from << " -> " << to;

    if (label != labels::Label::kNone) {
      dot_stream_ << " [label=\"" << labels::getLabelName(label)
                  << "\", fontname=\"JetBrains Mono\"]";
    }

    dot_stream_ << ";\n";
  }

  std::string show() const { return dot_stream_.str(); }

 private:
  std::size_t counter_ = 0;
  std::ostringstream dot_stream_;
};

} // namespace graphviz

class GraphvizASTVisitor : public BaseVariantVisitor<GraphvizASTVisitor> {
 private:
  friend class BaseTypeVisitor<GraphvizASTVisitor>;
  friend class BaseExpressionVisitor<GraphvizASTVisitor>;
  friend class BaseStatementVisitor<GraphvizASTVisitor>;
  friend class BaseDefinitionVisitor<GraphvizASTVisitor>;

 public:
  explicit GraphvizASTVisitor(graphviz::DotBuilder& builder)
      : builder_(builder) {}
  
  using BaseTypeVisitor<GraphvizASTVisitor>::visit;
  using BaseExpressionVisitor<GraphvizASTVisitor>::visit;
  using BaseStatementVisitor<GraphvizASTVisitor>::visit;
  using BaseDefinitionVisitor<GraphvizASTVisitor>::visit;

 protected:
  // ----------------------------------- Types --------------------------------
  void visitImpl(const BuiltinType& type) {
    std::string kindStr;
    switch (type.kind) {
      case BuiltinType::Kind::kInt:
        kindStr = "Integer";
        break;
      case BuiltinType::Kind::kFloat:
        kindStr = "Float";
        break;
      case BuiltinType::Kind::kBool:
        kindStr = "Boolean";
        break;
      case BuiltinType::Kind::kString:
        kindStr = "String";
        break;
    }

    auto label = std::format("{}\\n[{}]",
        toStringUnqualified<BuiltinType>(),
        kindStr
    );
    visit_result_ = 
        builder_.createNewNode(label, graphviz::colors::Color::kType);
  }

  void visitImpl(const UserType& type) {
    auto label = std::format("{}\\n[{}]",
        toStringUnqualified<UserType>(),
        type.name
    );
    visit_result_ = 
        builder_.createNewNode(label, graphviz::colors::Color::kType);
  }

  // -------------------------------- Literals --------------------------------
  template <token::concepts::IsLiteral LiteralT>
  void visitImpl(const LiteralT& literal) {
    auto label = std::format("{}\\n[{}]",
        toStringUnqualified<LiteralT>(),
        std::to_string(literal.value)
    );
    visit_result_ = 
        builder_.createNewNode(label, graphviz::colors::Color::kLiteral);
  }

  void visitImpl(const StrLiteral& literal) {
    auto label = std::format("{}\\n[{}]",
        toStringUnqualified<StrLiteral>(),
        literal.value
    );
    visit_result_ = 
        builder_.createNewNode(label, graphviz::colors::Color::kLiteral);
  }

  // ----------------------------- Identificator ------------------------------
  void visitImpl(const Identificator& id) {
    auto label = std::format("{}\\n[{}]",
        toStringUnqualified<Identificator>(),
        id.value
    );
    visit_result_ = 
        builder_.createNewNode(label, graphviz::colors::Color::kIdentitificator);
  }

  // ---------------------------- Binary operations ---------------------------
  template <parser::concepts::IsBinaryArithmetic ArithmeticOperationT>
  void visitImpl(const ArithmeticOperationT& op) {
    auto root = builder_.createNewNode(
        toStringUnqualified<ArithmeticOperationT>(),
        graphviz::colors::Color::kExpression
    );

    visit(*op.left_operand);
    auto left = visit_result_;

    visit(*op.right_operand);
    auto right = visit_result_;

    builder_.addEdge(root, left, graphviz::labels::Label::kLeftOperand);
    builder_.addEdge(root, right,graphviz::labels::Label::kRightOperand);

    visit_result_ = root;
  }

  template <parser::concepts::IsBinaryLogical LogicalOperationT>
  void visitImpl(const LogicalOperationT& op) {
    auto root = builder_.createNewNode(
        toStringUnqualified<LogicalOperationT>(),
        graphviz::colors::Color::kExpression
    );

    visit(*op.left_operand);
    auto left = visit_result_;

    visit(*op.right_operand);
    auto right = visit_result_;

    builder_.addEdge(root, left, graphviz::labels::Label::kLeftOperand);
    builder_.addEdge(root, right,graphviz::labels::Label::kRightOperand);

    visit_result_ = root;
  }

  void visitImpl(const Assign& op) {
    auto root = builder_.createNewNode(
        toStringUnqualified<Assign>(),
        graphviz::colors::Color::kAssign
    );

    visit(*op.left_operand);
    auto left = visit_result_;

    visit(*op.right_operand);
    auto right = visit_result_;

    builder_.addEdge(root, left, graphviz::labels::Label::kLeftValue);
    builder_.addEdge(root, right,graphviz::labels::Label::kRightValue);

    visit_result_ = root;
  }

  // ------------------------------ Unary operations --------------------------
  template <parser::concepts::IsUnary UnaryOperationT>
  void visitImpl(const UnaryOperationT& op) {
    auto root = builder_.createNewNode(
        toStringUnqualified<UnaryOperationT>(),
        graphviz::colors::Color::kUnary
    );

    visit(*op.operand);
    auto child = visit_result_;

    builder_.addEdge(root, child);

    visit_result_ = root;
  }

  // ---------------------------------- Call ----------------------------------
  void visitImpl(const Call& call) {
    auto root = builder_.createNewNode(
        toStringUnqualified<Call>(),
        graphviz::colors::Color::kCall
    );

    visit(*call.callee);
    auto callee = visit_result_;
    builder_.addEdge(root, callee);

    for (const auto& arg : call.arguments) {
      visit(*arg);
      auto argNode = visit_result_;
      builder_.addEdge(root, argNode);
    }

    visit_result_ = root;
  }

  // ------------------------------ Statements --------------------------------
  void visitImpl(const ReturnStatement& ret) {
    auto root = builder_.createNewNode(
        "Return",
        graphviz::colors::Color::kDefault
    );

    if (ret.value) {
      visit(**ret.value);
      auto val = visit_result_;
      builder_.addEdge(root, val);
    }

    visit_result_ = root;
  }

  void visitImpl(const ExpressionStatement& exprStmt) {
    auto root = builder_.createNewNode("ExprStmt");

    visit(*exprStmt.expression);
    auto child = visit_result_;

    builder_.addEdge(root, child);
    visit_result_ = root;
  }

  void visitImpl(const IfStatement& ifStmt) {
    auto root = builder_.createNewNode(
        "If",
        graphviz::colors::Color::kControlFlow
    );

    visit(*ifStmt.condition);
    auto cond = visit_result_;
    builder_.addEdge(
        root, 
        cond, 
        graphviz::labels::Label::kCondition
    );

    visit(*ifStmt.then_branch);
    auto thenNode = visit_result_;
    builder_.addEdge(
        root, 
        thenNode, 
        graphviz::labels::Label::kThen
    );

    if (ifStmt.else_branch) {
      visit(**ifStmt.else_branch);
      auto elseNode = visit_result_;
      builder_.addEdge(
          root, 
          elseNode, 
          graphviz::labels::Label::kElse
      );
    }

    visit_result_ = root;
  }

  void visitImpl(const WhileStatement& whileStmt) {
    auto root = builder_.createNewNode(
        "While",
        graphviz::colors::Color::kControlFlow
    );

    visit(*whileStmt.condition);
    auto cond = visit_result_;
    builder_.addEdge(
        root, 
        cond, 
        graphviz::labels::Label::kCondition
    );

    visit(*whileStmt.body);
    auto body = visit_result_;
    builder_.addEdge(
        root, 
        body, 
        graphviz::labels::Label::kBody
    );

    visit_result_ = root;
  }

  void visitImpl(const VariableDeclaration& varDecl) {
    auto label = std::format("{}\\n[{}]",
        toStringUnqualified<VariableDeclaration>(),
        varDecl.name
    );
    auto root = builder_.createNewNode(
        label,
        graphviz::colors::Color::kVarDecl
    );

    visit(varDecl.type);
    auto val = visit_result_;
    builder_.addEdge(root, val);

    visit_result_ = root;
  }

  void visitImpl(const Parameter& param) {
    auto label = std::format("{}\\n[{}]",
        toStringUnqualified<Parameter>(),
        param.name
    );
    auto root = builder_.createNewNode(
        label,
        graphviz::colors::Color::kVarDecl
    );

    visit(param.type);
    auto type = visit_result_;
    builder_.addEdge(root, type);

    visit_result_ = root;
  }

  void visitImpl(const ScopeStatement& scope) {
    auto root = builder_.createNewNode(
        "Scope",
        graphviz::colors::Color::kScope
    );

    for (const auto& stmt : scope.statements) {
      visit(*stmt);
      auto child = visit_result_;
      builder_.addEdge(root, child);
    }

    visit_result_ = root;
  }

  // ----------------------------- Definitions --------------------------------
  void visitImpl(const FunctionDeclaration& func) {
    auto label = std::format("{}\\n[{}]",
      "Function",
      func.name
    );

    auto root = builder_.createNewNode(
        label,
        graphviz::colors::Color::kFuncDecl
    );

    for (const auto& param : func.parameters) {
      visit(param);
      builder_.addEdge(root, visit_result_);
    }

    visit(func.body);
    auto body = visit_result_;
    builder_.addEdge(
        root, 
        body, 
        graphviz::labels::Label::kBody
    );
  }

 private:
  graphviz::DotBuilder& builder_;
  std::string visit_result_;
};

} // namespace visitor

class ASTGraphVizDumper {
 public:
  // Dumper needs to know, where it would generate new pictures
  explicit ASTGraphVizDumper(std::string output_dir_name);

  std::string generateFileName() const;

  void dumpToDotFile(const std::pair<Program, Positions>& ast);
  void dumpToDotFile(const std::pair<Program, Positions>& ast,
                     const std::string& dot_file_name);

  void dumpToPng(const std::pair<Program, Positions>& ast);
  void dumpToPng(const std::pair<Program, Positions>& ast,
                 const std::string& png_file_name);

 private:
  std::string output_dir_name_;
};

}  // namespace parser
