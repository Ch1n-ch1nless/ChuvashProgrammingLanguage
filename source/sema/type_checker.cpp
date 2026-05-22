#include "sema/type_checker.hpp"
#include <cassert>
#include "utils/overload.hpp"

namespace parser::sema {

// ----------------------------------------------------------------------------
// TypeContext
// ----------------------------------------------------------------------------
bool TypeChecker::TypeContext::isConvertableTo(const TypeVariant& lhs, const TypeVariant& rhs) const {
    return std::visit(overloaded{
        [](const BuiltinType& l, const BuiltinType& r) -> bool {
            if (l.kind == r.kind) return true;
            if (l.kind == BuiltinType::Kind::kInt && r.kind == BuiltinType::Kind::kFloat) return true;
            return false;
        },
        [](const UserType& l, const UserType& r) -> bool { return l.name == r.name; },
        [](const auto&, const auto&) -> bool { return false; }
    }, lhs, rhs);
}

bool TypeChecker::TypeContext::areBinaryOperandsCompatible(const TypeVariant& left, const TypeVariant& right, Operation op) const {
    switch (op) {
        case Operation::kAddition: case Operation::kSubtraction: case Operation::kMultiplication:
        case Operation::kDivision: case Operation::kRemainder:
            return isArithmetic(left) && isArithmetic(right);
        case Operation::kEqual: case Operation::kNotEqual:
            return isComparable(left, right);
        case Operation::kLessThan: case Operation::kLessEqual:
        case Operation::kGreaterThan: case Operation::kGreaterEqual:
            return isArithmetic(left) && isArithmetic(right);
        case Operation::kAnd: case Operation::kOr:
            return isConvertibleToBool(left) && isConvertibleToBool(right);
        case Operation::kXor:
            return (isInteger(left) || isBool(left)) && (isInteger(right) || isBool(right));
        case Operation::kAssign:
            return isConvertableTo(right, left);
        default:
            return false;
    }
}

TypeVariant TypeChecker::TypeContext::getBinaryOperationResultType(const TypeVariant& left, const TypeVariant& right, Operation op) const {
    switch (op) {
        case Operation::kAddition: case Operation::kSubtraction:
        case Operation::kMultiplication: case Operation::kDivision:
            if (isFloat(left) || isFloat(right))
                return BuiltinType{BuiltinType::Kind::kFloat};
            else
                return BuiltinType{BuiltinType::Kind::kInt};
        case Operation::kRemainder:
            return BuiltinType{BuiltinType::Kind::kInt};
        case Operation::kEqual: case Operation::kNotEqual:
        case Operation::kLessThan: case Operation::kLessEqual:
        case Operation::kGreaterThan: case Operation::kGreaterEqual:
        case Operation::kAnd: case Operation::kOr: case Operation::kXor:
            return BuiltinType{BuiltinType::Kind::kBool};
        case Operation::kAssign:
            return left;
        default:
            return BuiltinType{BuiltinType::Kind::kUnit};
    }
}

bool TypeChecker::TypeContext::isArithmetic(const TypeVariant& t) const {
    return std::visit(overloaded{
        [](const BuiltinType& bt) -> bool {
            return bt.kind == BuiltinType::Kind::kInt || bt.kind == BuiltinType::Kind::kFloat;
        },
        [](const UserType&) { return false; }
    }, t);
}

bool TypeChecker::TypeContext::isFloat(const TypeVariant& t) const {
    return std::visit(overloaded{
        [](const BuiltinType& bt) -> bool { return bt.kind == BuiltinType::Kind::kFloat; },
        [](const UserType&) { return false; }
    }, t);
}

bool TypeChecker::TypeContext::isInteger(const TypeVariant& t) const {
    return std::visit(overloaded{
        [](const BuiltinType& bt) -> bool { return bt.kind == BuiltinType::Kind::kInt; },
        [](const UserType&) { return false; }
    }, t);
}

bool TypeChecker::TypeContext::isBool(const TypeVariant& t) const {
    return std::visit(overloaded{
        [](const BuiltinType& bt) -> bool { return bt.kind == BuiltinType::Kind::kBool; },
        [](const UserType&) { return false; }
    }, t);
}

bool TypeChecker::TypeContext::isConvertibleToBool(const TypeVariant& t) const {
    return std::visit(overloaded{
        [](const BuiltinType& bt) -> bool {
            return bt.kind == BuiltinType::Kind::kBool ||
                   bt.kind == BuiltinType::Kind::kInt ||
                   bt.kind == BuiltinType::Kind::kFloat ||
                   bt.kind == BuiltinType::Kind::kString;
        },
        [](const UserType&) { return false; }
    }, t);
}

bool TypeChecker::TypeContext::isComparable(const TypeVariant& a, const TypeVariant& b) const {
    return std::visit(overloaded{
        [&](const BuiltinType& la, const BuiltinType& rb) -> bool {
            if (la.kind == rb.kind) return true;
            if ((la.kind == BuiltinType::Kind::kInt && rb.kind == BuiltinType::Kind::kFloat) ||
                (la.kind == BuiltinType::Kind::kFloat && rb.kind == BuiltinType::Kind::kInt))
                return true;
            return false;
        },
        [](const UserType& lu, const UserType& ru) -> bool { return lu.name == ru.name; },
        [](const auto&, const auto&) { return false; }
    }, a, b);
}

// ----------------------------------------------------------------------------
// convertTypeToOperation
// ----------------------------------------------------------------------------
TypeChecker::Operation TypeChecker::convertTypeToOperation(const ExpressionVariant& expr) {
    return std::visit(overloaded{
        [](const Addition&)       -> Operation { return Operation::kAddition; },
        [](const Subtraction&)    -> Operation { return Operation::kSubtraction; },
        [](const Multiplication&) -> Operation { return Operation::kMultiplication; },
        [](const Division&)       -> Operation { return Operation::kDivision; },
        [](const Remainder&)      -> Operation { return Operation::kRemainder; },
        [](const Equal&)          -> Operation { return Operation::kEqual; },
        [](const NotEqual&)       -> Operation { return Operation::kNotEqual; },
        [](const LessThan&)       -> Operation { return Operation::kLessThan; },
        [](const LessEqual&)      -> Operation { return Operation::kLessEqual; },
        [](const GreaterThan&)    -> Operation { return Operation::kGreaterThan; },
        [](const GreaterEqual&)   -> Operation { return Operation::kGreaterEqual; },
        [](const And&)            -> Operation { return Operation::kAnd; },
        [](const Or&)             -> Operation { return Operation::kOr; },
        [](const Xor&)            -> Operation { return Operation::kXor; },
        [](const Assign&)         -> Operation { return Operation::kAssign; },
        [](const UnaryPlus&)      -> Operation { return Operation::kUnaryPlus; },
        [](const UnaryMinus&)     -> Operation { return Operation::kUnaryMinus; },
        [](const Not&)            -> Operation { return Operation::kNot; },
        [](const auto&) {
            assert(false);
            return Operation::kAddition;
        }
    }, expr);
}

// ----------------------------------------------------------------------------
// Types and literals
// ----------------------------------------------------------------------------
ExpectedType TypeChecker::visitImpl(const BuiltinType& type) {
    return type;
}

ExpectedType TypeChecker::visitImpl(const UserType& type) {
    return type;
}

ExpectedType TypeChecker::visitImpl(const IntLiteral&) {
    return BuiltinType{BuiltinType::Kind::kInt};
}

ExpectedType TypeChecker::visitImpl(const FltLiteral&) {
    return BuiltinType{BuiltinType::Kind::kFloat};
}

ExpectedType TypeChecker::visitImpl(const StrLiteral&) {
    return BuiltinType{BuiltinType::Kind::kString};
}

ExpectedType TypeChecker::visitImpl(const Identificator& id) {
    auto* sym = current_scope_->Lookup(id.value);
    if (!sym) return std::unexpected("Undefined identifier: " + id.value);
    return sym->type;
}

ExpectedType TypeChecker::visitImpl(const Assign& op) {
    if (!std::holds_alternative<Identificator>(*op.left_operand)) {
        return std::unexpected("Left-hand side of assignment must be an identifier");
    }
    const auto& id = std::get<Identificator>(*op.left_operand);
    auto* sym = current_scope_->Lookup(id.value);
    if (!sym) return std::unexpected("Undefined variable: " + id.value);

    auto right_type = visit(*op.right_operand);
    if (!right_type) return std::unexpected(right_type.error());

    Operation op_kind = Operation::kAssign;
    if (type_context_.areBinaryOperandsCompatible(sym->type, *right_type, op_kind)) {
        return sym->type;
    }
    return std::unexpected("Assignment type mismatch");
}

ExpectedType TypeChecker::visitImpl(const Call& call) {
    if (!std::holds_alternative<Identificator>(*call.callee)) {
        return std::unexpected("Function call callee must be an identifier");
    }
    const auto& func_name = std::get<Identificator>(*call.callee);
    auto* sym = current_scope_->Lookup(func_name.value);
    if (!sym) return std::unexpected("Undefined function: " + func_name.value);
    if (sym->kind != symbols::SymbolInfo::SymbolKind::kFunction) {
        return std::unexpected("'" + func_name.value + "' is not a function");
    }
    return sym->type;
}

ExpectedType TypeChecker::visitImpl(const ReturnStatement& ret) {
    if (ret.value) {
        auto val_type = visit(**ret.value);
        if (!val_type) return std::unexpected(val_type.error());
    }
    return BuiltinType{BuiltinType::Kind::kUnit};
}

ExpectedType TypeChecker::visitImpl(const ExpressionStatement& stmt) {
    auto expr_type = visit(*stmt.expression);
    if (!expr_type) return std::unexpected(expr_type.error());
    return BuiltinType{BuiltinType::Kind::kUnit};
}

ExpectedType TypeChecker::visitImpl(const IfStatement& stmt) {
    auto cond_type = visit(*stmt.condition);
    if (!cond_type) return std::unexpected(cond_type.error());
    if (!type_context_.isConvertibleToBool(*cond_type)) {
        return std::unexpected("If condition must be convertible to bool");
    }
    auto then_type = visit(*stmt.then_branch);
    if (!then_type) return std::unexpected(then_type.error());
    if (stmt.else_branch) {
        auto else_type = visit(**stmt.else_branch);
        if (!else_type) return std::unexpected(else_type.error());
    }
    return BuiltinType{BuiltinType::Kind::kUnit};
}

ExpectedType TypeChecker::visitImpl(const WhileStatement& stmt) {
    auto cond_type = visit(*stmt.condition);
    if (!cond_type) return std::unexpected(cond_type.error());
    if (!type_context_.isConvertibleToBool(*cond_type)) {
        return std::unexpected("While condition must be convertible to bool");
    }
    auto body_type = visit(*stmt.body);
    if (!body_type) return std::unexpected(body_type.error());
    return BuiltinType{BuiltinType::Kind::kUnit};
}

ExpectedType TypeChecker::visitImpl(const VariableDeclaration& decl) {
    auto type_check = visit(decl.type);
    if (!type_check) return std::unexpected(type_check.error());
    return BuiltinType{BuiltinType::Kind::kUnit};
}

ExpectedType TypeChecker::visitImpl(const ScopeStatement& stmt) {
    symbols::Scope* prev_scope = current_scope_;
    //if (stmt.scope) current_scope_ = stmt.scope;
    for (const auto& s : stmt.statements) {
        auto res = visit(*s);
        if (!res) {
            current_scope_ = prev_scope;
            return std::unexpected(res.error());
        }
    }
    current_scope_ = prev_scope;
    return BuiltinType{BuiltinType::Kind::kUnit};
}

ExpectedType TypeChecker::visitImpl(const Parameter& param) {
    auto type_check = visit(param.type);
    if (!type_check) return std::unexpected(type_check.error());
    return BuiltinType{BuiltinType::Kind::kUnit};
}

ExpectedType TypeChecker::visitImpl(const FunctionDeclaration& func_decl) {
    symbols::Scope* prev_scope = current_scope_;
    //if (func_decl.scope) current_scope_ = func_decl.scope;

    for (const auto& param : func_decl.parameters) {
        auto res = visit(param);
        if (!res) {
            current_scope_ = prev_scope;
            return std::unexpected(res.error());
        }
    }

    auto body_res = visit(func_decl.body);
    if (!body_res) {
        current_scope_ = prev_scope;
        return std::unexpected(body_res.error());
    }

    current_scope_ = prev_scope;
    if (func_decl.return_type) return *func_decl.return_type;
    return BuiltinType{BuiltinType::Kind::kUnit};
}

} // namespace parser::sema