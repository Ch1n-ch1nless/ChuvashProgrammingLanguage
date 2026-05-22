#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Support/raw_ostream.h>

#include <parser/nodes.hpp>
#include <parser/visitor.hpp>

namespace codegen {

class IRBuilderVisitor
    : public parser::visitor::BaseVariantVisitor<llvm::Value*, IRBuilderVisitor> {
public:
    IRBuilderVisitor();

    llvm::Module* generate(const parser::Program& program);

    llvm::LLVMContext& context() { return context_; }
    llvm::Module* module() { return module_.get(); }
    void dump();

    // Make all visit overloads visible
    using parser::visitor::BaseTypeVisitor<llvm::Value*, IRBuilderVisitor>::visit;
    using parser::visitor::BaseExpressionVisitor<llvm::Value*, IRBuilderVisitor>::visit;
    using parser::visitor::BaseStatementVisitor<llvm::Value*, IRBuilderVisitor>::visit;
    using parser::visitor::BaseDefinitionVisitor<llvm::Value*, IRBuilderVisitor>::visit;

    // Types
    llvm::Value* visitImpl(const parser::BuiltinType& node);
    llvm::Value* visitImpl(const parser::UserType& node);

    // Literals
    llvm::Value* visitImpl(const parser::IntLiteral& node);
    llvm::Value* visitImpl(const parser::FltLiteral& node);
    llvm::Value* visitImpl(const parser::StrLiteral& node);

    // Identifier
    llvm::Value* visitImpl(const parser::Identificator& node);

    // Binary operations
    llvm::Value* visitImpl(const parser::Addition& node);
    llvm::Value* visitImpl(const parser::Subtraction& node);
    llvm::Value* visitImpl(const parser::Multiplication& node);
    llvm::Value* visitImpl(const parser::Division& node);
    llvm::Value* visitImpl(const parser::Remainder& node);
    llvm::Value* visitImpl(const parser::Equal& node);
    llvm::Value* visitImpl(const parser::NotEqual& node);
    llvm::Value* visitImpl(const parser::LessThan& node);
    llvm::Value* visitImpl(const parser::LessEqual& node);
    llvm::Value* visitImpl(const parser::GreaterThan& node);
    llvm::Value* visitImpl(const parser::GreaterEqual& node);
    llvm::Value* visitImpl(const parser::And& node);
    llvm::Value* visitImpl(const parser::Or& node);
    llvm::Value* visitImpl(const parser::Xor& node);
    llvm::Value* visitImpl(const parser::Assign& node);

    // Unary operations
    llvm::Value* visitImpl(const parser::UnaryPlus& node);
    llvm::Value* visitImpl(const parser::UnaryMinus& node);
    llvm::Value* visitImpl(const parser::Not& node);

    // Call
    llvm::Value* visitImpl(const parser::Call& node);

    // Statements
    llvm::Value* visitImpl(const parser::ExpressionStatement& node);
    llvm::Value* visitImpl(const parser::ReturnStatement& node);
    llvm::Value* visitImpl(const parser::VariableDeclaration& node);
    llvm::Value* visitImpl(const parser::ScopeStatement& node);
    llvm::Value* visitImpl(const parser::IfStatement& node);
    llvm::Value* visitImpl(const parser::WhileStatement& node);
    llvm::Value* visitImpl(const parser::Parameter& node);

    // Definition
    llvm::Value* visitImpl(const parser::FunctionDeclaration& node);

private:
    llvm::Type* toLLVMType(const parser::TypeVariant& type);
    bool isFloatingType(llvm::Type* ty);

    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* function,
                                             llvm::Type* type,
                                             const std::string& name);
    llvm::Value* loadVariable(const std::string& name);
    llvm::AllocaInst* lookupVariable(const std::string& name);

    void pushScope();
    void popScope();

    llvm::LLVMContext context_;
    llvm::IRBuilder<> builder_;
    std::unique_ptr<llvm::Module> module_;
    llvm::Function* current_function_ = nullptr;

    using Scope = std::unordered_map<std::string, llvm::AllocaInst*>;
    std::vector<Scope> scopes_;
};

} // namespace codegen