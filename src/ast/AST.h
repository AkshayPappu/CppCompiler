#pragma once

#include "common/SourceLocation.h"
#include <memory>
#include <string>
#include <vector>

namespace cc {

// ============================================================================
// Forward declarations
// ============================================================================

class Expr;
class BinaryExpr;
class UnaryExpr;
class AssignmentExpr;
class CallExpr;
class IdentifierExpr;
class IntegerLiteralExpr;
class BooleanLiteralExpr;

class Stmt;
class BlockStmt;
class IfStmt;
class WhileStmt;
class ReturnStmt;
class ExpressionStmt;
class VarDeclStmt;

class FunctionDecl;
class TranslationUnit;

// ============================================================================
// Visitor interface
// ============================================================================

class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    virtual void visit(BinaryExpr& node) = 0;
    virtual void visit(UnaryExpr& node) = 0;
    virtual void visit(AssignmentExpr& node) = 0;
    virtual void visit(CallExpr& node) = 0;
    virtual void visit(IdentifierExpr& node) = 0;
    virtual void visit(IntegerLiteralExpr& node) = 0;
    virtual void visit(BooleanLiteralExpr& node) = 0;

    virtual void visit(BlockStmt& node) = 0;
    virtual void visit(IfStmt& node) = 0;
    virtual void visit(WhileStmt& node) = 0;
    virtual void visit(ReturnStmt& node) = 0;
    virtual void visit(ExpressionStmt& node) = 0;
    virtual void visit(VarDeclStmt& node) = 0;

    virtual void visit(FunctionDecl& node) = 0;
    virtual void visit(TranslationUnit& node) = 0;
};

// ============================================================================
// Base AST node
// ============================================================================

class ASTNode {
public:
    SourceLocation location;

    virtual ~ASTNode() = default;
    virtual void accept(ASTVisitor& visitor) = 0;

protected:
    ASTNode() = default;
    explicit ASTNode(SourceLocation loc) : location(std::move(loc)) {}
};

// ============================================================================
// Enums
// ============================================================================

enum class BinaryOp {
    Add, Sub, Mul, Div, Mod,
    Equal, NotEqual,
    Less, LessEqual, Greater, GreaterEqual,
    LogicalAnd, LogicalOr
};

enum class UnaryOp {
    Negate,    // -
    LogicalNot // !
};

inline const char* binaryOpToString(BinaryOp op) {
    switch (op) {
        case BinaryOp::Add:          return "+";
        case BinaryOp::Sub:          return "-";
        case BinaryOp::Mul:          return "*";
        case BinaryOp::Div:          return "/";
        case BinaryOp::Mod:          return "%";
        case BinaryOp::Equal:        return "==";
        case BinaryOp::NotEqual:     return "!=";
        case BinaryOp::Less:         return "<";
        case BinaryOp::LessEqual:    return "<=";
        case BinaryOp::Greater:      return ">";
        case BinaryOp::GreaterEqual: return ">=";
        case BinaryOp::LogicalAnd:   return "&&";
        case BinaryOp::LogicalOr:    return "||";
    }
    return "?";
}

inline const char* unaryOpToString(UnaryOp op) {
    switch (op) {
        case UnaryOp::Negate:     return "-";
        case UnaryOp::LogicalNot: return "!";
    }
    return "?";
}

// ============================================================================
// Expressions
// ============================================================================

class Expr : public ASTNode {
public:
    using ASTNode::ASTNode;
};

class BinaryExpr : public Expr {
public:
    BinaryOp op;
    std::unique_ptr<Expr> left;
    std::unique_ptr<Expr> right;

    BinaryExpr(SourceLocation loc, BinaryOp op,
               std::unique_ptr<Expr> left, std::unique_ptr<Expr> right)
        : Expr(std::move(loc)), op(op),
          left(std::move(left)), right(std::move(right)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class UnaryExpr : public Expr {
public:
    UnaryOp op;
    std::unique_ptr<Expr> operand;

    UnaryExpr(SourceLocation loc, UnaryOp op, std::unique_ptr<Expr> operand)
        : Expr(std::move(loc)), op(op), operand(std::move(operand)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class AssignmentExpr : public Expr {
public:
    std::string target;
    std::unique_ptr<Expr> value;

    AssignmentExpr(SourceLocation loc, std::string target,
                   std::unique_ptr<Expr> value)
        : Expr(std::move(loc)), target(std::move(target)),
          value(std::move(value)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class CallExpr : public Expr {
public:
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;

    CallExpr(SourceLocation loc, std::string callee,
             std::vector<std::unique_ptr<Expr>> arguments)
        : Expr(std::move(loc)), callee(std::move(callee)),
          arguments(std::move(arguments)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class IdentifierExpr : public Expr {
public:
    std::string name;

    IdentifierExpr(SourceLocation loc, std::string name)
        : Expr(std::move(loc)), name(std::move(name)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class IntegerLiteralExpr : public Expr {
public:
    int64_t value;

    IntegerLiteralExpr(SourceLocation loc, int64_t value)
        : Expr(std::move(loc)), value(value) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class BooleanLiteralExpr : public Expr {
public:
    bool value;

    BooleanLiteralExpr(SourceLocation loc, bool value)
        : Expr(std::move(loc)), value(value) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

// ============================================================================
// Statements
// ============================================================================

class Stmt : public ASTNode {
public:
    using ASTNode::ASTNode;
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;

    BlockStmt(SourceLocation loc, std::vector<std::unique_ptr<Stmt>> stmts)
        : Stmt(std::move(loc)), statements(std::move(stmts)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch; // may be nullptr

    IfStmt(SourceLocation loc, std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch = nullptr)
        : Stmt(std::move(loc)), condition(std::move(condition)),
          thenBranch(std::move(thenBranch)),
          elseBranch(std::move(elseBranch)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(SourceLocation loc, std::unique_ptr<Expr> condition,
              std::unique_ptr<Stmt> body)
        : Stmt(std::move(loc)), condition(std::move(condition)),
          body(std::move(body)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class ReturnStmt : public Stmt {
public:
    std::unique_ptr<Expr> value; // may be nullptr for bare `return;`

    ReturnStmt(SourceLocation loc, std::unique_ptr<Expr> value = nullptr)
        : Stmt(std::move(loc)), value(std::move(value)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class ExpressionStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;

    ExpressionStmt(SourceLocation loc, std::unique_ptr<Expr> expr)
        : Stmt(std::move(loc)), expression(std::move(expr)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

class VarDeclStmt : public Stmt {
public:
    std::string typeName;
    std::string varName;
    std::unique_ptr<Expr> initializer; // may be nullptr

    VarDeclStmt(SourceLocation loc, std::string typeName, std::string varName,
                std::unique_ptr<Expr> init = nullptr)
        : Stmt(std::move(loc)), typeName(std::move(typeName)),
          varName(std::move(varName)), initializer(std::move(init)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

// ============================================================================
// Declarations
// ============================================================================

struct Parameter {
    std::string typeName;
    std::string name;
    SourceLocation location;
};

class FunctionDecl : public ASTNode {
public:
    std::string returnType;
    std::string name;
    std::vector<Parameter> parameters;
    std::unique_ptr<BlockStmt> body;

    FunctionDecl(SourceLocation loc, std::string returnType, std::string name,
                 std::vector<Parameter> params, std::unique_ptr<BlockStmt> body)
        : ASTNode(std::move(loc)), returnType(std::move(returnType)),
          name(std::move(name)), parameters(std::move(params)),
          body(std::move(body)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

// ============================================================================
// Translation unit (root of the AST)
// ============================================================================

class TranslationUnit : public ASTNode {
public:
    std::vector<std::unique_ptr<FunctionDecl>> functions;

    TranslationUnit() = default;
    explicit TranslationUnit(std::vector<std::unique_ptr<FunctionDecl>> funcs)
        : functions(std::move(funcs)) {}

    void accept(ASTVisitor& v) override { v.visit(*this); }
};

} // namespace cc
