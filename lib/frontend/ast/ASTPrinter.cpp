#include "moksha/AST/ASTPrinter.h"
#include "moksha/AST/ASTVisitor.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Lexer/Lexer.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

/** @brief Pretty-prints an AST node to the output stream. */
ASTPrinter::ASTPrinter(llvm::raw_ostream &OS) : OS(OS) {}

void ASTPrinter::printIndent() {
  for (int i = 0; i < indentLevel; ++i)
    OS << "  ";
}

std::string ASTPrinter::tokenToString(TokenKind kind) {
  switch (kind) {
  case TokenKind::Plus:
    return "+";
  case TokenKind::Minus:
    return "-";
  case TokenKind::Star:
    return "*";
  case TokenKind::Slash:
    return "/";
  case TokenKind::Percent:
    return "%";
  case TokenKind::Power:
    return "**";
  case TokenKind::PlusPlus:
    return "++";
  case TokenKind::MinusMinus:
    return "--";
  case TokenKind::Equal:
    return "=";
  case TokenKind::PlusEqual:
    return "+=";
  case TokenKind::MinusEqual:
    return "-=";
  case TokenKind::StarEqual:
    return "*=";
  case TokenKind::SlashEqual:
    return "/=";
  case TokenKind::PercentEqual:
    return "%=";
  case TokenKind::Amp:
    return "&";
  case TokenKind::Pipe:
    return "|";
  case TokenKind::Caret:
    return "^";
  case TokenKind::Tilde:
    return "~";
  case TokenKind::LessLess:
    return "<<";
  case TokenKind::GreaterGreater:
    return ">>";
  case TokenKind::AmpAmp:
    return "&&";
  case TokenKind::PipePipe:
    return "||";
  case TokenKind::Bang:
    return "!";
  case TokenKind::EqualEqual:
    return "==";
  case TokenKind::NotEqual:
    return "!=";
  case TokenKind::Less:
    return "<";
  case TokenKind::Greater:
    return ">";
  case TokenKind::LessEqual:
    return "<=";
  case TokenKind::GreaterEqual:
    return ">=";
  case TokenKind::FatArrow:
    return "=>";
  case TokenKind::CaretEqual:
    return "^=";
  case TokenKind::QuestionDot:
    return "?.";
  case TokenKind::QuestionQuestion:
    return "??";
  case TokenKind::DotDotDot:
    return "...";
  case TokenKind::Colon:
    return ":";
  case TokenKind::KwShared:
    return "shared ";
  default:
    return "(unknown)";
  }
}

void ASTPrinter::print(const Decl *decl) {
  if (decl)
    decl->accept(*this);
}
void ASTPrinter::print(const Stmt *stmt) {
  if (stmt)
    stmt->accept(*this);
}
void ASTPrinter::print(const Expr *expr) {
  if (expr)
    expr->accept(*this);
}
void ASTPrinter::print(const Type *type) {
  if (type)
    type->accept(*this);
}

void ASTPrinter::printVisibility(Visibility v) {
  switch (v) {
  case Visibility::Public:
    OS << "public ";
    break;
  case Visibility::Private:
    OS << "private ";
    break;
  case Visibility::Protected:
    OS << "protected ";
    break;
  default:
    break;
  }
}

// Types

void ASTPrinter::visitPrimitiveType(const PrimitiveType *type) {
  switch (type->getScalar()) {
  case PrimitiveType::Scalar::Void:
    OS << "void";
    break;
  case PrimitiveType::Scalar::Bool:
    OS << "bool";
    break;
  case PrimitiveType::Scalar::Char:
    OS << "char";
    break;
  case PrimitiveType::Scalar::String:
    OS << "string";
    break;
  case PrimitiveType::Scalar::I8:
    OS << "i8";
    break;
  case PrimitiveType::Scalar::I16:
    OS << "i16";
    break;
  case PrimitiveType::Scalar::I32:
    OS << "i32";
    break;
  case PrimitiveType::Scalar::I64:
    OS << "i64";
    break;
  case PrimitiveType::Scalar::ISize:
    OS << "isize";
    break;
  case PrimitiveType::Scalar::U8:
    OS << "u8";
    break;
  case PrimitiveType::Scalar::U16:
    OS << "u16";
    break;
  case PrimitiveType::Scalar::U32:
    OS << "u32";
    break;
  case PrimitiveType::Scalar::U64:
    OS << "u64";
    break;
  case PrimitiveType::Scalar::USize:
    OS << "usize";
    break;
  case PrimitiveType::Scalar::F8:
    OS << "quarter";
    break;
  case PrimitiveType::Scalar::F16:
    OS << "half";
    break;
  case PrimitiveType::Scalar::F32:
    OS << "f32";
    break;
  case PrimitiveType::Scalar::F64:
    OS << "f64";
    break;
  case PrimitiveType::Scalar::Int:
    OS << "int_literal";
    break;
  }
}

void ASTPrinter::visitPointerType(const PointerType *type) {
  print(type->getPointee());
  OS << "*";
}

void ASTPrinter::visitArrayType(const ArrayType *type) {
  if (auto nullElem =
          llvm::dyn_cast_or_null<NullableType>(type->getElementType())) {
    nullElem->getInner()->accept(*this);
    OS << "[]?";
  } else {
    type->getElementType()->accept(*this);
    OS << "[";
    if (type->getSizeExpr())
      type->getSizeExpr()->accept(*this);
    OS << "]";
  }
}

void ASTPrinter::visitSliceType(const SliceType *type) {
  print(type->getElementType());
  OS << "[]";
}

void ASTPrinter::visitNamedType(const NamedType *type) {
  OS << type->getName();
  if (!type->getGenericArgs().empty()) {
    OS << "<";
    for (size_t i = 0; i < type->getGenericArgs().size(); ++i) {
      if (i > 0)
        OS << ", ";
      print(type->getGenericArgs()[i].type.get());
    }
    OS << ">";
  }
}

void ASTPrinter::visitFunctionType(const FunctionType *type) {
  if (type->isInterruptFunc()) {
    OS << "interrupt ";
  }
  OS << "(";
  for (size_t i = 0; i < type->getParamTypes().size(); ++i) {
    if (i > 0)
      OS << ", ";
    print(type->getParamTypes()[i].get());
  }
  OS << ") => ";
  print(type->getReturnType());
}

void ASTPrinter::visitMapType(const MapType *type) {
  OS << "table<";
  print(type->getKeyType());
  OS << ", ";
  print(type->getValueType());
  OS << ">";
}

void ASTPrinter::visitReferenceType(const ReferenceType *type) {
  OS << "ref ";
  print(type->getInner());
}

void ASTPrinter::visitNullableType(const NullableType *type) {
  if (auto arrInner = llvm::dyn_cast_or_null<ArrayType>(type->getInner())) {
    arrInner->getElementType()->accept(*this);
    OS << "?[]";
  } else if (llvm::isa<WeakType>(type->getInner())) {
    type->getInner()->accept(*this);
  } else {
    type->getInner()->accept(*this);
    OS << "?";
  }
}

void ASTPrinter::visitAnyType(const AnyType *) { OS << "any"; }

void ASTPrinter::visitLockType(const LockType *type) {
  OS << "lock ";
  print(type->getInner());
}

void ASTPrinter::visitViewType(const ViewType *type) {
  OS << "view ";
  print(type->getInner());
}

void ASTPrinter::visitMutType(const MutType *type) {
  OS << "mut ";
  print(type->getInner());
}

void ASTPrinter::visitWeakType(const WeakType *type) {
  OS << "weak ";
  type->getInner()->accept(*this);
}

void ASTPrinter::visitEnumType(const EnumType *type) {
  OS << "enum " << type->getName();
}

void ASTPrinter::visitNullType(const NullType *) { OS << "null"; }

void ASTPrinter::visitVolatileType(const VolatileType *type) {
  OS << "volatile ";
  print(type->getInner());
}

void ASTPrinter::visitConstType(const ConstType *type) {
  OS << "const ";
  print(type->getInner());
}

void ASTPrinter::visitDecimalType(const DecimalType *type) {
  OS << type->toString();
}

void ASTPrinter::visitClosureType(const ClosureType *type) {
  OS << "closure(";
  for (size_t i = 0; i < type->getParamTypes().size(); ++i) {
    print(type->getParamTypes()[i].get());
    if (i < type->getParamTypes().size() - 1)
      OS << ", ";
  }
  OS << ") -> ";
  print(type->getReturnType());
}

void ASTPrinter::visitPromiseType(const PromiseType *type) {
  OS << "promise<";
  print(type->getInner());
  OS << ">";
}

// Expressions

void ASTPrinter::visitIntegerLiteral(const IntegerLiteral *expr) {
  OS << expr->getValue();
}
void ASTPrinter::visitFloatLiteral(const FloatLiteral *expr) {
  OS << expr->getValue();
}
void ASTPrinter::visitDecimalLiteral(const DecimalLiteral *expr) {
  OS << expr->getValue() << "d";
}
void ASTPrinter::visitStringLiteral(const StringLiteral *expr) {
  if (expr->isTemplateString()) {
    OS << expr->getValue();
  } else {
    OS << "\"" << expr->getValue() << "\"";
  }
}
void ASTPrinter::visitBoolLiteral(const BoolLiteral *expr) {
  OS << (expr->getValue() ? "true" : "false");
}
void ASTPrinter::visitCharLiteral(const CharLiteral *expr) {
  OS << "'" << expr->getValue() << "'";
}
void ASTPrinter::visitNullLiteral(const NullLiteral *) { OS << "null"; }
void ASTPrinter::visitIdentifierExpr(const IdentifierExpr *expr) {
  OS << expr->getName();
}

void ASTPrinter::visitBinaryExpr(const BinaryExpr *expr) {
  if (expr->getLHS())
    expr->getLHS()->accept(*this);
  OS << " " << tokenToString(expr->getOp()) << " ";
  if (expr->getRHS())
    expr->getRHS()->accept(*this);
}

void ASTPrinter::visitUnaryExpr(const UnaryExpr *expr) {
  if (expr->isPostfixOp()) {
    OS << "(";
    expr->getOperand()->accept(*this);
    OS << tokenToString(expr->getOp()) << ")";
  } else {
    OS << "(" << tokenToString(expr->getOp());
    expr->getOperand()->accept(*this);
    OS << ")";
  }
}

void ASTPrinter::visitCallExpr(const CallExpr *expr) {
  if (expr->getCallee())
    expr->getCallee()->accept(*this);
  OS << "(";
  for (size_t i = 0; i < expr->getArgs().size(); ++i) {
    if (expr->getArgs()[i]) {
      expr->getArgs()[i]->accept(*this);
    }
    if (i < expr->getArgs().size() - 1)
      OS << ", ";
  }
  OS << ")";
}

void ASTPrinter::visitAwaitExpr(const AwaitExpr *expr) {
  OS << "await ";
  expr->getExpr()->accept(*this);
}

void ASTPrinter::visitMemberExpr(const MemberExpr *expr) {
  expr->getObject()->accept(*this);
  OS << (expr->isOptionalAccess() ? "?." : ".") << expr->getName();
  if (expr->isVirtualMethod()) {
    OS << " /* virtual vtable[" << expr->getMemberIndex() << "] */";
  }
}

void ASTPrinter::visitIndexExpr(const IndexExpr *expr) {
  expr->getArray()->accept(*this);
  OS << "[";
  expr->getIndex()->accept(*this);
  OS << "]";
}

void ASTPrinter::visitLambdaExpr(const LambdaExpr *expr) {
  if (expr->isAsyncLambda()) {
    OS << "async ";
  }
  switch (expr->getCaptureMode()) {
  case CaptureMode::View:
    OS << "&";
    break;
  case CaptureMode::Mut:
    OS << "&mut ";
    break;
  case CaptureMode::Move:
    OS << "move ";
    break;
  case CaptureMode::Snapshot:
    break;
  }
  OS << "(";
  for (size_t i = 0; i < expr->getParams().size(); ++i) {
    if (i > 0)
      OS << ", ";
    if (expr->getParams()[i].getType()) {
      print(expr->getParams()[i].getType());
      OS << " ";
    }
    OS << expr->getParams()[i].getName();
  }
  OS << ") => ";
  if (expr->getBody())
    expr->getBody()->accept(*this);
  else
    OS << "{}";
}

void ASTPrinter::visitTernaryExpr(const TernaryExpr *expr) {
  OS << "(";
  expr->getCondition()->accept(*this);
  OS << " ? ";
  expr->getTrueBranch()->accept(*this);
  OS << " : ";
  expr->getFalseBranch()->accept(*this);
  OS << ")";
}

void ASTPrinter::visitCastExpr(const CastExpr *expr) {
  OS << "cast<";
  print(expr->getTargetType());
  OS << ">(";
  expr->getExpr()->accept(*this);
  OS << ")";
}

void ASTPrinter::visitBitcastExpr(const BitcastExpr *expr) {
  OS << "bitcast<";
  print(expr->getTargetType());
  OS << ">(";
  expr->getExpr()->accept(*this);
  OS << ")";
}

void ASTPrinter::visitNewExpr(const NewExpr *expr) {
  OS << "new ";
  print(expr->getType());
  OS << "(";
  for (size_t i = 0; i < expr->getArgs().size(); ++i) {
    if (i > 0)
      OS << ", ";
    expr->getArgs()[i]->accept(*this);
  }
  OS << ")";
}

void ASTPrinter::visitTemplateStringExpr(const TemplateStringExpr *expr) {
  OS << "`";
  for (const auto &part : expr->getParts()) {
    part->accept(*this);
  }
  OS << "`";
}

void ASTPrinter::visitThreadExpr(const ThreadExpr *expr) {
  OS << "new " << (expr->isWeakThread() ? "weak " : "") << "Thread() => ";
  expr->getBody()->accept(*this);
}

void ASTPrinter::visitInputExpr(const InputExpr *expr) {
  OS << "input(";
  if (expr->getPrompt()) {
    print(expr->getPrompt());
  }
  OS << ")";
}

void ASTPrinter::visitArrayLiteral(const ArrayLiteral *expr) {
  OS << "[";
  for (size_t i = 0; i < expr->getElements().size(); ++i) {
    if (i > 0)
      OS << ", ";
    expr->getElements()[i]->accept(*this);
  }
  OS << "]";
}

void ASTPrinter::visitMapLiteral(const MapLiteral *expr) {
  OS << "{";
  const auto &entries = expr->getEntries();
  for (size_t i = 0; i < entries.size(); ++i) {
    if (i > 0)
      OS << ", ";
    entries[i].first->accept(*this);
    OS << ": ";
    entries[i].second->accept(*this);
  }
  OS << "}";
}

void ASTPrinter::visitThisExpr(const ThisExpr *) { OS << "this"; }
void ASTPrinter::visitSuperExpr(const SuperExpr *) { OS << "super"; }

// Statements

void ASTPrinter::visitBlockStmt(const BlockStmt *stmt) {
  OS << " {\n";
  indentLevel++;
  for (const auto &s : stmt->getStatements()) {
    printIndent();
    s->accept(*this);
    OS << "\n";
  }
  indentLevel--;
  printIndent();
  OS << "}";
}

void ASTPrinter::visitExpressionStmt(const ExpressionStmt *stmt) {
  stmt->getExpr()->accept(*this);
  OS << ";";
}

void ASTPrinter::visitDeclStmt(const DeclStmt *stmt) {
  stmt->getDecl()->accept(*this);
  OS << ";";
}

void ASTPrinter::visitReturnStmt(const ReturnStmt *stmt) {
  OS << "return";
  if (stmt->getReturnValue()) {
    OS << " ";
    stmt->getReturnValue()->accept(*this);
  }
  OS << ";";
}

void ASTPrinter::visitIfStmt(const IfStmt *stmt) {
  OS << "if (";
  stmt->getCondition()->accept(*this);
  OS << ")";
  stmt->getThenStmt()->accept(*this);
  if (stmt->getElseStmt()) {
    OS << " else ";
    stmt->getElseStmt()->accept(*this);
  }
}

void ASTPrinter::visitWhileStmt(const WhileStmt *stmt) {
  OS << "while (";
  stmt->getCondition()->accept(*this);
  OS << ")";
  stmt->getBody()->accept(*this);
}

void ASTPrinter::visitDoWhileStmt(const DoWhileStmt *stmt) {
  OS << "do ";
  stmt->getBody()->accept(*this);
  OS << " while (";
  stmt->getCondition()->accept(*this);
  OS << ");";
}

void ASTPrinter::visitForStmt(const ForStmt *stmt) {
  OS << "for (";
  if (stmt->getInit())
    stmt->getInit()->accept(*this);
  else
    OS << ";";
  OS << " ";
  if (stmt->getCondition())
    stmt->getCondition()->accept(*this);
  OS << "; ";
  if (stmt->getIncrement())
    stmt->getIncrement()->accept(*this);
  OS << ")";
  stmt->getBody()->accept(*this);
}

void ASTPrinter::visitForInStmt(const ForInStmt *stmt) {
  OS << "for (";
  if (stmt->getIndexVariable()) {
    stmt->getIndexVariable()->accept(*this);
    OS << ", ";
  }
  stmt->getVariable()->accept(*this);
  OS << " in ";
  stmt->getCollection()->accept(*this);
  OS << ")";
  stmt->getBody()->accept(*this);
}

void ASTPrinter::visitSwitchStmt(const SwitchStmt *stmt) {
  OS << "switch (";
  stmt->getCondition()->accept(*this);
  OS << ") {\n";
  indentLevel++;
  for (const auto &c : stmt->getCases()) {
    printIndent();
    if (c.isDefaultCase())
      OS << "default";
    else {
      OS << "case ";
      for (size_t i = 0; i < c.getValues().size(); ++i) {
        if (i > 0)
          OS << ", ";
        c.getValues()[i]->accept(*this);
      }
    }
    OS << ":";
    c.getBody()->accept(*this);
    OS << "\n";
  }
  indentLevel--;
  printIndent();
  OS << "}";
}

void ASTPrinter::visitBreakStmt(const BreakStmt *) { OS << "break;"; }
void ASTPrinter::visitContinueStmt(const ContinueStmt *) { OS << "continue;"; }
void ASTPrinter::visitDeferStmt(const DeferStmt *stmt) {
  OS << "defer ";
  stmt->getDeferredStmt()->accept(*this);
}
void ASTPrinter::visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) {
  OS << "unsafe";
  visitBlockStmt(stmt);
}

void ASTPrinter::visitTryCatchStmt(const TryCatchStmt *stmt) {
  printIndent();
  OS << "try {\n";
  indentLevel++;
  if (stmt->getTryBody()) {
    stmt->getTryBody()->accept(*this);
  }
  indentLevel--;
  printIndent();
  OS << "}\n";

  for (const auto &clause : stmt->getCatches()) {
    printIndent();
    OS << "catch ";
    if (clause.var) {
      OS << "(";
      clause.var->accept(*this);
      OS << ") ";
    }
    OS << "{\n";
    indentLevel++;
    if (clause.body) {
      clause.body->accept(*this);
    }
    indentLevel--;
    printIndent();
    OS << "}\n";
  }

  if (stmt->getFinallyBody()) {
    printIndent();
    OS << "finally {\n";
    indentLevel++;
    stmt->getFinallyBody()->accept(*this);
    indentLevel--;
    printIndent();
    OS << "}\n";
  }
}

// Declarations

void ASTPrinter::visitModuleDecl(const ModuleDecl *decl) {
  OS << "module " << decl->getName() << " {\n";
  indentLevel++;
  for (const auto &d : decl->getDecls()) {
    d->accept(*this);
    OS << "\n";
  }
  indentLevel--;
  OS << "}\n";
}

void ASTPrinter::visitFunctionDecl(const FunctionDecl *decl) {
  printIndent();
  if (decl->isExternFunc()) {
    OS << "extern ";
    if (!decl->getExternLinkage().empty()) {
      OS << "\"" << decl->getExternLinkage() << "\" ";
    }
  }

  // --- System & Optimization Attributes ---
  if (decl->isInterruptFunc())
    OS << "interrupt ";
  if (decl->isNakedFunc())
    OS << "naked ";
  if (decl->isNoReturnFunc())
    OS << "noreturn ";
  if (decl->isNoInlineFunc())
    OS << "noinline ";
  if (decl->isInlineFunc())
    OS << "inline ";
  if (decl->isPureFunc())
    OS << "pure ";
  if (decl->isColdFunc())
    OS << "cold ";
  if (decl->isUsedFunc())
    OS << "used ";
  if (!decl->getSection().empty()) {
    OS << "section(\"" << decl->getSection() << "\") ";
  }

  // --- Standard Modifiers ---
  printVisibility(decl->getVisibility());
  if (decl->isStaticFunc())
    OS << "static ";
  if (decl->isAsyncFunc())
    OS << "async ";
  if (decl->isVirtualFunc()) {
    OS << "virtual ";
    if (decl->getVTableIndex() != -1)
      OS << "/*vtable_idx=" << decl->getVTableIndex() << "*/ ";
  }
  if (decl->isOverrideFunc())
    OS << " override";
  if (decl->isWeakFunc())
    OS << "weak ";

  bool isCtorDtor =
      (decl->getName() == "constructor" || decl->getName() == "destructor");
  if (!isCtorDtor) {
    if (decl->getReturnType()) {
      print(decl->getReturnType());
      OS << " ";
    } else {
      OS << "void ";
    }
  }

  // Then Function Name
  OS << decl->getName() << "(";

  // Then Parameters
  for (size_t i = 0; i < decl->getParams().size(); ++i) {
    if (i > 0)
      OS << ", ";
    print(decl->getParams()[i].type.get());
    OS << " " << decl->getParams()[i].name;
  }

  if (decl->isVariadicFunc()) {
    if (!decl->getParams().empty())
      OS << ", ";
    OS << "...";
  }

  OS << ")";

  // Then Body
  if (decl->getBody()) {
    OS << " ";
    decl->getBody()->accept(*this);
  } else {
    OS << ";";
  }
}

void ASTPrinter::visitVariableDecl(const VariableDecl *decl) {
  printIndent();
  if (decl->isExternVar())
    OS << "extern ";
  if (decl->isWeakVar())
    OS << "weak ";
  if (decl->isThreadLocalVar())
    OS << "thread_local ";
  if (decl->isUsedVar())
    OS << "used ";
  if (!decl->getSection().empty()) {
    OS << "section(\"" << decl->getSection() << "\") ";
  }
  if (decl->getAlignment() > 0) {
    OS << "align(" << decl->getAlignment() << ") ";
  }
  printVisibility(decl->getVisibility());
  if (decl->isStaticVar())
    OS << "static ";
  if (decl->isSharedVar())
    OS << "shared ";
  print(decl->getType());
  OS << " " << decl->getName();
  if (decl->getBitWidth() != -1) {
    OS << " : " << decl->getBitWidth();
  }
  if (decl->getInitializer()) {
    OS << " = ";
    decl->getInitializer()->accept(*this);
  }
}

void ASTPrinter::visitClassDecl(const ClassDecl *decl) {
  printIndent();
  if (decl->isPackedClass())
    OS << "packed ";
  if (decl->hasVTable())
    OS << "/*has_vtable*/ ";
  if (!decl->getSection().empty()) {
    OS << "section(\"" << decl->getSection() << "\") ";
  }
  if (decl->getAlignment() > 0) {
    OS << "align(" << decl->getAlignment() << ") ";
  }
  if (decl->isReferenceType())
    OS << "ref ";
  switch (decl->getAggregateKind()) {
  case AggregateKind::Struct:
    OS << "struct ";
    break;
  case AggregateKind::Union:
    OS << "union ";
    break;
  default:
    OS << "class ";
    break;
  }
  OS << decl->getName();
  if (!decl->getParentNames().empty()) {
    OS << "(";
    for (size_t i = 0; i < decl->getParentNames().size(); ++i) {
      if (i > 0)
        OS << ", ";
      OS << decl->getParentNames()[i];
    }
    OS << ")";
  }
  OS << " {\n";

  indentLevel++;
  for (const auto &member : decl->getMembers()) {
    member->accept(*this);
    if (member->getKind() == StmtKind::VariableDecl) {
      OS << ";\n";
    } else if (member->getKind() == StmtKind::FunctionDecl) {
      OS << "\n";
    }
  }
  indentLevel--;

  printIndent();
  OS << "}";
}

void ASTPrinter::visitGenericDecl(const GenericDecl *decl) {
  printIndent();
  OS << "generic <";
  const auto &params = decl->getTypeParams();
  for (size_t i = 0; i < params.size(); ++i) {
    if (params[i].isShared)
      OS << "shared ";
    OS << params[i].name;
    if (i < params.size() - 1)
      OS << ", ";
  }
  OS << ">\n";

  decl->getInnerDecl()->accept(*this);
}

void ASTPrinter::visitImportDecl(const ImportDecl *decl) {
  if (!decl->getSymbols().empty()) {
    OS << "from \"" << decl->getModuleName() << "\" import { ";
    for (size_t i = 0; i < decl->getSymbols().size(); ++i) {
      if (i > 0)
        OS << ", ";

      const auto &symPair = decl->getSymbols()[i];
      if (symPair.first == symPair.second) {
        OS << symPair.first;
      } else {
        OS << symPair.first << " as " << symPair.second;
      }
    }
    OS << " }";
  } else {
    OS << "import \"" << decl->getModuleName() << "\"";
    if (!decl->getAliasName().empty()) {
      OS << " as " << decl->getAliasName();
    }
  }
  OS << ";";
}

void ASTPrinter::visitEnumDecl(const EnumDecl *decl) {
  OS << "enum " << decl->getName() << " {\n";
  indentLevel++;
  for (const auto &c : decl->getCases()) {
    printIndent();
    OS << c.name;
    if (c.value) {
      OS << " = ";
      c.value->accept(*this);
    }
    OS << ",\n";
  }
  indentLevel--;
  printIndent();
  OS << "}";
}

void ASTPrinter::visitMacroDecl(const MacroDecl *decl) {
  OS << "macro " << decl->getName();
  if (decl->isFunctionMacro()) {
    OS << "(";
    for (size_t i = 0; i < decl->getParams().size(); ++i) {
      if (i > 0)
        OS << ", ";
      OS << decl->getParams()[i];
    }
    OS << ")";
  }
  OS << " {\n";
  indentLevel++;
  for (const auto &s : decl->getBody()) {
    printIndent();
    s->accept(*this);
    OS << "\n";
  }
  indentLevel--;
  printIndent();
  OS << "}";
}

void ASTPrinter::visitThrowStmt(const ThrowStmt *stmt) {
  printIndent();
  OS << "throw ";
  if (stmt->getExpr()) {
    stmt->getExpr()->accept(*this);
  }
  OS << ";\n";
}

void ASTPrinter::visitUsingDecl(const UsingDecl *decl) {
  OS << "using " << decl->getName() << " = ";
  print(decl->getTargetType());
  OS << ";";
}

void ASTPrinter::visitAsmExpr(const AsmExpr *expr) {
  OS << "asm";
  if (expr->getType() && !expr->getType()->isVoid()) {
    OS << "<";
    print(expr->getType());
    OS << ">";
  }
  OS << "(\"" << expr->getAssemblyStr() << "\")\n";

  indentLevel++;
  for (const auto &out : expr->getOutputs()) {
    printIndent();
    OS << "out(\"" << out.constraint << "\"(";
    out.expr->accept(*this);
    OS << "))\n";
  }
  for (const auto &in : expr->getInputs()) {
    printIndent();
    OS << "in(\"" << in.constraint << "\"(";
    in.expr->accept(*this);
    OS << "))\n";
  }
  for (const auto &inout : expr->getInouts()) {
    printIndent();
    OS << "inout(\"" << inout.constraint << "\"(";
    inout.expr->accept(*this);
    OS << "))\n";
  }
  if (!expr->getClobbers().empty()) {
    printIndent();
    OS << "clobber(";
    for (size_t i = 0; i < expr->getClobbers().size(); ++i) {
      if (i > 0)
        OS << ", ";
      OS << "\"" << expr->getClobbers()[i] << "\"";
    }
    OS << ")\n";
  }
  if (expr->getIsVolatile()) {
    printIndent();
    OS << "volatile";
  }
  indentLevel--;
}

void ASTPrinter::visitSizeOfExpr(const SizeOfExpr *expr) {
  OS << "sizeof(";
  expr->getExpr()->accept(*this);
  OS << ")";
}

void ASTPrinter::visitLockStmt(const LockStmt *stmt) {
  printIndent();
  if (stmt->isAsyncLock()) {
    OS << "async ";
  }
  OS << "lock ";
  if (stmt->getTarget()) {
    OS << "(";
    stmt->getTarget()->accept(*this);
    OS << ") ";
  }
  if (stmt->getBody()) {
    stmt->getBody()->accept(*this);
  }
}

} // namespace moksha
