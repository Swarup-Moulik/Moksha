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

class DiagnosticEngine;
class ModuleDecl;
class ASTContext;

class Parser {
public:
  explicit Parser(Lexer &lexer, ASTContext &context, llvm::SourceMgr &srcMgr,
                  DiagnosticEngine &diags);

  std::unique_ptr<ModuleDecl> parseModule();

private:
  Lexer &lexer;
  ASTContext &context;
  llvm::SourceMgr &srcMgr;
  DiagnosticEngine &Diags;

  Token curTok;
  Token nextTok;

  int loopDepth = 0;

  void consume();
  bool consumeIf(TokenKind kind);
  bool expect(TokenKind kind);
  bool expectGreater();
  void advance();
  bool peekIs(TokenKind kind) const { return nextTok.is(kind); }
  bool isStartOfDeclaration();
  void parseImportSymbolList(std::vector<std::string> &symbols);
  void error(const std::string &message);
  void synchronize();

  DeclPtr parseTopLevelDecl();
  DeclPtr parseImportDecl();
  DeclPtr parseClassDecl();
  DeclPtr parseGenericDecl();
  DeclPtr parseEnumDecl();
  DeclPtr parseMacroDecl();
  DeclPtr parseFunctionRest(TypePtr type, std::string name,
                            bool isAsync = false, bool isStatic = false,
                            bool isWeak = false,
                            Visibility vis = Visibility::Default);
  DeclPtr parseVariableRest(TypePtr type, std::string name,
                            bool isConst = false, bool isStatic = false,
                            bool isShared = false,
                            Visibility vis = Visibility::Default);
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
  StmtPtr parseThrowStmt();
  StmtPtr parseUnsafeBlock();
  StmtPtr parseLockStmt();
  StmtPtr parseForInStmt();
  StmtPtr parseAsmStmt();

  ExprPtr parseExpression();
  ExprPtr parseAssignment();
  ExprPtr parsePipe();
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
  ExprPtr parsePower();
  ExprPtr parsePrefix();
  ExprPtr parsePostfix();
  ExprPtr parsePrimary();
  ExprPtr parseNewExpr();
  ExprPtr parseInputExpr();

  ExprPtr parseStringLiteral();
  ExprPtr parseLambdaBody(std::vector<LambdaParam> params, CaptureMode mode);
  ExprPtr parseArrayLiteral();
  ExprPtr parseMapLiteral();
  ExprPtr parseTemplateString();

  TypePtr parseType();
  TypePtr parseClosureType();
};

} // namespace moksha
