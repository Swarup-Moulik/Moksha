#pragma once

namespace moksha {
namespace hir {

// --- Forward Declarations (Statements) ---
class BlockStmt;
class UnsafeBlockStmt;
class LockStmt;
class ExprStmt;
class ReturnStmt;
class IfStmt;
class WhileStmt;
class DoWhileStmt;
class ForStmt;
class ForInStmt;
class SwitchStmt;
class DeferStmt;
class TryCatchStmt;
class BreakStmt;
class ContinueStmt;
class HIRVarDeclStmt;
class HIRFunction;

// --- Forward Declarations (Expressions) ---
class HIRIntegerLiteral;
class HIRFloatLiteral;
class HIRBoolLiteral;
class HIRStringLiteral;
class HIRNullLiteral;
class HIRArrayLiteral;

class HIRBinaryExpr;
class HIRUnaryExpr;
class HIRCastExpr;
class HIRTernaryExpr;

class HIRIdentifierExpr;
class HIRMemberExpr;
class HIRIndexExpr;
class HIRThisExpr;

class HIRCallExpr;
class HIRNewExpr;
class HIRLambdaExpr;
class HIRThreadExpr;

// ============================================================================
// [Expression Visitor Interface]
// ============================================================================

class HIRExprVisitor {
public:
    virtual ~HIRExprVisitor() = default;

    // Literals
    virtual void visitIntegerLiteral(HIRIntegerLiteral &expr) = 0;
    virtual void visitFloatLiteral(HIRFloatLiteral &expr) = 0;
    virtual void visitBoolLiteral(HIRBoolLiteral &expr) = 0;
    virtual void visitStringLiteral(HIRStringLiteral &expr) = 0;
    virtual void visitNullLiteral(HIRNullLiteral &expr) = 0;
    virtual void visitArrayLiteral(HIRArrayLiteral &expr) = 0;

    // Operations
    virtual void visitBinaryExpr(HIRBinaryExpr &expr) = 0;
    virtual void visitUnaryExpr(HIRUnaryExpr &expr) = 0;
    virtual void visitCastExpr(HIRCastExpr &expr) = 0;
    virtual void visitTernaryExpr(HIRTernaryExpr &expr) = 0;

    // Access
    virtual void visitIdentifierExpr(HIRIdentifierExpr &expr) = 0;
    virtual void visitMemberExpr(HIRMemberExpr &expr) = 0;
    virtual void visitIndexExpr(HIRIndexExpr &expr) = 0;
    virtual void visitThisExpr(HIRThisExpr &expr) = 0;

    // High-Level
    virtual void visitCallExpr(HIRCallExpr &expr) = 0;
    virtual void visitNewExpr(HIRNewExpr &expr) = 0;
    virtual void visitLambdaExpr(HIRLambdaExpr &expr) = 0;
    virtual void visitThreadExpr(HIRThreadExpr &expr) = 0;
};

// ============================================================================
// [Statement Visitor Interface]
// ============================================================================

class HIRStmtVisitor {
public:
    virtual ~HIRStmtVisitor() = default;

    virtual void visitBlockStmt(BlockStmt &stmt) = 0;
    virtual void visitUnsafeBlockStmt(UnsafeBlockStmt &stmt) = 0;
    virtual void visitLockStmt(LockStmt &stmt) = 0;
    virtual void visitExprStmt(ExprStmt &stmt) = 0;

    // Control Flow
    virtual void visitReturnStmt(ReturnStmt &stmt) = 0;
    virtual void visitIfStmt(IfStmt &stmt) = 0;
    virtual void visitWhileStmt(WhileStmt &stmt) = 0;
    virtual void visitDoWhileStmt(DoWhileStmt &stmt) = 0;
    virtual void visitForStmt(ForStmt &stmt) = 0;
    virtual void visitForInStmt(ForInStmt &stmt) = 0;
    virtual void visitSwitchStmt(SwitchStmt &stmt) = 0;

    // Jumps & Misc
    virtual void visitBreakStmt(BreakStmt &stmt) = 0;
    virtual void visitContinueStmt(ContinueStmt &stmt) = 0;
    virtual void visitDeferStmt(DeferStmt &stmt) = 0;
    virtual void visitTryCatchStmt(TryCatchStmt &stmt) = 0;

    // Declarations
    virtual void visitVarDeclStmt(HIRVarDeclStmt &stmt) = 0;
};

// ============================================================================
// [Unified Mutable Visitor]
// ============================================================================

class HIRVisitor : public HIRExprVisitor, public HIRStmtVisitor {
public:
    virtual ~HIRVisitor() = default;
    virtual void visitFunction(HIRFunction &func) = 0;
};

// ============================================================================
// [Const Visitor Interfaces]
// Allows read-only analysis passes (e.g., Liveness Analysis, Cost Modeling)
// ============================================================================

class ConstHIRExprVisitor {
public:
    virtual ~ConstHIRExprVisitor() = default;

    virtual void visitIntegerLiteral(const HIRIntegerLiteral &expr) = 0;
    virtual void visitFloatLiteral(const HIRFloatLiteral &expr) = 0;
    virtual void visitBoolLiteral(const HIRBoolLiteral &expr) = 0;
    virtual void visitStringLiteral(const HIRStringLiteral &expr) = 0;
    virtual void visitNullLiteral(const HIRNullLiteral &expr) = 0;
    virtual void visitArrayLiteral(const HIRArrayLiteral &expr) = 0;

    virtual void visitBinaryExpr(const HIRBinaryExpr &expr) = 0;
    virtual void visitUnaryExpr(const HIRUnaryExpr &expr) = 0;
    virtual void visitCastExpr(const HIRCastExpr &expr) = 0;
    virtual void visitTernaryExpr(const HIRTernaryExpr &expr) = 0;

    virtual void visitIdentifierExpr(const HIRIdentifierExpr &expr) = 0;
    virtual void visitMemberExpr(const HIRMemberExpr &expr) = 0;
    virtual void visitIndexExpr(const HIRIndexExpr &expr) = 0;
    virtual void visitThisExpr(const HIRThisExpr &expr) = 0;

    virtual void visitCallExpr(const HIRCallExpr &expr) = 0;
    virtual void visitNewExpr(const HIRNewExpr &expr) = 0;
    virtual void visitLambdaExpr(const HIRLambdaExpr &expr) = 0;
    virtual void visitThreadExpr(const HIRThreadExpr &expr) = 0;
};

class ConstHIRStmtVisitor {
public:
    virtual ~ConstHIRStmtVisitor() = default;

    virtual void visitBlockStmt(const BlockStmt &stmt) = 0;
    virtual void visitUnsafeBlockStmt(const UnsafeBlockStmt &stmt) = 0;
    virtual void visitLockStmt(const LockStmt &stmt) = 0;
    virtual void visitExprStmt(const ExprStmt &stmt) = 0;

    virtual void visitReturnStmt(const ReturnStmt &stmt) = 0;
    virtual void visitIfStmt(const IfStmt &stmt) = 0;
    virtual void visitWhileStmt(const WhileStmt &stmt) = 0;
    virtual void visitDoWhileStmt(const DoWhileStmt &stmt) = 0;
    virtual void visitForStmt(const ForStmt &stmt) = 0;
    virtual void visitForInStmt(const ForInStmt &stmt) = 0;
    virtual void visitSwitchStmt(const SwitchStmt &stmt) = 0;

    virtual void visitBreakStmt(const BreakStmt &stmt) = 0;
    virtual void visitContinueStmt(const ContinueStmt &stmt) = 0;
    virtual void visitDeferStmt(const DeferStmt &stmt) = 0;
    virtual void visitTryCatchStmt(const TryCatchStmt &stmt) = 0;

    virtual void visitVarDeclStmt(const HIRVarDeclStmt &stmt) = 0;
};

class ConstHIRVisitor : public ConstHIRExprVisitor, public ConstHIRStmtVisitor {
public:
    virtual ~ConstHIRVisitor() = default;
    virtual void visitFunction(const HIRFunction &func) = 0;
};

} // namespace hir
} // namespace moksha
