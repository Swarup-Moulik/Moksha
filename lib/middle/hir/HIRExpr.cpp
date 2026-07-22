#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRVisitor.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {
namespace hir {

static void printExprIndent(llvm::raw_ostream &os, int indent) {
  os << std::string(indent * 2, ' ');
}

static const char *opToString(BinaryOp op) {
  switch (op) {
  case BinaryOp::Add:
    return "+";
  case BinaryOp::Sub:
    return "-";
  case BinaryOp::Mul:
    return "*";
  case BinaryOp::Div:
    return "/";
  case BinaryOp::Mod:
    return "%";
  case BinaryOp::Pow:
    return "**";
  case BinaryOp::Assign:
    return "=";
  case BinaryOp::Equal:
    return "==";
  case BinaryOp::NotEqual:
    return "!=";
  case BinaryOp::Less:
    return "<";
  case BinaryOp::LessEqual:
    return "<=";
  case BinaryOp::Greater:
    return ">";
  case BinaryOp::GreaterEqual:
    return ">=";
  case BinaryOp::And:
    return "&&";
  case BinaryOp::Or:
    return "||";
  case BinaryOp::BitAnd:
    return "&";
  case BinaryOp::BitOr:
    return "|";
  case BinaryOp::BitXor:
    return "^";
  case BinaryOp::Shl:
    return "<<";
  case BinaryOp::Shr:
    return ">>";
  case BinaryOp::NullCoalesce:
    return "??";
  case BinaryOp::Range:
    return ":";
  default:
    return "?";
  }
}

static const char *unaryOpToString(hir::UnaryOp op) {
  switch (op) {
  case hir::UnaryOp::Neg:
    return "-";
  case hir::UnaryOp::Not:
    return "!";
  case hir::UnaryOp::BitNot:
    return "~";
  case hir::UnaryOp::PreInc:
    return "++ (pre)";
  case hir::UnaryOp::PreDec:
    return "-- (pre)";
  case hir::UnaryOp::PostInc:
    return "++ (post)";
  case hir::UnaryOp::PostDec:
    return "-- (post)";
  default:
    return "?";
  }
}

// Literals
void HIRIntegerLiteral::accept(HIRVisitor &v) { v.visitIntegerLiteral(*this); }
void HIRIntegerLiteral::accept(ConstHIRVisitor &v) const {
  v.visitIntegerLiteral(*this);
}

void HIRFloatLiteral::accept(HIRVisitor &v) { v.visitFloatLiteral(*this); }
void HIRFloatLiteral::accept(ConstHIRVisitor &v) const {
  v.visitFloatLiteral(*this);
}

void HIRDecimalLiteral::accept(HIRVisitor &v) { v.visitDecimalLiteral(*this); }
void HIRDecimalLiteral::accept(ConstHIRVisitor &v) const {
  v.visitDecimalLiteral(*this);
}

void HIRBoolLiteral::accept(HIRVisitor &v) { v.visitBoolLiteral(*this); }
void HIRBoolLiteral::accept(ConstHIRVisitor &v) const {
  v.visitBoolLiteral(*this);
}

void HIRStringLiteral::accept(HIRVisitor &v) { v.visitStringLiteral(*this); }
void HIRStringLiteral::accept(ConstHIRVisitor &v) const {
  v.visitStringLiteral(*this);
}

void HIRTemplateStringExpr::accept(HIRVisitor &v) {
  v.visitTemplateStringExpr(*this);
}
void HIRTemplateStringExpr::accept(ConstHIRVisitor &v) const {
  v.visitTemplateStringExpr(*this);
}

void HIRNullLiteral::accept(HIRVisitor &v) { v.visitNullLiteral(*this); }
void HIRNullLiteral::accept(ConstHIRVisitor &v) const {
  v.visitNullLiteral(*this);
}

void HIRArrayLiteral::accept(HIRVisitor &v) { v.visitArrayLiteral(*this); }
void HIRArrayLiteral::accept(ConstHIRVisitor &v) const {
  v.visitArrayLiteral(*this);
}

void HIRMapLiteral::accept(HIRVisitor &v) { v.visitMapLiteral(*this); }
void HIRMapLiteral::accept(ConstHIRVisitor &v) const {
  v.visitMapLiteral(*this);
}

// Operations
void HIRBinaryExpr::accept(HIRVisitor &v) { v.visitBinaryExpr(*this); }
void HIRBinaryExpr::accept(ConstHIRVisitor &v) const {
  v.visitBinaryExpr(*this);
}

void HIRUnaryExpr::accept(HIRVisitor &v) { v.visitUnaryExpr(*this); }
void HIRUnaryExpr::accept(ConstHIRVisitor &v) const { v.visitUnaryExpr(*this); }

void HIRCastExpr::accept(HIRVisitor &v) { v.visitCastExpr(*this); }
void HIRCastExpr::accept(ConstHIRVisitor &v) const { v.visitCastExpr(*this); }

void HIRTernaryExpr::accept(HIRVisitor &v) { v.visitTernaryExpr(*this); }
void HIRTernaryExpr::accept(ConstHIRVisitor &v) const {
  v.visitTernaryExpr(*this);
}

// Variables & Access
void HIRIdentifierExpr::accept(HIRVisitor &v) { v.visitIdentifierExpr(*this); }
void HIRIdentifierExpr::accept(ConstHIRVisitor &v) const {
  v.visitIdentifierExpr(*this);
}

void HIRMemberExpr::accept(HIRVisitor &v) { v.visitMemberExpr(*this); }
void HIRMemberExpr::accept(ConstHIRVisitor &v) const {
  v.visitMemberExpr(*this);
}

void HIRIndexExpr::accept(HIRVisitor &v) { v.visitIndexExpr(*this); }
void HIRIndexExpr::accept(ConstHIRVisitor &v) const { v.visitIndexExpr(*this); }

void HIRThisExpr::accept(HIRVisitor &v) { v.visitThisExpr(*this); }
void HIRThisExpr::accept(ConstHIRVisitor &v) const { v.visitThisExpr(*this); }

// High-Level Constructs
void HIRCallExpr::accept(HIRVisitor &v) { v.visitCallExpr(*this); }
void HIRCallExpr::accept(ConstHIRVisitor &v) const { v.visitCallExpr(*this); }

void HIRNewExpr::accept(HIRVisitor &v) { v.visitNewExpr(*this); }
void HIRNewExpr::accept(ConstHIRVisitor &v) const { v.visitNewExpr(*this); }

void HIRThreadExpr::accept(HIRVisitor &v) { v.visitThreadExpr(*this); }
void HIRThreadExpr::accept(ConstHIRVisitor &v) const {
  v.visitThreadExpr(*this);
}

// Lambda Implementation
HIRLambdaExpr::HIRLambdaExpr(std::vector<HIRLambdaParam> params,
                             std::vector<HIRCapture> captures,
                             std::unique_ptr<HIRStmt> body, const HIRType *type,
                             CaptureMode mode, bool isAsync, SourceLocation loc)
    : HIRExpr(Kind::Lambda, type, ValueCategory::RValue, loc),
      params(std::move(params)), captures(std::move(captures)),
      body(std::move(body)), captureMode(mode), isAsync(isAsync) {}

HIRLambdaExpr::~HIRLambdaExpr() = default;

void HIRLambdaExpr::accept(HIRVisitor &v) { v.visitLambdaExpr(*this); }
void HIRLambdaExpr::accept(ConstHIRVisitor &v) const {
  v.visitLambdaExpr(*this);
}

void HIRSizeOfExpr::accept(HIRVisitor &v) { v.visitSizeOfExpr(*this); }
void HIRSizeOfExpr::accept(ConstHIRVisitor &v) const {
  v.visitSizeOfExpr(*this);
}

void HIRAwaitExpr::accept(HIRVisitor &v) { v.visitAwaitExpr(*this); }
void HIRAwaitExpr::accept(ConstHIRVisitor &v) const { v.visitAwaitExpr(*this); }

void HIRSuperExpr::accept(HIRVisitor &v) { v.visitSuperExpr(*this); }
void HIRSuperExpr::accept(ConstHIRVisitor &v) const { v.visitSuperExpr(*this); }

void HIRDerefExpr::accept(HIRVisitor &v) { v.visitDerefExpr(*this); }
void HIRDerefExpr::accept(ConstHIRVisitor &v) const { v.visitDerefExpr(*this); }

void HIRAddressOfExpr::accept(HIRVisitor &v) { v.visitAddressOfExpr(*this); }
void HIRAddressOfExpr::accept(ConstHIRVisitor &v) const {
  v.visitAddressOfExpr(*this);
}

void HIRSpreadExpr::accept(HIRVisitor &v) { v.visitSpreadExpr(*this); }
void HIRSpreadExpr::accept(ConstHIRVisitor &v) const {
  v.visitSpreadExpr(*this);
}

void HIRInputExpr::accept(HIRVisitor &v) { v.visitInputExpr(*this); }
void HIRInputExpr::accept(ConstHIRVisitor &v) const { v.visitInputExpr(*this); }

const HIRStmt *HIRLambdaExpr::getBody() const { return body.get(); }

void HIRIntegerLiteral::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "IntLiteral (" << value << ")\n";
}
void HIRFloatLiteral::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "FloatLiteral (" << value << ")\n";
}
void HIRDecimalLiteral::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "DecimalLiteral: " << value << " (" << getType()->toString() << ")\n";
}
void HIRBoolLiteral::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "BoolLiteral (" << (value ? "true" : "false") << ")\n";
}
void HIRStringLiteral::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "StringLiteral (\"" << value << "\")\n";
}
void HIRNullLiteral::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "NullLiteral\n";
}
void HIRIdentifierExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "Identifier (" << name << ")\n";
}
void HIRBinaryExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "BinaryExpr (Op: " << opToString(op) << ")\n";
  if (lhs)
    lhs->dump(os, indent + 1);
  if (rhs)
    rhs->dump(os, indent + 1);
}
void HIRUnaryExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "UnaryExpr (Op: " << unaryOpToString(op) << ")\n";
  if (operand)
    operand->dump(os, indent + 1);
}
void HIRCastExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "CastExpr (Op: " << static_cast<int>(op) << ")\n";
  if (expr)
    expr->dump(os, indent + 1);
}
void HIRCallExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "CallExpr\n";
  if (callee)
    callee->dump(os, indent + 1);
  for (const auto &arg : args) {
    if (arg)
      arg->dump(os, indent + 1);
  }
}
void HIRDerefExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "DerefExpr\n";
  if (pointer)
    pointer->dump(os, indent + 1);
}
void HIRAddressOfExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "AddressOfExpr\n";
  if (operand)
    operand->dump(os, indent + 1);
}

void HIRInputExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "InputExpr\n";
  if (prompt) {
    prompt->dump(os, indent + 1);
  }
}

// Complex Expression Dumps
void HIRArrayLiteral::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "ArrayLiteral\n";
  for (const auto &el : getElements()) {
    if (el)
      el->dump(os, indent + 1);
  }
}

void HIRMapLiteral::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "MapLiteral\n";
  for (const auto &pair : entries) {
    if (pair.first)
      pair.first->dump(os, indent + 1);
    if (pair.second)
      pair.second->dump(os, indent + 1);
  }
}

void HIRTemplateStringExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "TemplateStringExpr\n";
  for (const auto &part : parts) {
    if (part)
      part->dump(os, indent + 1);
  }
}

void HIRMemberExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "MemberExpr (Member: " << getMemberName();
  if (info.isBitfield) {
    os << " [bitfield: width=" << info.bitWidth << ", offset=" << info.bitOffset
       << "]";
  }
  os << ")\n";

  if (getObject()) {
    getObject()->dump(os, indent + 1);
  }
}

void HIRIndexExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "IndexExpr\n";
  if (getBase())
    getBase()->dump(os, indent + 1);
  if (getIndex())
    getIndex()->dump(os, indent + 1);
}

void HIRTernaryExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "TernaryExpr\n";
  if (getCond())
    getCond()->dump(os, indent + 1);
  if (getTrueExpr())
    getTrueExpr()->dump(os, indent + 1);
  if (getFalseExpr())
    getFalseExpr()->dump(os, indent + 1);
}

void HIRThisExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "ThisExpr\n";
}

void HIRSuperExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "SuperExpr\n";
}

void HIRNewExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "NewExpr\n";
  for (const auto &arg : getArgs()) {
    if (arg)
      arg->dump(os, indent + 1);
  }
}

void HIRLambdaExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "LambdaExpr ";
  if (isAsync)
    os << "[async] ";
  os << "(Params: ";
  const auto &paramsList = getParams();
  for (size_t i = 0; i < paramsList.size(); ++i) {
    os << paramsList[i].name;
    if (paramsList[i].defaultValue) {
      os << " = <default_val>";
    }
    if (i < paramsList.size() - 1)
      os << ", ";
  }
  os << ")\n";

  if (!captures.empty()) {
    printExprIndent(os, indent + 1);
    os << "[Captures: ";
    for (size_t i = 0; i < captures.size(); ++i) {
      os << captures[i].name
         << (captures[i].kind == CaptureKind::ByReference ? "(&)" : "(v)");
      if (i < captures.size() - 1)
        os << ", ";
    }
    os << "]\n";
  }

  if (getBody()) {
    getBody()->dump(os, indent + 1);
  }
}

void HIRThreadExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "ThreadExpr\n";
  if (getTask())
    getTask()->dump(os, indent + 1);
}

void HIRSizeOfExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "SizeOfExpr (Target: " << targetType->toString() << ")\n";
}

void HIRAwaitExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "AwaitExpr\n";
  if (getExpr()) {
    getExpr()->dump(os, indent + 1);
  }
}

void HIRSpreadExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "SpreadExpr (...)\n";
  if (iterable)
    iterable->dump(os, indent + 1);
}

void HIRSharedExpr::accept(HIRVisitor &v) { v.visitSharedExpr(*this); }

void HIRSharedExpr::accept(ConstHIRVisitor &v) const {
  v.visitSharedExpr(*this);
}

void HIRSharedExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "SharedExpr\n";
  if (getExpr()) {
    getExpr()->dump(os, indent + 1);
  }
}

void HIRAsmExpr::accept(HIRVisitor &v) { v.visitAsmExpr(*this); }
void HIRAsmExpr::accept(ConstHIRVisitor &v) const { v.visitAsmExpr(*this); }

void HIRAsmExpr::dump(llvm::raw_ostream &os, int indent) const {
  printExprIndent(os, indent);
  os << "AsmExpr: \"" << assemblyStr << "\"\n";

  for (const auto &op : outputs) {
    printExprIndent(os, indent + 1);
    os << "out(\"" << op.constraint << "\")\n";
    if (op.expr)
      op.expr->dump(os, indent + 2);
  }

  for (const auto &op : inputs) {
    printExprIndent(os, indent + 1);
    os << "in(\"" << op.constraint << "\")\n";
    if (op.expr)
      op.expr->dump(os, indent + 2);
  }

  for (const auto &op : inouts) {
    printExprIndent(os, indent + 1);
    os << "inout(\"" << op.constraint << "\")\n";
    if (op.expr)
      op.expr->dump(os, indent + 2);
  }

  if (!clobbers.empty()) {
    printExprIndent(os, indent + 1);
    os << "clobber(";
    for (size_t i = 0; i < clobbers.size(); ++i) {
      if (i > 0)
        os << ", ";
      os << "\"" << clobbers[i] << "\"";
    }
    os << ")\n";
  }

  if (isVolatile) {
    printExprIndent(os, indent + 1);
    os << "[volatile]\n";
  }
}

} // namespace hir
} // namespace moksha
