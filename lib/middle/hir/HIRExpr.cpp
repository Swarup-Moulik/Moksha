#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRStmt.h"    // Required: Defines HIRStmt for Lambda body access
#include "moksha/HIR/HIRVisitor.h" // Required: Defines the Visitor interface

namespace moksha {
namespace hir {

// ============================================================================
// [Literals]
// ============================================================================

void HIRIntegerLiteral::accept(HIRVisitor &v) { v.visitIntegerLiteral(*this); }
void HIRIntegerLiteral::accept(ConstHIRVisitor &v) const { v.visitIntegerLiteral(*this); }

void HIRFloatLiteral::accept(HIRVisitor &v)   { v.visitFloatLiteral(*this); }
void HIRFloatLiteral::accept(ConstHIRVisitor &v) const { v.visitFloatLiteral(*this); }

void HIRBoolLiteral::accept(HIRVisitor &v)    { v.visitBoolLiteral(*this); }
void HIRBoolLiteral::accept(ConstHIRVisitor &v) const { v.visitBoolLiteral(*this); }

void HIRStringLiteral::accept(HIRVisitor &v)  { v.visitStringLiteral(*this); }
void HIRStringLiteral::accept(ConstHIRVisitor &v) const { v.visitStringLiteral(*this); }

void HIRNullLiteral::accept(HIRVisitor &v)    { v.visitNullLiteral(*this); }
void HIRNullLiteral::accept(ConstHIRVisitor &v) const { v.visitNullLiteral(*this); }

void HIRArrayLiteral::accept(HIRVisitor &v)   { v.visitArrayLiteral(*this); }
void HIRArrayLiteral::accept(ConstHIRVisitor &v) const { v.visitArrayLiteral(*this); }

// ============================================================================
// [Operations]
// ============================================================================

void HIRBinaryExpr::accept(HIRVisitor &v)  { v.visitBinaryExpr(*this); }
void HIRBinaryExpr::accept(ConstHIRVisitor &v) const { v.visitBinaryExpr(*this); }

void HIRUnaryExpr::accept(HIRVisitor &v)   { v.visitUnaryExpr(*this); }
void HIRUnaryExpr::accept(ConstHIRVisitor &v) const { v.visitUnaryExpr(*this); }

void HIRCastExpr::accept(HIRVisitor &v)    { v.visitCastExpr(*this); }
void HIRCastExpr::accept(ConstHIRVisitor &v) const { v.visitCastExpr(*this); }

void HIRTernaryExpr::accept(HIRVisitor &v) { v.visitTernaryExpr(*this); }
void HIRTernaryExpr::accept(ConstHIRVisitor &v) const { v.visitTernaryExpr(*this); }

// ============================================================================
// [Variables & Access]
// ============================================================================

void HIRIdentifierExpr::accept(HIRVisitor &v) { v.visitIdentifierExpr(*this); }
void HIRIdentifierExpr::accept(ConstHIRVisitor &v) const { v.visitIdentifierExpr(*this); }

void HIRMemberExpr::accept(HIRVisitor &v)     { v.visitMemberExpr(*this); }
void HIRMemberExpr::accept(ConstHIRVisitor &v) const { v.visitMemberExpr(*this); }

void HIRIndexExpr::accept(HIRVisitor &v)      { v.visitIndexExpr(*this); }
void HIRIndexExpr::accept(ConstHIRVisitor &v) const { v.visitIndexExpr(*this); }

void HIRThisExpr::accept(HIRVisitor &v)       { v.visitThisExpr(*this); }
void HIRThisExpr::accept(ConstHIRVisitor &v) const { v.visitThisExpr(*this); }

// ============================================================================
// [High-Level Constructs]
// ============================================================================

void HIRCallExpr::accept(HIRVisitor &v)   { v.visitCallExpr(*this); }
void HIRCallExpr::accept(ConstHIRVisitor &v) const { v.visitCallExpr(*this); }

void HIRNewExpr::accept(HIRVisitor &v)    { v.visitNewExpr(*this); }
void HIRNewExpr::accept(ConstHIRVisitor &v) const { v.visitNewExpr(*this); }

void HIRThreadExpr::accept(HIRVisitor &v) { v.visitThreadExpr(*this); }
void HIRThreadExpr::accept(ConstHIRVisitor &v) const { v.visitThreadExpr(*this); }

// ============================================================================
// [Lambda Implementation]
// ============================================================================

void HIRLambdaExpr::accept(HIRVisitor &v) { v.visitLambdaExpr(*this); }
void HIRLambdaExpr::accept(ConstHIRVisitor &v) const { v.visitLambdaExpr(*this); }

// Defined here because HIRStmt is fully defined in this file (via include),
// whereas it is only forward-declared in HIRExpr.h
const HIRStmt *HIRLambdaExpr::getBody() const {
    return body.get();
}

} // namespace hir
} // namespace moksha
