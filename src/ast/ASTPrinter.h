#pragma once

#include "ast/AST.h"
#include <iostream>
#include <string>

namespace cc {

class ASTPrinter : public ASTVisitor {
public:
    explicit ASTPrinter(std::ostream& out = std::cout) : out_(out) {}

    void print(ASTNode& node) {
        node.accept(*this);
    }

    void visit(TranslationUnit& node) override {
        printIndent();
        out_ << "TranslationUnit\n";
        indent();
        for (auto& fn : node.functions) {
            fn->accept(*this);
        }
        dedent();
    }

    void visit(FunctionDecl& node) override {
        printIndent();
        out_ << "FunctionDecl: " << node.returnType << " " << node.name << "(";
        for (size_t i = 0; i < node.parameters.size(); ++i) {
            if (i > 0) out_ << ", ";
            out_ << node.parameters[i].typeName << " "
                 << node.parameters[i].name;
        }
        out_ << ")\n";
        indent();
        node.body->accept(*this);
        dedent();
    }

    // Statements
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
        out_ << "Condition:\n";
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
        out_ << "Condition:\n";
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
        if (node.initializer) {
            out_ << " =\n";
            indent();
            node.initializer->accept(*this);
            dedent();
        } else {
            out_ << "\n";
        }
    }

    // Expressions
    void visit(BinaryExpr& node) override {
        printIndent();
        out_ << "BinaryExpr: " << binaryOpToString(node.op) << "\n";
        indent();
        node.left->accept(*this);
        node.right->accept(*this);
        dedent();
    }

    void visit(UnaryExpr& node) override {
        printIndent();
        out_ << "UnaryExpr: " << unaryOpToString(node.op) << "\n";
        indent();
        node.operand->accept(*this);
        dedent();
    }

    void visit(AssignmentExpr& node) override {
        printIndent();
        out_ << "AssignmentExpr: " << node.target << " =\n";
        indent();
        node.value->accept(*this);
        dedent();
    }

    void visit(CallExpr& node) override {
        printIndent();
        out_ << "CallExpr: " << node.callee << "\n";
        indent();
        for (auto& arg : node.arguments) {
            arg->accept(*this);
        }
        dedent();
    }

    void visit(IdentifierExpr& node) override {
        printIndent();
        out_ << "IdentifierExpr: " << node.name << "\n";
    }

    void visit(IntegerLiteralExpr& node) override {
        printIndent();
        out_ << "IntegerLiteralExpr: " << node.value << "\n";
    }

    void visit(BooleanLiteralExpr& node) override {
        printIndent();
        out_ << "BooleanLiteralExpr: "
             << (node.value ? "true" : "false") << "\n";
    }

private:
    void printIndent() {
        for (int i = 0; i < depth_; ++i) {
            out_ << "  ";
        }
    }

    void indent() { ++depth_; }
    void dedent() { --depth_; }

    std::ostream& out_;
    int depth_ = 0;
};

} // namespace cc
