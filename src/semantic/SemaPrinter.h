#pragma once

#include "ast/AST.h"
#include "semantic/Symbol.h"
#include "semantic/Type.h"
#include <iostream>
#include <string>

namespace cc {

// Prints the AST annotated with semantic information (resolved types,
// symbols, and function resolutions) produced by SemanticAnalyzer.
class SemaPrinter : public ASTVisitor {
public:
    explicit SemaPrinter(std::ostream& out = std::cout) : out_(out) {}

    void print(ASTNode& node) { node.accept(*this); }

    // --- Top level -----------------------------------------------------------

    void visit(TranslationUnit& node) override {
        out_ << "=== Semantic Analysis ===\n";
        for (auto& fn : node.functions) {
            fn->accept(*this);
        }
    }

    void visit(FunctionDecl& node) override {
        printIndent();
        out_ << "Function: " << node.returnType << " " << node.name << "(";
        for (size_t i = 0; i < node.parameters.size(); ++i) {
            if (i > 0) out_ << ", ";
            out_ << node.parameters[i].typeName << " "
                 << node.parameters[i].name;
        }
        out_ << ")\n";
        indent();

        if (!node.parameters.empty()) {
            printIndent();
            out_ << "Params:\n";
            indent();
            for (auto& p : node.parameters) {
                printIndent();
                auto* pt = Types::fromString(p.typeName);
                out_ << p.name << " : "
                     << (pt ? pt->toString() : p.typeName) << " [param]\n";
            }
            dedent();
        }

        printIndent();
        out_ << "Body:\n";
        indent();
        for (auto& stmt : node.body->statements) {
            stmt->accept(*this);
        }
        dedent();
        dedent();
    }

    // --- Statements ----------------------------------------------------------

    void visit(BlockStmt& node) override {
        printIndent();
        out_ << "BlockStmt\n";
        indent();
        for (auto& stmt : node.statements) {
            stmt->accept(*this);
        }
        dedent();
    }

    void visit(IfStmt& node) override {
        printIndent();
        out_ << "IfStmt\n";
        indent();

        printIndent();
        out_ << "Condition" << typeAnnotation(node.condition.get()) << ":\n";
        indent();
        node.condition->accept(*this);
        dedent();

        printIndent();
        out_ << "Then:\n";
        indent();
        node.thenBranch->accept(*this);
        dedent();

        if (node.elseBranch) {
            printIndent();
            out_ << "Else:\n";
            indent();
            node.elseBranch->accept(*this);
            dedent();
        }
        dedent();
    }

    void visit(WhileStmt& node) override {
        printIndent();
        out_ << "WhileStmt\n";
        indent();

        printIndent();
        out_ << "Condition" << typeAnnotation(node.condition.get()) << ":\n";
        indent();
        node.condition->accept(*this);
        dedent();

        printIndent();
        out_ << "Body:\n";
        indent();
        node.body->accept(*this);
        dedent();

        dedent();
    }

    void visit(ReturnStmt& node) override {
        printIndent();
        out_ << "ReturnStmt\n";
        if (node.value) {
            indent();
            node.value->accept(*this);
            dedent();
        }
    }

    void visit(ExpressionStmt& node) override {
        printIndent();
        out_ << "ExpressionStmt\n";
        indent();
        node.expression->accept(*this);
        dedent();
    }

    void visit(VarDeclStmt& node) override {
        printIndent();
        out_ << "VarDeclStmt: " << node.typeName << " " << node.varName;
        if (node.resolvedSymbol) {
            out_ << " [" << node.resolvedSymbol->kindString() << "]";
        }
        if (node.initializer) {
            out_ << " =\n";
            indent();
            node.initializer->accept(*this);
            dedent();
        } else {
            out_ << "\n";
        }
    }

    // --- Expressions ---------------------------------------------------------

    void visit(BinaryExpr& node) override {
        printIndent();
        out_ << "BinaryExpr \"" << binaryOpToString(node.op) << "\""
             << typeAnnotation(&node) << "\n";
        indent();
        node.left->accept(*this);
        node.right->accept(*this);
        dedent();
    }

    void visit(UnaryExpr& node) override {
        printIndent();
        out_ << "UnaryExpr \"" << unaryOpToString(node.op) << "\""
             << typeAnnotation(&node) << "\n";
        indent();
        node.operand->accept(*this);
        dedent();
    }

    void visit(AssignmentExpr& node) override {
        printIndent();
        out_ << "AssignmentExpr \"" << node.target << " =\""
             << typeAnnotation(&node);
        if (node.resolvedSymbol) {
            out_ << " -> " << node.resolvedSymbol->kindString()
                 << " " << node.resolvedSymbol->name();
        }
        out_ << "\n";
        indent();
        node.value->accept(*this);
        dedent();
    }

    void visit(CallExpr& node) override {
        printIndent();
        out_ << "CallExpr \"" << node.callee << "\""
             << typeAnnotation(&node);
        if (node.resolvedFunction) {
            out_ << " -> function " << node.resolvedFunction->name();
        }
        out_ << "\n";
        indent();
        for (auto& arg : node.arguments) {
            arg->accept(*this);
        }
        dedent();
    }

    void visit(IdentifierExpr& node) override {
        printIndent();
        out_ << "Identifier \"" << node.name << "\""
             << typeAnnotation(&node);
        if (node.resolvedSymbol) {
            out_ << " -> " << node.resolvedSymbol->kindString()
                 << " " << node.resolvedSymbol->name();
        }
        out_ << "\n";
    }

    void visit(IntegerLiteralExpr& node) override {
        printIndent();
        out_ << "IntegerLiteral " << node.value
             << typeAnnotation(&node) << "\n";
    }

    void visit(BooleanLiteralExpr& node) override {
        printIndent();
        out_ << "BooleanLiteral " << (node.value ? "true" : "false")
             << typeAnnotation(&node) << "\n";
    }

private:
    std::string typeAnnotation(const Expr* expr) const {
        if (!expr || !expr->resolvedType) return "";
        return std::string(" : ") + expr->resolvedType->toString();
    }

    void printIndent() {
        for (int i = 0; i < depth_; ++i) out_ << "  ";
    }

    void indent() { ++depth_; }
    void dedent() { --depth_; }

    std::ostream& out_;
    int depth_ = 0;
};

} // namespace cc
