#pragma once

#include "ast/AST.h"
#include "common/Diagnostic.h"
#include "semantic/Scope.h"
#include "semantic/Symbol.h"
#include "semantic/Type.h"
#include <memory>
#include <vector>

namespace cc {

class SemanticAnalyzer : public ASTVisitor {
public:
    explicit SemanticAnalyzer(DiagnosticEngine& diag);

    // Entry point. Returns true if no semantic errors were found.
    bool analyze(TranslationUnit& unit);

    // ASTVisitor overrides
    void visit(TranslationUnit& node) override;
    void visit(FunctionDecl& node) override;

    void visit(BlockStmt& node) override;
    void visit(IfStmt& node) override;
    void visit(WhileStmt& node) override;
    void visit(ReturnStmt& node) override;
    void visit(ExpressionStmt& node) override;
    void visit(VarDeclStmt& node) override;

    void visit(BinaryExpr& node) override;
    void visit(UnaryExpr& node) override;
    void visit(AssignmentExpr& node) override;
    void visit(CallExpr& node) override;
    void visit(IdentifierExpr& node) override;
    void visit(IntegerLiteralExpr& node) override;
    void visit(BooleanLiteralExpr& node) override;

private:
    // Phase 1: register all function signatures in the global scope.
    void registerFunction(FunctionDecl& node);

    // Phase 2: analyze a single function body.
    void analyzeFunction(FunctionDecl& node);

    // Resolves a type-name string from the parser into a semantic Type.
    // This is the single point where string-based type lookup occurs.
    const Type* resolveType(const std::string& name,
                            const SourceLocation& loc);

    // Expression analysis wrapper: dispatches via the visitor and
    // returns the resolved type through exprResult_.
    const Type* analyzeExpr(Expr& expr);

    // Scope management
    void pushScope();
    void popScope();

    DiagnosticEngine& diag_;

    // Scope state
    Scope* globalScope_ = nullptr;
    Scope* currentScope_ = nullptr;
    std::vector<std::unique_ptr<Scope>> ownedScopes_;

    // Symbol ownership — AST nodes hold non-owning pointers back into these.
    std::vector<std::unique_ptr<Symbol>> ownedSymbols_;

    // Analysis state
    const FunctionSymbol* currentFunction_ = nullptr;

    // Return channel for expression visitors (void visit → member variable).
    const Type* exprResult_ = nullptr;
};

} // namespace cc
