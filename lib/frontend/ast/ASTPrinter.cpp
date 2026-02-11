#include "moksha/AST/ASTPrinter.h"
#include "moksha/AST/ASTVisitor.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Lexer/Lexer.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

ASTPrinter::ASTPrinter(llvm::raw_ostream &OS) : OS(OS) {}

void ASTPrinter::printIndent() {
  for (int i = 0; i < indentLevel; ++i)
    OS << "  ";
}

std::string ASTPrinter::tokenToString(TokenKind kind) {
  switch (kind) {
  // Math
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

  // Increment/Decrement
  case TokenKind::PlusPlus:
    return "++";
  case TokenKind::MinusMinus:
    return "--";

  // Assignment
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

  // Bitwise
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

  // Logical
  case TokenKind::AmpAmp:
    return "&&";
  case TokenKind::PipePipe:
    return "||";
  case TokenKind::Bang:
    return "!";

  // Comparison
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

  // Misc
  case TokenKind::FatArrow:
    return "=>";
  case TokenKind::QuestionQuestion:
    return "??";
  case TokenKind::DotDotDot:
    return "...";

  default:
    llvm::errs() << "Warning: unknown token kind encountered: "
                 << static_cast<int>(kind) << "\n";
    return "(unknown)";
  }
}

// Public Entry Points
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

// --- Types ---

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
  }
}

void ASTPrinter::visitPointerType(const PointerType *type) {
  print(type->getPointee());
  OS << "*";
}

void ASTPrinter::visitArrayType(const ArrayType *type) {
  print(type->getElementType());
  OS << "[";
  if (auto size = type->getSizeExpr()) {
    size->accept(*this);
  }
  OS << "]";
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
  print(type->getInner());
  OS << "?";
}

void ASTPrinter::visitAnyType(const AnyType *type) { OS << "any"; }

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

void ASTPrinter::visitEnumType(const EnumType *type) {
  OS << "enum " << type->getName();
}

// --- Expressions ---

void ASTPrinter::visitIntegerLiteral(const IntegerLiteral *expr) {
  OS << expr->getValue();
}

void ASTPrinter::visitFloatLiteral(const FloatLiteral *expr) {
  OS << expr->getValue();
}

void ASTPrinter::visitStringLiteral(const StringLiteral *expr) {
  OS << "\"";
  for (unsigned char c : expr->getValue()) {
    if (c == '\\')
      OS << "\\\\";
    else if (c == '"')
      OS << "\\\"";
    else if (c == '\n')
      OS << "\\n";
    else if (c == '\t')
      OS << "\\t";
    else if (c == '\r')
      OS << "\\r";
    else if (std::isprint(c) || c >= 128)
      OS << c;
    else {
      OS << "\\x";
      OS.write_hex(c);
    }
  }
  OS << "\"";
}

void ASTPrinter::visitBoolLiteral(const BoolLiteral *expr) {
  OS << (expr->getValue() ? "true" : "false");
}

void ASTPrinter::visitCharLiteral(const CharLiteral *expr) {
  OS << "'";
  unsigned char c = static_cast<unsigned char>(expr->getValue());
  if (c == '\\')
    OS << "\\\\";
  else if (c == '\'')
    OS << "\\'";
  else if (c == '\n')
    OS << "\\n";
  else if (c == '\t')
    OS << "\\t";
  else if (c == '\r')
    OS << "\\r";
  else if (std::isprint(c) || c >= 128)
    OS << static_cast<char>(c);
  else {
    OS << "\\x";
    OS.write_hex(c);
  }
  OS << "'";
}

void ASTPrinter::visitNullLiteral(const NullLiteral *expr) { OS << "null"; }

void ASTPrinter::visitIdentifierExpr(const IdentifierExpr *expr) {
  OS << expr->getName();
}

void ASTPrinter::visitBinaryExpr(const BinaryExpr *expr) {
  OS << "(";
  expr->getLHS()->accept(*this);
  OS << " " << tokenToString(expr->getOp()) << " ";
  expr->getRHS()->accept(*this);
  OS << ")";
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
  expr->getCallee()->accept(*this);
  OS << "(";
  for (size_t i = 0; i < expr->getArgs().size(); ++i) {
    if (i > 0)
      OS << ", ";
    expr->getArgs()[i]->accept(*this);
  }
  OS << ")";
}

void ASTPrinter::visitMemberExpr(const MemberExpr *expr) {
  expr->getObject()->accept(*this);
  OS << (expr->isOptionalAccess() ? "?." : ".") << expr->getMemberName();
}

void ASTPrinter::visitIndexExpr(const IndexExpr *expr) {
  expr->getArray()->accept(*this);
  OS << "[";
  expr->getIndex()->accept(*this);
  OS << "]";
}

void ASTPrinter::visitLambdaExpr(const LambdaExpr *expr) {
  OS << "(";
  for (size_t i = 0; i < expr->getParams().size(); ++i) {
    if (i > 0)
      OS << ", ";
    if (expr->getParams()[i].getType()) {
      print(expr->getParams()[i].getType());
      OS << " ";
    } else {
      OS << "_ ";
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
  OS << "new ";
  if (expr->isWeakThread())
    OS << "weak ";
  OS << "Thread() => ";
  expr->getBody()->accept(*this);
}

void ASTPrinter::visitArrayLiteral(const ArrayLiteral *expr) {
  const auto &elements = expr->getElements();
  if (elements.size() > 10) {
    OS << "[\n";
    indentLevel++;
    for (const auto &e : elements) {
      printIndent();
      e->accept(*this);
      OS << ",\n";
    }
    indentLevel--;
    printIndent();
    OS << "]";
  } else {
    OS << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
      if (i > 0)
        OS << ", ";
      elements[i]->accept(*this);
    }
    OS << "]";
  }
}

// [FIX] Implemented missing methods
void ASTPrinter::visitThisExpr(const ThisExpr *expr) { OS << "this"; }

void ASTPrinter::visitSuperExpr(const SuperExpr *expr) { OS << "super"; }

// --- Statements ---

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
  if (stmt->getBody()) {
    stmt->getBody()->accept(*this);
  } else {
    OS << "{}";
  }
}

void ASTPrinter::visitDoWhileStmt(const DoWhileStmt *stmt) {
  OS << "do ";
  if (stmt->getBody()) {
    stmt->getBody()->accept(*this);
  } else {
    OS << "{}";
  }
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
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
  else
    OS << "{}";
}

void ASTPrinter::visitForInStmt(const ForInStmt *stmt) {
  OS << "for (";
  stmt->getVariable()->accept(*this);
  OS << " in ";
  stmt->getCollection()->accept(*this);
  OS << ")";
  if (stmt->getBody())
    stmt->getBody()->accept(*this);
  else
    OS << "{}";
}

void ASTPrinter::visitSwitchStmt(const SwitchStmt *stmt) {
  OS << "switch (";
  stmt->getCondition()->accept(*this);
  OS << ") {\n";
  indentLevel++;
  for (const auto &c : stmt->getCases()) {
    printIndent();
    if (c.isDefaultCase()) {
      OS << "default";
    } else {
      OS << "case ";
      bool first = true;
      for (const auto &v : c.getValues()) {
        if (!first)
          OS << ", ";
        v->accept(*this);
        first = false;
      }
    }
    OS << ":";
    if (c.getBody())
      c.getBody()->accept(*this);
    OS << "\n";
  }
  indentLevel--;
  printIndent();
  OS << "}";
}

void ASTPrinter::visitBreakStmt(const BreakStmt *stmt) { OS << "break;"; }
void ASTPrinter::visitContinueStmt(const ContinueStmt *stmt) {
  OS << "continue;";
}

void ASTPrinter::visitDeferStmt(const DeferStmt *stmt) {
  OS << "defer ";
  stmt->getDeferredStmt()->accept(*this);
}

void ASTPrinter::visitUnsafeBlockStmt(const UnsafeBlockStmt *stmt) {
  OS << "unsafe";
  visitBlockStmt(stmt);
}

void ASTPrinter::visitTryCatchStmt(const TryCatchStmt *stmt) {
  OS << "try";
  stmt->getTryBody()->accept(*this);
  if (stmt->getCatchBody()) {
    OS << " catch";
    if (stmt->getCatchVar()) {
      OS << " (";
      stmt->getCatchVar()->accept(*this);
      OS << ")";
    }
    stmt->getCatchBody()->accept(*this);
  }
  if (stmt->getFinallyBody()) {
    OS << " finally";
    stmt->getFinallyBody()->accept(*this);
  }
}

// --- Declarations ---

void ASTPrinter::visitModuleDecl(const ModuleDecl *decl) {
  OS << "module " << decl->getName() << " {\n";
  indentLevel++;
  for (const auto &d : decl->getDecls()) {
    printIndent();
    d->accept(*this);
    OS << "\n";
  }
  indentLevel--;
  OS << "}\n";
}

void ASTPrinter::visitFunctionDecl(const FunctionDecl *decl) {
  if (decl->isAsyncFunc())
    OS << "async ";
  OS << "func " << decl->getName() << "(";
  for (size_t i = 0; i < decl->getParams().size(); ++i) {
    if (i > 0)
      OS << ", ";
    print(decl->getParams()[i].type.get());
    OS << " " << decl->getParams()[i].name;
  }
  OS << ") ";
  if (decl->getReturnType()) {
    OS << "-> ";
    print(decl->getReturnType());
    OS << " ";
  }
  if (decl->getBody()) {
    decl->getBody()->accept(*this);
  } else {
    OS << ";";
  }
}

void ASTPrinter::visitVariableDecl(const VariableDecl *decl) {
  print(decl->getType());
  OS << " " << decl->getName();
  if (decl->getInitializer()) {
    OS << " = ";
    decl->getInitializer()->accept(*this);
  }
}

void ASTPrinter::visitClassDecl(const ClassDecl *decl) {
  if (decl->isReferenceType())
    OS << "ref ";
  OS << "class " << decl->getName() << " {\n";
  indentLevel++;
  for (const auto &member : decl->getMembers()) {
    printIndent();
    member->accept(*this);
    OS << ";\n";
  }
  indentLevel--;
  printIndent();
  OS << "}";
}

void ASTPrinter::visitGenericDecl(const GenericDecl *decl) {
  OS << "generic <";
  for (size_t i = 0; i < decl->getTypeParams().size(); ++i) {
    if (i > 0)
      OS << ", ";
    OS << decl->getTypeParams()[i];
  }
  OS << "> ";
  decl->getInnerDecl()->accept(*this);
}

void ASTPrinter::visitImportDecl(const ImportDecl *decl) {
  if (!decl->getSymbols().empty()) {
    OS << "from \"" << decl->getModuleName() << "\" import { ";
    for (size_t i = 0; i < decl->getSymbols().size(); ++i) {
      if (i > 0)
        OS << ", ";
      OS << decl->getSymbols()[i];
    }
    OS << " }";
  } else {
    OS << "import \"" << decl->getModuleName() << "\"";
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

// Public API wrapper
void printAST(const Decl *decl, llvm::raw_ostream &OS) {
  ASTPrinter printer(OS);
  printer.print(decl);
}

} // namespace moksha
