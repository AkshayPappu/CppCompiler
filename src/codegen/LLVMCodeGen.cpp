#include "codegen/LLVMCodeGen.h"

#include <llvm/IR/CFG.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/raw_ostream.h>

#include <iostream>

namespace cc {

LLVMCodeGen::LLVMCodeGen() : builder_(context_) {}

// ============================================================================
// Entry point
// ============================================================================

bool LLVMCodeGen::generate(TranslationUnit& unit) {
    module_ = std::make_unique<llvm::Module>("module", context_);

    // Pass 1: declare all functions so forward calls resolve.
    for (auto& fn : unit.functions) {
        declareFunction(*fn);
    }

    // Pass 2: generate each function body.
    bool ok = true;
    for (auto& fn : unit.functions) {
        if (!generateFunction(*fn)) ok = false;
    }
    if (!ok) return false;

    // Verify the complete module.
    std::string errStr;
    llvm::raw_string_ostream errStream(errStr);
    if (llvm::verifyModule(*module_, &errStream)) {
        std::cerr << "internal error: LLVM IR verification failed:\n"
                  << errStr << "\n";
        return false;
    }

    return true;
}

void LLVMCodeGen::printIR(llvm::raw_ostream& out) const {
    module_->print(out, nullptr);
}

// ============================================================================
// Type mapping
// ============================================================================

llvm::Type* LLVMCodeGen::lowerType(const Type& type) {
    switch (type.kind()) {
        case TypeKind::Int:  return llvm::Type::getInt32Ty(context_);
        case TypeKind::Bool: return llvm::Type::getInt1Ty(context_);
        case TypeKind::Void: return llvm::Type::getVoidTy(context_);
        default:             return llvm::Type::getInt32Ty(context_);
    }
}

// ============================================================================
// Helpers
// ============================================================================

llvm::AllocaInst* LLVMCodeGen::createEntryBlockAlloca(
    llvm::Function* fn, llvm::Type* type, const std::string& name) {
    llvm::IRBuilder<> tmp(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmp.CreateAlloca(type, nullptr, name);
}

bool LLVMCodeGen::isTerminated() const {
    llvm::BasicBlock* bb = builder_.GetInsertBlock();
    // No insert point means every predecessor already returned.
    return !bb || bb->getTerminator() != nullptr;
}

// ============================================================================
// Function generation
// ============================================================================

void LLVMCodeGen::declareFunction(FunctionDecl& node) {
    const FunctionSymbol* sym = node.resolvedSymbol;

    llvm::Type* retType = lowerType(*sym->returnType());

    std::vector<llvm::Type*> paramTypes;
    for (const Type* pt : sym->parameterTypes) {
        paramTypes.push_back(lowerType(*pt));
    }

    auto* fnType = llvm::FunctionType::get(retType, paramTypes, false);
    auto* fn = llvm::Function::Create(
        fnType, llvm::Function::ExternalLinkage, node.name, module_.get());

    size_t i = 0;
    for (auto& arg : fn->args()) {
        arg.setName(sym->parameterNames[i++]);
    }

    functions_[sym] = fn;
}

bool LLVMCodeGen::generateFunction(FunctionDecl& node) {
    const FunctionSymbol* sym = node.resolvedSymbol;
    llvm::Function* fn = functions_[sym];

    auto* entry = llvm::BasicBlock::Create(context_, "entry", fn);
    builder_.SetInsertPoint(entry);

    locals_.clear();

    // Create allocas for parameters and store the incoming argument values.
    size_t i = 0;
    for (auto& arg : fn->args()) {
        const Symbol* paramSym = node.parameters[i].resolvedSymbol;
        llvm::AllocaInst* alloca =
            createEntryBlockAlloca(fn, arg.getType(), arg.getName().str());
        builder_.CreateStore(&arg, alloca);
        locals_[paramSym] = alloca;
        ++i;
    }

    // Emit the function body directly (the outermost BlockStmt shares scope
    // with parameters, so we iterate its statements, not visit the block).
    for (auto& stmt : node.body->statements) {
        if (isTerminated()) break;
        emitStmt(*stmt);
    }

    // Ensure every reachable path ends with a terminator.
    if (builder_.GetInsertBlock() && !builder_.GetInsertBlock()->getTerminator()) {
        if (fn->getReturnType()->isVoidTy()) {
            builder_.CreateRetVoid();
        } else {
            builder_.CreateRet(
                llvm::Constant::getNullValue(fn->getReturnType()));
        }
    }

    if (llvm::verifyFunction(*fn, &llvm::errs())) {
        std::cerr << "internal error: verification failed for function '"
                  << node.name << "'\n";
        return false;
    }
    return true;
}

// ============================================================================
// Statement dispatch
// ============================================================================

void LLVMCodeGen::emitStmt(Stmt& stmt) {
    if (isTerminated()) return;

    if (auto* p = dynamic_cast<BlockStmt*>(&stmt))       return emitBlockStmt(*p);
    if (auto* p = dynamic_cast<IfStmt*>(&stmt))          return emitIfStmt(*p);
    if (auto* p = dynamic_cast<WhileStmt*>(&stmt))       return emitWhileStmt(*p);
    if (auto* p = dynamic_cast<ReturnStmt*>(&stmt))      return emitReturnStmt(*p);
    if (auto* p = dynamic_cast<ExpressionStmt*>(&stmt))  return emitExpressionStmt(*p);
    if (auto* p = dynamic_cast<VarDeclStmt*>(&stmt))     return emitVarDeclStmt(*p);
}

// ============================================================================
// Statements
// ============================================================================

void LLVMCodeGen::emitBlockStmt(BlockStmt& node) {
    for (auto& stmt : node.statements) {
        if (isTerminated()) break;
        emitStmt(*stmt);
    }
}

void LLVMCodeGen::emitIfStmt(IfStmt& node) {
    llvm::Function* fn = builder_.GetInsertBlock()->getParent();

    llvm::Value* cond = emitExpr(*node.condition);

    auto* thenBB  = llvm::BasicBlock::Create(context_, "if.then", fn);
    auto* elseBB  = node.elseBranch
                        ? llvm::BasicBlock::Create(context_, "if.else", fn)
                        : nullptr;
    auto* mergeBB = llvm::BasicBlock::Create(context_, "if.end", fn);

    builder_.CreateCondBr(cond, thenBB, elseBB ? elseBB : mergeBB);

    // Then branch
    builder_.SetInsertPoint(thenBB);
    emitStmt(*node.thenBranch);
    if (!isTerminated()) builder_.CreateBr(mergeBB);

    // Else branch
    if (node.elseBranch) {
        builder_.SetInsertPoint(elseBB);
        emitStmt(*node.elseBranch);
        if (!isTerminated()) builder_.CreateBr(mergeBB);
    }

    // If both branches returned, merge has no predecessors — drop it.
    if (llvm::pred_empty(mergeBB)) {
        mergeBB->eraseFromParent();
        builder_.ClearInsertionPoint();
    } else {
        builder_.SetInsertPoint(mergeBB);
    }
}

void LLVMCodeGen::emitWhileStmt(WhileStmt& node) {
    llvm::Function* fn = builder_.GetInsertBlock()->getParent();

    auto* condBB = llvm::BasicBlock::Create(context_, "while.cond", fn);
    auto* bodyBB = llvm::BasicBlock::Create(context_, "while.body", fn);
    auto* endBB  = llvm::BasicBlock::Create(context_, "while.end", fn);

    builder_.CreateBr(condBB);

    // Condition
    builder_.SetInsertPoint(condBB);
    llvm::Value* cond = emitExpr(*node.condition);
    builder_.CreateCondBr(cond, bodyBB, endBB);

    // Body
    builder_.SetInsertPoint(bodyBB);
    emitStmt(*node.body);
    if (!isTerminated()) builder_.CreateBr(condBB);

    builder_.SetInsertPoint(endBB);
}

void LLVMCodeGen::emitReturnStmt(ReturnStmt& node) {
    if (node.value) {
        llvm::Value* val = emitExpr(*node.value);
        builder_.CreateRet(val);
    } else {
        builder_.CreateRetVoid();
    }
}

void LLVMCodeGen::emitExpressionStmt(ExpressionStmt& node) {
    emitExpr(*node.expression);
}

void LLVMCodeGen::emitVarDeclStmt(VarDeclStmt& node) {
    llvm::Function* fn = builder_.GetInsertBlock()->getParent();
    const Symbol* sym = node.resolvedSymbol;

    llvm::Type* ty = lowerType(*sym->type());
    llvm::AllocaInst* alloca = createEntryBlockAlloca(fn, ty, node.varName);

    if (node.initializer) {
        llvm::Value* init = emitExpr(*node.initializer);
        builder_.CreateStore(init, alloca);
    }

    locals_[sym] = alloca;
}

// ============================================================================
// Expression dispatch
// ============================================================================

llvm::Value* LLVMCodeGen::emitExpr(Expr& expr) {
    if (auto* p = dynamic_cast<BinaryExpr*>(&expr))         return emitBinaryExpr(*p);
    if (auto* p = dynamic_cast<UnaryExpr*>(&expr))          return emitUnaryExpr(*p);
    if (auto* p = dynamic_cast<AssignmentExpr*>(&expr))     return emitAssignmentExpr(*p);
    if (auto* p = dynamic_cast<CallExpr*>(&expr))           return emitCallExpr(*p);
    if (auto* p = dynamic_cast<IdentifierExpr*>(&expr))     return emitIdentifierExpr(*p);
    if (auto* p = dynamic_cast<IntegerLiteralExpr*>(&expr)) return emitIntegerLiteralExpr(*p);
    if (auto* p = dynamic_cast<BooleanLiteralExpr*>(&expr)) return emitBooleanLiteralExpr(*p);
    return nullptr;
}

// ============================================================================
// Expressions
// ============================================================================

llvm::Value* LLVMCodeGen::emitIntegerLiteralExpr(IntegerLiteralExpr& node) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(context_),
                                  node.value, true);
}

llvm::Value* LLVMCodeGen::emitBooleanLiteralExpr(BooleanLiteralExpr& node) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(context_),
                                  node.value ? 1 : 0);
}

llvm::Value* LLVMCodeGen::emitIdentifierExpr(IdentifierExpr& node) {
    llvm::AllocaInst* alloca = locals_.at(node.resolvedSymbol);
    return builder_.CreateLoad(lowerType(*node.resolvedType), alloca,
                               node.name);
}

llvm::Value* LLVMCodeGen::emitBinaryExpr(BinaryExpr& node) {
    // Short-circuit operators use control flow, not eager evaluation.
    if (node.op == BinaryOp::LogicalAnd) return emitLogicalAnd(node);
    if (node.op == BinaryOp::LogicalOr)  return emitLogicalOr(node);

    llvm::Value* lhs = emitExpr(*node.left);
    llvm::Value* rhs = emitExpr(*node.right);

    switch (node.op) {
        case BinaryOp::Add: return builder_.CreateAdd(lhs, rhs, "add");
        case BinaryOp::Sub: return builder_.CreateSub(lhs, rhs, "sub");
        case BinaryOp::Mul: return builder_.CreateMul(lhs, rhs, "mul");
        case BinaryOp::Div: return builder_.CreateSDiv(lhs, rhs, "div");
        case BinaryOp::Mod: return builder_.CreateSRem(lhs, rhs, "mod");

        case BinaryOp::Equal:        return builder_.CreateICmpEQ(lhs, rhs, "eq");
        case BinaryOp::NotEqual:     return builder_.CreateICmpNE(lhs, rhs, "ne");
        case BinaryOp::Less:         return builder_.CreateICmpSLT(lhs, rhs, "lt");
        case BinaryOp::LessEqual:    return builder_.CreateICmpSLE(lhs, rhs, "le");
        case BinaryOp::Greater:      return builder_.CreateICmpSGT(lhs, rhs, "gt");
        case BinaryOp::GreaterEqual: return builder_.CreateICmpSGE(lhs, rhs, "ge");

        default: return nullptr; // LogicalAnd/Or handled above
    }
}

llvm::Value* LLVMCodeGen::emitUnaryExpr(UnaryExpr& node) {
    llvm::Value* operand = emitExpr(*node.operand);

    switch (node.op) {
        case UnaryOp::Negate:
            return builder_.CreateNeg(operand, "neg");
        case UnaryOp::LogicalNot:
            return builder_.CreateNot(operand, "not");
    }
    return nullptr;
}

llvm::Value* LLVMCodeGen::emitAssignmentExpr(AssignmentExpr& node) {
    llvm::Value* val = emitExpr(*node.value);
    llvm::AllocaInst* alloca = locals_.at(node.resolvedSymbol);
    builder_.CreateStore(val, alloca);
    return val;
}

llvm::Value* LLVMCodeGen::emitCallExpr(CallExpr& node) {
    llvm::Function* callee = functions_.at(node.resolvedFunction);

    std::vector<llvm::Value*> args;
    for (auto& arg : node.arguments) {
        args.push_back(emitExpr(*arg));
    }

    if (callee->getReturnType()->isVoidTy()) {
        builder_.CreateCall(callee, args);
        return nullptr;
    }

    return builder_.CreateCall(callee, args, "call");
}

// ============================================================================
// Short-circuit logical operators
// ============================================================================

llvm::Value* LLVMCodeGen::emitLogicalAnd(BinaryExpr& node) {
    llvm::Function* fn = builder_.GetInsertBlock()->getParent();

    auto* rhsBB   = llvm::BasicBlock::Create(context_, "and.rhs", fn);
    auto* mergeBB = llvm::BasicBlock::Create(context_, "and.merge", fn);

    // Evaluate LHS; if false, short-circuit to merge with result = false.
    llvm::Value* lhs = emitExpr(*node.left);
    builder_.CreateCondBr(lhs, rhsBB, mergeBB);
    llvm::BasicBlock* lhsEndBB = builder_.GetInsertBlock();

    // Evaluate RHS (only reached when LHS is true).
    builder_.SetInsertPoint(rhsBB);
    llvm::Value* rhs = emitExpr(*node.right);
    builder_.CreateBr(mergeBB);
    llvm::BasicBlock* rhsEndBB = builder_.GetInsertBlock();

    // Merge: result is false (from LHS path) or rhs value (from RHS path).
    builder_.SetInsertPoint(mergeBB);
    auto* phi = builder_.CreatePHI(llvm::Type::getInt1Ty(context_), 2,
                                    "and.result");
    phi->addIncoming(llvm::ConstantInt::getFalse(context_), lhsEndBB);
    phi->addIncoming(rhs, rhsEndBB);

    return phi;
}

llvm::Value* LLVMCodeGen::emitLogicalOr(BinaryExpr& node) {
    llvm::Function* fn = builder_.GetInsertBlock()->getParent();

    auto* rhsBB   = llvm::BasicBlock::Create(context_, "or.rhs", fn);
    auto* mergeBB = llvm::BasicBlock::Create(context_, "or.merge", fn);

    // Evaluate LHS; if true, short-circuit to merge with result = true.
    llvm::Value* lhs = emitExpr(*node.left);
    builder_.CreateCondBr(lhs, mergeBB, rhsBB);
    llvm::BasicBlock* lhsEndBB = builder_.GetInsertBlock();

    // Evaluate RHS (only reached when LHS is false).
    builder_.SetInsertPoint(rhsBB);
    llvm::Value* rhs = emitExpr(*node.right);
    builder_.CreateBr(mergeBB);
    llvm::BasicBlock* rhsEndBB = builder_.GetInsertBlock();

    // Merge: result is true (from LHS path) or rhs value (from RHS path).
    builder_.SetInsertPoint(mergeBB);
    auto* phi = builder_.CreatePHI(llvm::Type::getInt1Ty(context_), 2,
                                    "or.result");
    phi->addIncoming(llvm::ConstantInt::getTrue(context_), lhsEndBB);
    phi->addIncoming(rhs, rhsEndBB);

    return phi;
}

} // namespace cc
