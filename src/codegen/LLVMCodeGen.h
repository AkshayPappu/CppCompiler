#pragma once

#include "ast/AST.h"
#include "semantic/Symbol.h"
#include "semantic/Type.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_map>

namespace cc {

class LLVMCodeGen {
public:
    LLVMCodeGen();

    bool generate(TranslationUnit& unit);
    void printIR(llvm::raw_ostream& out) const;

private:
    // Semantic type → LLVM type (single centralized mapping)
    llvm::Type* lowerType(const Type& type);

    // Two-pass function generation
    void declareFunction(FunctionDecl& node);
    bool generateFunction(FunctionDecl& node);

    // Statement emission
    void emitStmt(Stmt& stmt);
    void emitBlockStmt(BlockStmt& node);
    void emitIfStmt(IfStmt& node);
    void emitWhileStmt(WhileStmt& node);
    void emitReturnStmt(ReturnStmt& node);
    void emitExpressionStmt(ExpressionStmt& node);
    void emitVarDeclStmt(VarDeclStmt& node);

    // Expression emission (returns the LLVM value for the expression)
    llvm::Value* emitExpr(Expr& expr);
    llvm::Value* emitBinaryExpr(BinaryExpr& node);
    llvm::Value* emitUnaryExpr(UnaryExpr& node);
    llvm::Value* emitAssignmentExpr(AssignmentExpr& node);
    llvm::Value* emitCallExpr(CallExpr& node);
    llvm::Value* emitIdentifierExpr(IdentifierExpr& node);
    llvm::Value* emitIntegerLiteralExpr(IntegerLiteralExpr& node);
    llvm::Value* emitBooleanLiteralExpr(BooleanLiteralExpr& node);

    // Short-circuit logical operators (BasicBlocks + PHI)
    llvm::Value* emitLogicalAnd(BinaryExpr& node);
    llvm::Value* emitLogicalOr(BinaryExpr& node);

    // Helpers
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* fn,
                                              llvm::Type* type,
                                              const std::string& name);
    bool isTerminated() const;

    llvm::LLVMContext context_;
    std::unique_ptr<llvm::Module> module_;
    llvm::IRBuilder<> builder_;

    // Symbol identity → LLVM storage (NOT string-based)
    std::unordered_map<const Symbol*, llvm::AllocaInst*> locals_;
    std::unordered_map<const FunctionSymbol*, llvm::Function*> functions_;
};

} // namespace cc
