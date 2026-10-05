#pragma once

#include "ast/AST.h"
#include "common/Diagnostic.h"
#include "lexer/Token.h"
#include <memory>
#include <vector>

namespace cc {

class Parser {
public:
    Parser(const std::vector<Token>& tokens, DiagnosticEngine& diag);

    std::unique_ptr<TranslationUnit> parseTranslationUnit();

private:
    // Token stream navigation
    const Token& peek() const;
    const Token& previous() const;
    const Token& advance();
    bool isAtEnd() const;
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    const Token& expect(TokenKind kind, const std::string& message);

    bool isTypeName() const;

    // Declarations
    std::unique_ptr<FunctionDecl> parseFunctionDecl();
    std::vector<Parameter> parseParameterList();

    // Statements
    std::unique_ptr<Stmt> parseStatement();
    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<Stmt> parseIfStatement();
    std::unique_ptr<Stmt> parseWhileStatement();
    std::unique_ptr<Stmt> parseReturnStatement();
    std::unique_ptr<Stmt> parseVarDeclStatement();
    std::unique_ptr<Stmt> parseExpressionStatement();

    // Expressions (precedence climbing)
    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseAssignment();
    std::unique_ptr<Expr> parseBinary(int minPrecedence);
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parseCallExpr(const std::string& name,
                                         SourceLocation loc);

    // Precedence helpers
    int getBinaryPrecedence(TokenKind kind) const;
    BinaryOp tokenToBinaryOp(TokenKind kind) const;

    // Error recovery
    void synchronize();

    const std::vector<Token>& tokens_;
    DiagnosticEngine& diag_;
    size_t current_ = 0;
};

} // namespace cc
