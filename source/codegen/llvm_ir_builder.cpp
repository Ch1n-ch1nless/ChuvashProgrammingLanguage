#include "codegen/llvm_ir_builder.hpp"
#include <stdexcept>

namespace codegen {

IRBuilderVisitor::IRBuilderVisitor()
    : builder_(context_)
    , module_(std::make_unique<llvm::Module>("module", context_)) {
    pushScope();
}

llvm::Module* IRBuilderVisitor::generate(const parser::Program& program) {
    for (const auto& func : program.functions) {
        visitImpl(func);
    }
    llvm::verifyModule(*module_);
    return module_.get();
}

void IRBuilderVisitor::dump() {
    module_->print(llvm::outs(), nullptr);
}

llvm::Type* IRBuilderVisitor::toLLVMType(const parser::TypeVariant& type) {
    return std::visit([this](const auto& t) -> llvm::Type* {
        using T = std::decay_t<decltype(t)>;
        if constexpr (std::is_same_v<T, parser::BuiltinType>) {
            switch (t.kind) {
                case parser::BuiltinType::Kind::kInt:
                    return llvm::Type::getInt32Ty(context_);
                case parser::BuiltinType::Kind::kFloat:
                    return llvm::Type::getDoubleTy(context_);
                case parser::BuiltinType::Kind::kBool:
                    return llvm::Type::getInt1Ty(context_);
                case parser::BuiltinType::Kind::kString:
                    return llvm::PointerType::get(context_, 0);
                case parser::BuiltinType::Kind::kUnit:
                    return llvm::Type::getVoidTy(context_);
                default:
                    throw std::runtime_error("Unsupported builtin type");
            }
        } else if constexpr (std::is_same_v<T, parser::UserType>) {
            throw std::runtime_error("User types not supported in codegen");
        } else {
            throw std::runtime_error("Unknown type");
        }
    }, type);
}

bool IRBuilderVisitor::isFloatingType(llvm::Type* ty) {
    return ty->isFloatTy() || ty->isDoubleTy();
}

void IRBuilderVisitor::pushScope() {
    scopes_.emplace_back();
}

void IRBuilderVisitor::popScope() {
    scopes_.pop_back();
}

llvm::AllocaInst* IRBuilderVisitor::lookupVariable(const std::string& name) {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end())
            return found->second;
    }
    throw std::runtime_error("Unknown variable: " + name);
}

llvm::Value* IRBuilderVisitor::loadVariable(const std::string& name) {
    auto* alloca = lookupVariable(name);
    return builder_.CreateLoad(alloca->getAllocatedType(), alloca, name + ".load");
}

llvm::AllocaInst* IRBuilderVisitor::createEntryBlockAlloca(llvm::Function* function,
                                                           llvm::Type* type,
                                                           const std::string& name) {
    llvm::IRBuilder<> tmp(&function->getEntryBlock(),
                          function->getEntryBlock().begin());
    return tmp.CreateAlloca(type, nullptr, name);
}

// Types
llvm::Value* IRBuilderVisitor::visitImpl(const parser::BuiltinType&) {
    throw std::runtime_error("Type node should not be visited directly");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::UserType&) {
    throw std::runtime_error("Type node should not be visited directly");
}

// Literals
llvm::Value* IRBuilderVisitor::visitImpl(const parser::IntLiteral& node) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_), node.value);
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::FltLiteral& node) {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(context_), node.value);
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::StrLiteral& node) {
    return builder_.CreateGlobalString(node.value, "str");
}

// Identifier
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Identificator& node) {
    return loadVariable(node.value);
}

// Arithmetic binary
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Addition& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFAdd(lhs, rhs, "addtmp");
    else
        return builder_.CreateAdd(lhs, rhs, "addtmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Subtraction& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFSub(lhs, rhs, "subtmp");
    else
        return builder_.CreateSub(lhs, rhs, "subtmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Multiplication& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFMul(lhs, rhs, "multmp");
    else
        return builder_.CreateMul(lhs, rhs, "multmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Division& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFDiv(lhs, rhs, "divtmp");
    else
        return builder_.CreateSDiv(lhs, rhs, "divtmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Remainder& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    return builder_.CreateSRem(lhs, rhs, "remtmp");
}

// Comparisons
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Equal& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFCmpOEQ(lhs, rhs, "eqtmp");
    else
        return builder_.CreateICmpEQ(lhs, rhs, "eqtmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::NotEqual& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFCmpONE(lhs, rhs, "netmp");
    else
        return builder_.CreateICmpNE(lhs, rhs, "netmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::LessThan& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFCmpOLT(lhs, rhs, "lttmp");
    else
        return builder_.CreateICmpSLT(lhs, rhs, "lttmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::LessEqual& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFCmpOLE(lhs, rhs, "letmp");
    else
        return builder_.CreateICmpSLE(lhs, rhs, "letmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::GreaterThan& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFCmpOGT(lhs, rhs, "gttmp");
    else
        return builder_.CreateICmpSGT(lhs, rhs, "gttmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::GreaterEqual& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    if (isFloatingType(lhs->getType()))
        return builder_.CreateFCmpOGE(lhs, rhs, "getmp");
    else
        return builder_.CreateICmpSGE(lhs, rhs, "getmp");
}

// Logical
llvm::Value* IRBuilderVisitor::visitImpl(const parser::And& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    return builder_.CreateAnd(lhs, rhs, "andtmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Or& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    return builder_.CreateOr(lhs, rhs, "ortmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Xor& node) {
    auto* lhs = visit(*node.left_operand);
    auto* rhs = visit(*node.right_operand);
    return builder_.CreateXor(lhs, rhs, "xortmp");
}

// Assign
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Assign& node) {
    auto* ident = std::get_if<parser::Identificator>(node.left_operand.get());
    if (!ident)
        throw std::runtime_error("Assignment target must be identifier");
    auto* alloca = lookupVariable(ident->value);
    auto* value = visit(*node.right_operand);
    builder_.CreateStore(value, alloca);
    return value;
}

// Unary
llvm::Value* IRBuilderVisitor::visitImpl(const parser::UnaryPlus& node) {
    return visit(*node.operand);
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::UnaryMinus& node) {
    auto* operand = visit(*node.operand);
    if (isFloatingType(operand->getType()))
        return builder_.CreateFNeg(operand, "negtmp");
    else
        return builder_.CreateNeg(operand, "negtmp");
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Not& node) {
    auto* operand = visit(*node.operand);
    return builder_.CreateNot(operand, "nottmp");
}

// Call
llvm::Value* IRBuilderVisitor::visitImpl(const parser::Call& node) {
    auto* ident = std::get_if<parser::Identificator>(node.callee.get());
    if (!ident)
        throw std::runtime_error("Unsupported callee");
    auto* function = module_->getFunction(ident->value);
    if (!function)
        throw std::runtime_error("Unknown function: " + ident->value);
    std::vector<llvm::Value*> args;
    for (const auto& arg : node.arguments)
        args.push_back(visit(*arg));
    return builder_.CreateCall(function, args);
}

// Statements
llvm::Value* IRBuilderVisitor::visitImpl(const parser::ExpressionStatement& node) {
    return visit(*node.expression);
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::ReturnStatement& node) {
    if (node.value.has_value())
        return builder_.CreateRet(visit(**node.value));
    else
        return builder_.CreateRetVoid();
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::VariableDeclaration& node) {
    auto* type = toLLVMType(node.type);
    if (type->isVoidTy())
        throw std::runtime_error("Cannot declare variable of void type");
    auto* alloca = createEntryBlockAlloca(current_function_, type, node.name);
    scopes_.back()[node.name] = alloca;
    return alloca;
}
llvm::Value* IRBuilderVisitor::visitImpl(const parser::ScopeStatement& node) {
    pushScope();
    for (const auto& stmt : node.statements)
        visit(*stmt);
    popScope();
    return nullptr;
}

llvm::Value* IRBuilderVisitor::visitImpl(const parser::IfStatement& node) {
    auto* cond = visit(*node.condition);
    auto* function = builder_.GetInsertBlock()->getParent();

    auto* thenBB = llvm::BasicBlock::Create(context_, "then", function);
    auto* elseBB = llvm::BasicBlock::Create(context_, "else");

    builder_.CreateCondBr(cond, thenBB, elseBB);

    builder_.SetInsertPoint(thenBB);
    visit(*node.then_branch);
    bool thenTerminated = builder_.GetInsertBlock()->getTerminator() != nullptr;

    function->insert(function->end(), elseBB);
    builder_.SetInsertPoint(elseBB);
    if (node.else_branch.has_value())
        visit(**node.else_branch);
    bool elseTerminated = builder_.GetInsertBlock()->getTerminator() != nullptr;

    if (thenTerminated && elseTerminated) {
        return nullptr;
    }

    auto* mergeBB = llvm::BasicBlock::Create(context_, "ifend", function);
    if (!thenTerminated) {
        builder_.SetInsertPoint(thenBB);
        builder_.CreateBr(mergeBB);
    }
    if (!elseTerminated) {
        builder_.SetInsertPoint(elseBB);
        builder_.CreateBr(mergeBB);
    }
    builder_.SetInsertPoint(mergeBB);
    return nullptr;
}

llvm::Value* IRBuilderVisitor::visitImpl(const parser::WhileStatement& node) {
    auto* function = builder_.GetInsertBlock()->getParent();
    auto* condBB = llvm::BasicBlock::Create(context_, "while.cond", function);
    auto* bodyBB = llvm::BasicBlock::Create(context_, "while.body");
    auto* endBB = llvm::BasicBlock::Create(context_, "while.end");
    
    builder_.CreateBr(condBB);
    
    builder_.SetInsertPoint(condBB);
    auto* cond = visit(*node.condition);
    builder_.CreateCondBr(cond, bodyBB, endBB);
    
    function->insert(function->end(), bodyBB);
    builder_.SetInsertPoint(bodyBB);
    visit(*node.body);
    if (builder_.GetInsertBlock()->getTerminator() == nullptr)
        builder_.CreateBr(condBB);
    
    function->insert(function->end(), endBB);
    builder_.SetInsertPoint(endBB);
    return nullptr;
}

llvm::Value* IRBuilderVisitor::visitImpl(const parser::Parameter&) {
    throw std::runtime_error("Parameter node should not be visited directly");
}

// Function
llvm::Value* IRBuilderVisitor::visitImpl(const parser::FunctionDeclaration& node) {
    std::vector<llvm::Type*> paramTypes;
    for (const auto& p : node.parameters)
        paramTypes.push_back(toLLVMType(p.type));
    llvm::Type* retType = node.return_type.has_value()
        ? toLLVMType(*node.return_type)
        : llvm::Type::getVoidTy(context_);
    auto* funcType = llvm::FunctionType::get(retType, paramTypes, false);
    auto* function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage,
                                            node.name, module_.get());
    
    current_function_ = function;
    size_t idx = 0;
    for (auto& arg : function->args()) {
        arg.setName(node.parameters[idx].name);
        ++idx;
    }

    auto* entryBB = llvm::BasicBlock::Create(context_, "entry", function);
    builder_.SetInsertPoint(entryBB);
    pushScope();
    idx = 0;
    for (auto& arg : function->args()) {
        auto* alloca = createEntryBlockAlloca(function, arg.getType(),
                                              std::string(arg.getName()));
        builder_.CreateStore(&arg, alloca);
        scopes_.back()[node.parameters[idx].name] = alloca;
        ++idx;
    }

    visitImpl(node.body);
    if (builder_.GetInsertBlock()->getTerminator() == nullptr) {
        if (retType->isVoidTy())
            builder_.CreateRetVoid();
    }

    llvm::verifyFunction(*function);
    popScope();
    return function;
}

} // namespace codegen