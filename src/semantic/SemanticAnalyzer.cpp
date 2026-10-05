#include "semantic/SemanticAnalyzer.h"

namespace cc {

SemanticAnalyzer::SemanticAnalyzer(DiagnosticEngine& diag) : diag_(diag) {}

// ============================================================================
// Entry point
// ============================================================================

bool SemanticAnalyzer::analyze(TranslationUnit& unit) {
    // Create the global scope.
    auto global = std::make_unique<Scope>(nullptr);
    globalScope_ = global.get();
    currentScope_ = globalScope_;
    ownedScopes_.push_back(std::move(global));

    // Phase 1: register all function signatures so that forward calls work.
    for (auto& fn : unit.functions) {
        registerFunction(*fn);
    }

    // Phase 2: analyze every function body.
    for (auto& fn : unit.functions) {
        analyzeFunction(*fn);
    }

    return !diag_.hasErrors();
}

// ============================================================================
// Visitor stubs for TranslationUnit / FunctionDecl
// (traversal is driven by analyze(), not by accept())
// ============================================================================

void SemanticAnalyzer::visit(TranslationUnit&) {}
void SemanticAnalyzer::visit(FunctionDecl&) {}

// ============================================================================
// Phase 1 — function registration
// ============================================================================

void SemanticAnalyzer::registerFunction(FunctionDecl& node) {
    const Type* retType = resolveType(node.returnType, node.location);

    std::vector<const Type*> paramTypes;
    std::vector<std::string> paramNames;

    for (auto& param : node.parameters) {
        const Type* pt = resolveType(param.typeName, param.location);
        if (pt->isVoid()) {
            diag_.error(param.location,
                        "parameter '" + param.name + "' cannot have type 'void'");
            pt = &Types::Error;
        }
        paramTypes.push_back(pt);
        paramNames.push_back(param.name);
    }

    auto sym = std::make_unique<FunctionSymbol>(
        node.name, retType, std::move(paramTypes), std::move(paramNames),
        node.location);

    node.resolvedSymbol = sym.get();

    if (!globalScope_->declare(node.name, sym.get())) {
        diag_.error(node.location,
                    "redefinition of function '" + node.name + "'");
    }

    ownedSymbols_.push_back(std::move(sym));
}

// ============================================================================
// Phase 2 — function body analysis
// ============================================================================

void SemanticAnalyzer::analyzeFunction(FunctionDecl& node) {
    currentFunction_ = node.resolvedSymbol;

    // The function scope is a child of global. Parameters and top-level
    // body declarations share this scope, matching C++ rules where
    // re-declaring a parameter name in the outermost body block is illegal.
    pushScope();

    for (size_t i = 0; i < node.parameters.size(); ++i) {
        auto& param = node.parameters[i];
        const Type* pt = currentFunction_->parameterTypes[i];
        if (pt->isError()) continue;

        auto sym = std::make_unique<ParameterSymbol>(
            param.name, pt, param.location);

        param.resolvedSymbol = sym.get();

        if (!currentScope_->declare(param.name, sym.get())) {
            diag_.error(param.location,
                        "duplicate parameter name '" + param.name + "'");
        }

        ownedSymbols_.push_back(std::move(sym));
    }

    // Analyze body statements directly — DO NOT push another scope for the
    // outermost block, so parameters and body-level declarations share one
    // scope.  Nested blocks inside the body will push their own scopes.
    for (auto& stmt : node.body->statements) {
        stmt->accept(*this);
    }

    popScope();
    currentFunction_ = nullptr;
}

// ============================================================================
// Scope helpers
// ============================================================================

void SemanticAnalyzer::pushScope() {
    auto scope = std::make_unique<Scope>(currentScope_);
    currentScope_ = scope.get();
    ownedScopes_.push_back(std::move(scope));
}

void SemanticAnalyzer::popScope() {
    currentScope_ = currentScope_->parent();
}

const Type* SemanticAnalyzer::resolveType(const std::string& name,
                                           const SourceLocation& loc) {
    const Type* t = Types::fromString(name);
    if (!t) {
        diag_.error(loc, "unknown type '" + name + "'");
        return &Types::Error;
    }
    return t;
}

const Type* SemanticAnalyzer::analyzeExpr(Expr& expr) {
    exprResult_ = &Types::Error;
    expr.accept(*this);
    return exprResult_;
}

// ============================================================================
// Statement visitors
// ============================================================================

void SemanticAnalyzer::visit(BlockStmt& node) {
    pushScope();
    for (auto& stmt : node.statements) {
        stmt->accept(*this);
    }
    popScope();
}

void SemanticAnalyzer::visit(IfStmt& node) {
    const Type* condType = analyzeExpr(*node.condition);
    if (!condType->isError() && !condType->isBool()) {
        diag_.error(node.condition->location,
                    "condition must have type 'bool', found '" +
                    std::string(condType->toString()) + "'");
    }

    node.thenBranch->accept(*this);

    if (node.elseBranch) {
        node.elseBranch->accept(*this);
    }
}

void SemanticAnalyzer::visit(WhileStmt& node) {
    const Type* condType = analyzeExpr(*node.condition);
    if (!condType->isError() && !condType->isBool()) {
        diag_.error(node.condition->location,
                    "condition must have type 'bool', found '" +
                    std::string(condType->toString()) + "'");
    }

    node.body->accept(*this);
}

void SemanticAnalyzer::visit(ReturnStmt& node) {
    const Type* retType = currentFunction_->returnType();

    if (node.value) {
        const Type* valType = analyzeExpr(*node.value);
        if (!valType->isError() && !retType->isError() && *valType != *retType) {
            diag_.error(node.location,
                        "cannot return value of type '" +
                        std::string(valType->toString()) +
                        "' from function returning '" +
                        std::string(retType->toString()) + "'");
        }
    } else {
        if (!retType->isVoid() && !retType->isError()) {
            diag_.error(node.location,
                        "non-void function '" + currentFunction_->name() +
                        "' must return a value");
        }
    }
}

void SemanticAnalyzer::visit(ExpressionStmt& node) {
    analyzeExpr(*node.expression);
}

void SemanticAnalyzer::visit(VarDeclStmt& node) {
    const Type* declType = resolveType(node.typeName, node.location);

    if (declType->isVoid()) {
        diag_.error(node.location,
                    "variable '" + node.varName + "' cannot have type 'void'");
        declType = &Types::Error;
    }

    if (node.initializer) {
        const Type* initType = analyzeExpr(*node.initializer);
        if (!initType->isError() && !declType->isError() &&
            *initType != *declType) {
            diag_.error(node.initializer->location,
                        "cannot initialize variable '" + node.varName +
                        "' of type '" + std::string(declType->toString()) +
                        "' with value of type '" +
                        std::string(initType->toString()) + "'");
        }
    }

    auto sym = std::make_unique<VariableSymbol>(
        node.varName, declType, node.location);
    node.resolvedSymbol = sym.get();

    if (!currentScope_->declare(node.varName, sym.get())) {
        diag_.error(node.location,
                    "redefinition of variable '" + node.varName + "'");
    }

    ownedSymbols_.push_back(std::move(sym));
}

// ============================================================================
// Expression visitors
// ============================================================================

void SemanticAnalyzer::visit(IntegerLiteralExpr& node) {
    node.resolvedType = &Types::Int;
    exprResult_ = &Types::Int;
}

void SemanticAnalyzer::visit(BooleanLiteralExpr& node) {
    node.resolvedType = &Types::Bool;
    exprResult_ = &Types::Bool;
}

void SemanticAnalyzer::visit(IdentifierExpr& node) {
    Symbol* sym = currentScope_->lookup(node.name);
    if (!sym) {
        diag_.error(node.location,
                    "use of undeclared identifier '" + node.name + "'");
        node.resolvedType = &Types::Error;
        exprResult_ = &Types::Error;
        return;
    }

    node.resolvedSymbol = sym;
    node.resolvedType = sym->type();
    exprResult_ = sym->type();
}

void SemanticAnalyzer::visit(BinaryExpr& node) {
    const Type* leftType = analyzeExpr(*node.left);
    const Type* rightType = analyzeExpr(*node.right);

    if (leftType->isError() || rightType->isError()) {
        node.resolvedType = &Types::Error;
        exprResult_ = &Types::Error;
        return;
    }

    const char* opStr = binaryOpToString(node.op);

    switch (node.op) {
        // Arithmetic: int OP int → int
        case BinaryOp::Add:
        case BinaryOp::Sub:
        case BinaryOp::Mul:
        case BinaryOp::Div:
        case BinaryOp::Mod:
            if (!leftType->isInt() || !rightType->isInt()) {
                diag_.error(node.location,
                            std::string("invalid operands to '") + opStr +
                            "': '" + leftType->toString() + "' and '" +
                            rightType->toString() + "'");
                node.resolvedType = &Types::Error;
                exprResult_ = &Types::Error;
                return;
            }
            node.resolvedType = &Types::Int;
            exprResult_ = &Types::Int;
            return;

        // Relational: int OP int → bool
        case BinaryOp::Less:
        case BinaryOp::LessEqual:
        case BinaryOp::Greater:
        case BinaryOp::GreaterEqual:
            if (!leftType->isInt() || !rightType->isInt()) {
                diag_.error(node.location,
                            std::string("invalid operands to '") + opStr +
                            "': '" + leftType->toString() + "' and '" +
                            rightType->toString() + "'");
                node.resolvedType = &Types::Error;
                exprResult_ = &Types::Error;
                return;
            }
            node.resolvedType = &Types::Bool;
            exprResult_ = &Types::Bool;
            return;

        // Equality: same non-void type OP same type → bool
        case BinaryOp::Equal:
        case BinaryOp::NotEqual:
            if (*leftType != *rightType || leftType->isVoid()) {
                diag_.error(node.location,
                            std::string("invalid operands to '") + opStr +
                            "': '" + leftType->toString() + "' and '" +
                            rightType->toString() + "'");
                node.resolvedType = &Types::Error;
                exprResult_ = &Types::Error;
                return;
            }
            node.resolvedType = &Types::Bool;
            exprResult_ = &Types::Bool;
            return;

        // Logical: bool OP bool → bool
        case BinaryOp::LogicalAnd:
        case BinaryOp::LogicalOr:
            if (!leftType->isBool() || !rightType->isBool()) {
                diag_.error(node.location,
                            std::string("invalid operands to '") + opStr +
                            "': '" + leftType->toString() + "' and '" +
                            rightType->toString() + "'");
                node.resolvedType = &Types::Error;
                exprResult_ = &Types::Error;
                return;
            }
            node.resolvedType = &Types::Bool;
            exprResult_ = &Types::Bool;
            return;
    }
}

void SemanticAnalyzer::visit(UnaryExpr& node) {
    const Type* operandType = analyzeExpr(*node.operand);

    if (operandType->isError()) {
        node.resolvedType = &Types::Error;
        exprResult_ = &Types::Error;
        return;
    }

    switch (node.op) {
        case UnaryOp::Negate:
            if (!operandType->isInt()) {
                diag_.error(node.location,
                            "invalid operand to unary '-': '" +
                            std::string(operandType->toString()) + "'");
                node.resolvedType = &Types::Error;
                exprResult_ = &Types::Error;
                return;
            }
            node.resolvedType = &Types::Int;
            exprResult_ = &Types::Int;
            return;

        case UnaryOp::LogicalNot:
            if (!operandType->isBool()) {
                diag_.error(node.location,
                            "invalid operand to '!': '" +
                            std::string(operandType->toString()) + "'");
                node.resolvedType = &Types::Error;
                exprResult_ = &Types::Error;
                return;
            }
            node.resolvedType = &Types::Bool;
            exprResult_ = &Types::Bool;
            return;
    }
}

void SemanticAnalyzer::visit(AssignmentExpr& node) {
    Symbol* sym = currentScope_->lookup(node.target);
    if (!sym) {
        diag_.error(node.location,
                    "use of undeclared identifier '" + node.target + "'");
        analyzeExpr(*node.value);
        node.resolvedType = &Types::Error;
        exprResult_ = &Types::Error;
        return;
    }

    if (sym->symbolKind() == SymbolKind::Function) {
        diag_.error(node.location,
                    "cannot assign to function '" + node.target + "'");
        analyzeExpr(*node.value);
        node.resolvedType = &Types::Error;
        exprResult_ = &Types::Error;
        return;
    }

    node.resolvedSymbol = sym;
    const Type* targetType = sym->type();

    const Type* valueType = analyzeExpr(*node.value);

    if (!valueType->isError() && !targetType->isError() &&
        *valueType != *targetType) {
        diag_.error(node.location,
                    "cannot assign value of type '" +
                    std::string(valueType->toString()) +
                    "' to variable '" + node.target + "' of type '" +
                    std::string(targetType->toString()) + "'");
        node.resolvedType = &Types::Error;
        exprResult_ = &Types::Error;
        return;
    }

    node.resolvedType = targetType;
    exprResult_ = targetType;
}

void SemanticAnalyzer::visit(CallExpr& node) {
    Symbol* sym = globalScope_->lookupLocal(node.callee);
    if (!sym) {
        diag_.error(node.location,
                    "call to undeclared function '" + node.callee + "'");
        for (auto& arg : node.arguments) {
            analyzeExpr(*arg);
        }
        node.resolvedType = &Types::Error;
        exprResult_ = &Types::Error;
        return;
    }

    if (sym->symbolKind() != SymbolKind::Function) {
        diag_.error(node.location,
                    "'" + node.callee + "' is not a function");
        for (auto& arg : node.arguments) {
            analyzeExpr(*arg);
        }
        node.resolvedType = &Types::Error;
        exprResult_ = &Types::Error;
        return;
    }

    auto* fnSym = static_cast<FunctionSymbol*>(sym);
    node.resolvedFunction = fnSym;

    if (node.arguments.size() != fnSym->parameterTypes.size()) {
        diag_.error(node.location,
                    "function '" + node.callee + "' expects " +
                    std::to_string(fnSym->parameterTypes.size()) +
                    " argument(s) but " +
                    std::to_string(node.arguments.size()) +
                    " provided");
        for (auto& arg : node.arguments) {
            analyzeExpr(*arg);
        }
        node.resolvedType = fnSym->returnType();
        exprResult_ = fnSym->returnType();
        return;
    }

    for (size_t i = 0; i < node.arguments.size(); ++i) {
        const Type* argType = analyzeExpr(*node.arguments[i]);
        const Type* paramType = fnSym->parameterTypes[i];

        if (!argType->isError() && !paramType->isError() &&
            *argType != *paramType) {
            diag_.error(node.arguments[i]->location,
                        "argument " + std::to_string(i + 1) +
                        " to function '" + node.callee + "' expects '" +
                        std::string(paramType->toString()) +
                        "' but received '" +
                        std::string(argType->toString()) + "'");
        }
    }

    const Type* retType = fnSym->returnType();
    node.resolvedType = retType;
    exprResult_ = retType;
}

} // namespace cc
