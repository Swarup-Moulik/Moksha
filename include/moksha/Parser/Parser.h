#pragma once

#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Lexer/Lexer.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/SourceMgr.h"
#include <memory>
#include <vector>

namespace moksha {

class ModuleDecl;
class ASTContext;

class Parser {
public:
  // [FIX] Updated constructor
  explicit Parser(Lexer &lexer, ASTContext &context, llvm::SourceMgr &srcMgr);

  std::unique_ptr<ModuleDecl> parseModule();

private:
  Lexer &lexer;
  ASTContext &context;
  llvm::SourceMgr &srcMgr;

  Token curTok;
  Token nextTok;

  int loopDepth = 0;

  void consume();
  bool consumeIf(TokenKind kind);
  bool expect(TokenKind kind);
  void advance();
  bool peekIs(TokenKind kind) const { return nextTok.is(kind); }
  bool isStartOfDeclaration();

  // [FIX] Use string reference
  void error(const std::string &message);
  void synchronize();

  DeclPtr parseTopLevelDecl();
  DeclPtr parseImportDecl();
  DeclPtr parseClassDecl();
  DeclPtr parseGenericDecl();
  DeclPtr parseEnumDecl(); // [FIX] Added missing declaration

  DeclPtr parseFunctionRest(TypePtr type, std::string name);
  DeclPtr parseVariableRest(TypePtr type, std::string name);

  DeclPtr parseVariableDecl();
  StmtPtr parseVariableStmt();

  StmtPtr parseStatement();
  StmtPtr parseBlock();
  StmtPtr parseIfStmt();
  StmtPtr parseWhileStmt();
  StmtPtr parseDoWhileStmt();
  StmtPtr parseForStmt();
  StmtPtr parseSwitchStmt();
  StmtPtr parseReturnStmt();
  StmtPtr parseBreakStmt();
  StmtPtr parseContinueStmt();
  StmtPtr parseDeferStmt();
  StmtPtr parseTryCatchStmt();
  StmtPtr parseUnsafeBlock();
  StmtPtr parseLockStmt();

  ExprPtr parseExpression();
  ExprPtr parseAssignment();
  ExprPtr parseTernary();
  ExprPtr parseNullCoalescing();
  ExprPtr parseLogicalOr();
  ExprPtr parseLogicalAnd();
  ExprPtr parseBitwiseOr();
  ExprPtr parseBitwiseXor();
  ExprPtr parseBitwiseAnd();
  ExprPtr parseEquality();
  ExprPtr parseRelational();
  ExprPtr parseShift();
  ExprPtr parseAdditive();
  ExprPtr parseMultiplicative();
  ExprPtr parsePrefix();
  ExprPtr parsePostfix();
  ExprPtr parsePrimary();
  ExprPtr parseNewExpr();

  ExprPtr parseStringLiteral();
  ExprPtr parseLambdaBody(std::vector<LambdaParam> params);
  ExprPtr parseArrayLiteral();
  ExprPtr parseTemplateString();

  TypePtr parseType();
};

} // namespace moksha
