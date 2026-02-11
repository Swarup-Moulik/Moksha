#include "moksha/Parser/Parser.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

// --- Parser Implementation ---

static std::string tokenKindToString(TokenKind kind);

Parser::Parser(Lexer &lexer, ASTContext &context, llvm::SourceMgr &srcMgr)
    : lexer(lexer), context(context), srcMgr(srcMgr) {
  curTok = lexer.next();
  nextTok = lexer.next();
}

void Parser::advance() {
  curTok = nextTok;
  nextTok = lexer.next();
  while (curTok.is(TokenKind::Comment)) {
    curTok = nextTok;
    nextTok = lexer.next();
  }
}

// [NEW] Helper for better error messages
static std::string tokenKindToString(TokenKind kind) {
  switch (kind) {
  case TokenKind::Eof:
    return "end of file";
  case TokenKind::Error:
    return "error";
  case TokenKind::Identifier:
    return "identifier";
  case TokenKind::IntegerLiteral:
    return "integer literal";
  case TokenKind::FloatLiteral:
    return "float literal";
  case TokenKind::StringLiteral:
    return "string literal";
  case TokenKind::TemplateString:
    return "template string";
  case TokenKind::CharLiteral:
    return "char literal";
  case TokenKind::StringFragment:
    return "string fragment";
  case TokenKind::InterpolationStart:
    return "${";
  case TokenKind::InterpolationEnd:
    return "}";

  // --- Keywords ---
  case TokenKind::KwClass:
    return "'class'";
  case TokenKind::KwRef:
    return "'ref'";
  case TokenKind::KwWeak:
    return "'weak'";
  case TokenKind::KwShared:
    return "'shared'";
  case TokenKind::KwNew:
    return "'new'";
  case TokenKind::KwDelete:
    return "'delete'";
  case TokenKind::KwUnsafe:
    return "'unsafe'";
  case TokenKind::KwConstructor:
    return "'constructor'";
  case TokenKind::KwDestructor:
    return "'destructor'";
  case TokenKind::KwThis:
    return "'this'";
  case TokenKind::KwNull:
    return "'null'";
  case TokenKind::KwIf:
    return "'if'";
  case TokenKind::KwElse:
    return "'else'";
  case TokenKind::KwWhile:
    return "'while'";
  case TokenKind::KwDo:
    return "'do'";
  case TokenKind::KwFor:
    return "'for'";
  case TokenKind::KwIn:
    return "'in'";
  case TokenKind::KwSwitch:
    return "'switch'";
  case TokenKind::KwCase:
    return "'case'";
  case TokenKind::KwDefault:
    return "'default'";
  case TokenKind::KwReturn:
    return "'return'";
  case TokenKind::KwBreak:
    return "'break'";
  case TokenKind::KwContinue:
    return "'continue'";
  case TokenKind::KwDefer:
    return "'defer'";
  case TokenKind::KwWith:
    return "'with'";
  case TokenKind::KwTry:
    return "'try'";
  case TokenKind::KwCatch:
    return "'catch'";
  case TokenKind::KwThrow:
    return "'throw'";
  case TokenKind::KwFinally:
    return "'finally'";
  case TokenKind::KwVoid:
    return "'void'";
  case TokenKind::KwInt:
    return "'int'";
  case TokenKind::KwFloat:
    return "'float'";
  case TokenKind::KwBool:
    return "'bool'";
  case TokenKind::KwString:
    return "'string'";
  case TokenKind::KwChar:
    return "'char'";
  case TokenKind::KwAny:
    return "'any'";
  case TokenKind::KwTrue:
    return "'true'";
  case TokenKind::KwFalse:
    return "'false'";
  case TokenKind::KwPublic:
    return "'public'";
  case TokenKind::KwPrivate:
    return "'private'";
  case TokenKind::KwProtected:
    return "'protected'";
  case TokenKind::KwConst:
    return "'const'";
  case TokenKind::KwStatic:
    return "'static'";
  case TokenKind::KwAsync:
    return "'async'";
  case TokenKind::KwAwait:
    return "'await'";
  case TokenKind::KwImport:
    return "'import'";
  case TokenKind::KwFrom:
    return "'from'";
  case TokenKind::KwMacro:
    return "'macro'";
  case TokenKind::KwTable:
    return "'table'";
  case TokenKind::KwGeneric:
    return "'generic'";
  case TokenKind::KwStruct:
    return "'struct'";
  case TokenKind::KwUnion:
    return "'union'";
  case TokenKind::KwPacked:
    return "'packed'";
  case TokenKind::KwVolatile:
    return "'volatile'";
  case TokenKind::KwAlign:
    return "'align'";
  case TokenKind::KwSizeof:
    return "'sizeof'";
  case TokenKind::KwInterrupt:
    return "'interrupt'";
  case TokenKind::KwNaked:
    return "'naked'";
  case TokenKind::KwLock:
    return "'lock'";
  case TokenKind::KwView:
    return "'view'";
  case TokenKind::KwMut:
    return "'mut'";

  // --- Operators ---
  case TokenKind::Plus:
    return "'+'";
  case TokenKind::Minus:
    return "'-'";
  case TokenKind::Star:
    return "'*'";
  case TokenKind::Slash:
    return "'/'";
  case TokenKind::Percent:
    return "'%'";
  case TokenKind::Power:
    return "'**'";
  case TokenKind::PlusPlus:
    return "'++'";
  case TokenKind::MinusMinus:
    return "'--'";
  case TokenKind::Equal:
    return "'='";
  case TokenKind::PlusEqual:
    return "'+='";
  case TokenKind::MinusEqual:
    return "'-='";
  case TokenKind::StarEqual:
    return "'*='";
  case TokenKind::SlashEqual:
    return "'/='";
  case TokenKind::PercentEqual:
    return "'%='";
  case TokenKind::Amp:
    return "'&'";
  case TokenKind::Pipe:
    return "'|'";
  case TokenKind::Caret:
    return "'^'";
  case TokenKind::Tilde:
    return "'~'";
  case TokenKind::AmpEqual:
    return "'&='";
  case TokenKind::PipeEqual:
    return "'|='";
  case TokenKind::CaretEqual:
    return "'^='";
  case TokenKind::LessLess:
    return "'<<'";
  case TokenKind::GreaterGreater:
    return "'>>'";
  case TokenKind::LessLessEqual:
    return "'<<='";
  case TokenKind::GreaterGreaterEqual:
    return "'>>='";
  case TokenKind::AmpAmp:
    return "'&&'";
  case TokenKind::PipePipe:
    return "'||'";
  case TokenKind::Bang:
    return "'!'";
  case TokenKind::EqualEqual:
    return "'=='";
  case TokenKind::NotEqual:
    return "'!='";
  case TokenKind::Less:
    return "'<'";
  case TokenKind::LessEqual:
    return "'<='";
  case TokenKind::Greater:
    return "'>'";
  case TokenKind::GreaterEqual:
    return "'>='";
  case TokenKind::FatArrow:
    return "'=>'";
  case TokenKind::Dot:
    return "'.'";
  case TokenKind::DotDotDot:
    return "'...'";
  case TokenKind::QuestionDot:
    return "'?.'";
  case TokenKind::QuestionQuestion:
    return "'??'";
  case TokenKind::Question:
    return "'?'";
  case TokenKind::Colon:
    return "':'";
  case TokenKind::Comma:
    return "','";
  case TokenKind::Semicolon:
    return "';'";
  case TokenKind::LParen:
    return "'('";
  case TokenKind::RParen:
    return "')'";
  case TokenKind::LBrace:
    return "'{'";
  case TokenKind::RBrace:
    return "'}'";
  case TokenKind::LBracket:
    return "'['";
  case TokenKind::RBracket:
    return "']'";

  default:
    return "token";
  }
}

void Parser::consume() { advance(); }

bool Parser::consumeIf(TokenKind kind) {
  if (curTok.is(kind)) {
    advance();
    return true;
  }
  return false;
}

bool Parser::expect(TokenKind kind) {
  if (curTok.is(kind)) {
    advance();
    return true;
  }
  // [FIX] Correct string concatenation
  std::string msg = "Expected " + tokenKindToString(kind) + ", but found " +
                    curTok.getSpelling().str();
  error(msg);
  return false;
}

void Parser::error(const std::string &message) {
  srcMgr.PrintMessage(curTok.location, llvm::SourceMgr::DK_Error, message);
}

// void Parser::error(const std::string &message) {
//   llvm::errs() << "Error at " << curTok.location.getPointer() << ": " <<
//   message
//                << "\n";
// }

void Parser::synchronize() {
  advance();
  while (curTok.isNot(TokenKind::Eof)) {
    if (curTok.is(TokenKind::Semicolon)) {
      advance();
      return;
    }
    switch (curTok.kind) {
    case TokenKind::KwClass:
    case TokenKind::KwIf:
    case TokenKind::KwWhile:
    case TokenKind::KwFor:
    case TokenKind::KwReturn:
    case TokenKind::KwImport:
      return;
    default:
      advance();
    }
  }
}

bool Parser::isStartOfDeclaration() {
  // 1. keywords that always start a declaration
  if (curTok.isAny(TokenKind::KwImport, TokenKind::KwFrom, TokenKind::KwGeneric,
                   TokenKind::KwEnum, TokenKind::KwClass, TokenKind::KwStruct,
                   TokenKind::KwUnion, TokenKind::KwRef, TokenKind::KwThread))
    return true;

  // 2. Primitive types start declarations (int x, void foo)
  if (curTok.isAny(TokenKind::KwInt, TokenKind::KwFloat, TokenKind::KwBool,
                   TokenKind::KwString, TokenKind::KwVoid, TokenKind::KwAny,
                   TokenKind::KwChar, TokenKind::KwISize, TokenKind::KwUSize,
                   TokenKind::KwShort, TokenKind::KwLong, TokenKind::KwDouble,
                   TokenKind::KwHalf, TokenKind::KwQuarter))
    return true;

  // 3. Identifier Ambiguity: "MyType x" (Decl) vs "myVar = 5" (Stmt)
  if (curTok.is(TokenKind::Identifier)) {
    // "Type Name" -> Declaration
    if (nextTok.is(TokenKind::Identifier))
      return true;
    // "Type<...> Name" -> Declaration
    if (nextTok.is(TokenKind::Less))
      return true;
    // "Type[] Name" -> Declaration
    if (nextTok.is(TokenKind::LBracket))
      return true;
    // "Type* Name" -> Declaration
    if (nextTok.is(TokenKind::Star))
      return true;
    // "Type? Name" -> Declaration
    if (nextTok.is(TokenKind::Question))
      return true;
  }

  return false;
}

// --- Top Level ---

std::unique_ptr<ModuleDecl> Parser::parseModule() {
  std::vector<DeclPtr> decls;
  std::vector<StmtPtr> scriptStatements;
  SourceLocation startLoc = curTok.location;

  while (curTok.isNot(TokenKind::Eof)) {
    if (isStartOfDeclaration()) {
      DeclPtr decl = parseTopLevelDecl();
      if (decl) {
        // [FIX] If it's a Variable, move it to the script body (inside main)
        // This ensures 'int x = 10;' runs in order with other statements.
        if (decl->getKind() == StmtKind::VariableDecl) {
          scriptStatements.push_back(
              std::make_unique<DeclStmt>(std::move(decl), decl->getLoc()));
        } else {
          // Functions, Classes, Enums stay global
          decls.push_back(std::move(decl));
        }
      } else {
        synchronize();
      }
    } else {
      // It's a statement (if, while, expression)
      if (auto stmt = parseStatement()) {
        scriptStatements.push_back(std::move(stmt));
      } else {
        synchronize();
      }
    }
  }

  // Wrap all script statements (variables + logic) in a synthetic 'main'
  if (!scriptStatements.empty()) {
    bool hasMain = false;
    for (const auto &d : decls) {
      if (d->getName() == "main") {
        hasMain = true;
        break;
      }
    }

    if (hasMain) {
      error(
          "Cannot mix top-level statements with an explicit 'main' function.");
    } else {
      auto body =
          std::make_unique<BlockStmt>(std::move(scriptStatements), startLoc);
      auto voidType = std::make_unique<PrimitiveType>(
          PrimitiveType::Scalar::Void, startLoc);

      auto mainFunc = std::make_unique<FunctionDecl>(
          std::move(voidType), "main", std::vector<FunctionDecl::Param>{},
          std::move(body), false, startLoc);

      decls.push_back(std::move(mainFunc));
    }
  }

  return std::make_unique<ModuleDecl>("main", std::move(decls), startLoc);
}

DeclPtr Parser::parseTopLevelDecl() {
  if (curTok.is(TokenKind::KwImport) || curTok.is(TokenKind::KwFrom))
    return parseImportDecl();
  if (curTok.is(TokenKind::KwGeneric))
    return parseGenericDecl();
  if (curTok.is(TokenKind::KwEnum))
    return parseEnumDecl();
  if (curTok.isAny(TokenKind::KwClass, TokenKind::KwStruct, TokenKind::KwRef,
                   TokenKind::KwUnion))
    return parseClassDecl();

  TypePtr type = parseType();
  if (!type)
    return nullptr;

  std::string name = curTok.getSpelling().str();
  expect(TokenKind::Identifier);

  if (curTok.is(TokenKind::LParen)) {
    return parseFunctionRest(std::move(type), name);
  } else {
    return parseVariableRest(std::move(type), name);
  }
}

DeclPtr Parser::parseFunctionRest(TypePtr type, std::string name) {
  expect(TokenKind::LParen);

  // [FIX] Use FunctionDecl::Param instead of std::pair
  std::vector<FunctionDecl::Param> params;

  if (curTok.isNot(TokenKind::RParen)) {
    do {
      consumeIf(TokenKind::KwRef);
      TypePtr paramType = parseType();
      if (!paramType)
        break;

      std::string paramName;
      if (curTok.is(TokenKind::Identifier)) {
        paramName = curTok.getSpelling().str();
        consume();
      }
      // Construct the struct
      params.push_back({std::move(paramType), paramName});
    } while (consumeIf(TokenKind::Comma));
  }
  expect(TokenKind::RParen);

  StmtPtr body = nullptr;
  if (curTok.is(TokenKind::LBrace)) {
    body = parseBlock();
  } else {
    expect(TokenKind::Semicolon);
  }

  return std::make_unique<FunctionDecl>(std::move(type), name,
                                        std::move(params), std::move(body),
                                        false, curTok.location);
}

DeclPtr Parser::parseVariableRest(TypePtr type, std::string name) {
  ExprPtr init = nullptr;
  if (consumeIf(TokenKind::Equal)) {
    init = parseExpression();
  }
  consumeIf(TokenKind::Semicolon);
  return std::make_unique<VariableDecl>(std::move(type), name, std::move(init),
                                        curTok.location);
}

DeclPtr Parser::parseVariableDecl() {
  TypePtr type = parseType();
  if (!type)
    return nullptr;
  std::string name = curTok.getSpelling().str();
  expect(TokenKind::Identifier);
  return parseVariableRest(std::move(type), name);
}

StmtPtr Parser::parseVariableStmt() {
  DeclPtr decl = parseVariableDecl();
  if (!decl)
    return nullptr;
  return std::make_unique<DeclStmt>(std::move(decl), curTok.location);
}

DeclPtr Parser::parseImportDecl() {
  std::string moduleName;
  std::vector<std::string> symbols;
  SourceLocation loc = curTok.location;

  if (consumeIf(TokenKind::KwFrom)) {
    if (curTok.is(TokenKind::StringLiteral)) {
      moduleName = curTok.getSpelling().str();
      consume();
    }
    expect(TokenKind::KwImport);
    expect(TokenKind::LBrace);
    while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
      if (curTok.is(TokenKind::Identifier)) {
        symbols.push_back(curTok.getSpelling().str());
        consume();
      }
      consumeIf(TokenKind::Comma);
    }
    expect(TokenKind::RBrace);
  } else {
    expect(TokenKind::KwImport);
    if (curTok.is(TokenKind::StringLiteral)) {
      moduleName = curTok.getSpelling().str();
      consume();
    }
  }
  consumeIf(TokenKind::Semicolon);
  return std::make_unique<ImportDecl>(moduleName, symbols, loc);
}

DeclPtr Parser::parseGenericDecl() {
  SourceLocation loc = curTok.location;
  expect(TokenKind::KwGeneric);

  // Parse Type Parameters: <T, U>
  std::vector<std::string> typeParams;
  if (consumeIf(TokenKind::Less)) {
    do {
      if (curTok.is(TokenKind::Identifier)) {
        typeParams.push_back(curTok.getSpelling().str());
        consume();
      } else {
        error("Expected type parameter name");
      }
    } while (consumeIf(TokenKind::Comma));
    expect(TokenKind::Greater);
  }

  // Parse Inner Declaration (Class, Function, etc.)
  DeclPtr innerDecl = parseTopLevelDecl();
  if (!innerDecl) {
    error("Expected declaration after generic parameters");
    return nullptr;
  }

  return std::make_unique<GenericDecl>("generic_wrapper", std::move(typeParams),
                                       std::move(innerDecl), loc);
}

DeclPtr Parser::parseClassDecl() {
  bool isRef = consumeIf(TokenKind::KwRef);
  SourceLocation loc = curTok.location;

  // [FIX] Explicitly check for class/struct/union keywords
  if (curTok.isAny(TokenKind::KwClass, TokenKind::KwStruct,
                   TokenKind::KwUnion)) {
    consume();
  } else {
    error("Expected 'class', 'struct', or 'union'");
    return nullptr;
  }

  std::string name = curTok.getSpelling().str();
  expect(TokenKind::Identifier);

  if (consumeIf(TokenKind::Colon)) {
    parseType(); // Inheritance placeholder
  }

  expect(TokenKind::LBrace);
  std::vector<DeclPtr> members;

  while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
    if (curTok.isAny(TokenKind::KwPublic, TokenKind::KwPrivate,
                     TokenKind::KwProtected))
      consume();

    TypePtr memberType = parseType();
    if (!memberType) {
      synchronize();
      continue;
    }

    std::string memName = curTok.getSpelling().str();
    expect(TokenKind::Identifier);

    if (curTok.is(TokenKind::LParen)) {
      members.push_back(parseFunctionRest(std::move(memberType), memName));
    } else {
      members.push_back(parseVariableRest(std::move(memberType), memName));
    }
  }
  expect(TokenKind::RBrace);
  return std::make_unique<ClassDecl>(name, std::move(members), isRef, loc);
}

DeclPtr Parser::parseEnumDecl() {
  SourceLocation loc = curTok.location;
  consume(); // eat 'enum'

  std::string name = curTok.getSpelling().str();
  expect(TokenKind::Identifier);
  expect(TokenKind::LBrace);

  std::vector<EnumDecl::Case> cases;

  while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
    std::string caseName = curTok.getSpelling().str();
    expect(TokenKind::Identifier);

    ExprPtr value = nullptr;
    if (consumeIf(TokenKind::Equal)) {
      value = parseExpression();
    }

    cases.push_back({caseName, std::move(value)});
    consumeIf(TokenKind::Comma);
  }

  expect(TokenKind::RBrace);
  return std::make_unique<EnumDecl>(name, std::move(cases), loc);
}

// --- Statements ---

StmtPtr Parser::parseStatement() {
  switch (curTok.kind) {
  case TokenKind::LBrace:
    return parseBlock();
  case TokenKind::KwIf:
    return parseIfStmt();
  case TokenKind::KwWhile:
    return parseWhileStmt();
  case TokenKind::KwDo:
    return parseDoWhileStmt();
  case TokenKind::KwFor:
    return parseForStmt();
  case TokenKind::KwSwitch:
    return parseSwitchStmt();
  case TokenKind::KwReturn:
    return parseReturnStmt();
  case TokenKind::KwBreak:
    return parseBreakStmt();
  case TokenKind::KwContinue:
    return parseContinueStmt();
  case TokenKind::KwDefer:
    return parseDeferStmt();
  case TokenKind::KwTry:
    return parseTryCatchStmt();
  case TokenKind::KwUnsafe:
    return parseUnsafeBlock();
  default: {
    // Variable Declaration
    if (curTok.isAny(TokenKind::KwInt, TokenKind::KwFloat, TokenKind::KwBool,
                     TokenKind::KwString, TokenKind::KwVoid)) {
      return parseVariableStmt();
    }
    // Ambiguity Check
    if (curTok.is(TokenKind::Identifier) && peekIs(TokenKind::Identifier)) {
      return parseVariableStmt();
    }

    // Expression Statement
    ExprPtr expr = parseExpression();

    // [FIX] Check for null to ensure we don't create an invalid statement
    if (!expr) {
      return nullptr; // Caller (parseBlock) will call synchronize()
    }

    consumeIf(TokenKind::Semicolon);
    return std::make_unique<ExpressionStmt>(std::move(expr), curTok.location);
  }
  }
}

StmtPtr Parser::parseBlock() {
  SourceLocation loc = curTok.location;
  expect(TokenKind::LBrace);
  std::vector<StmtPtr> stmts;
  while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
    if (auto stmt = parseStatement()) {
      stmts.push_back(std::move(stmt));
    } else {
      synchronize();
    }
  }
  expect(TokenKind::RBrace);
  return std::make_unique<BlockStmt>(std::move(stmts), loc);
}

StmtPtr Parser::parseIfStmt() {
  SourceLocation loc = curTok.location;
  consume();
  expect(TokenKind::LParen);
  auto cond = parseExpression();
  expect(TokenKind::RParen);
  auto thenStmt = parseStatement();
  StmtPtr elseStmt = nullptr;
  if (consumeIf(TokenKind::KwElse))
    elseStmt = parseStatement();
  return std::make_unique<IfStmt>(std::move(cond), std::move(thenStmt),
                                  std::move(elseStmt), loc);
}

StmtPtr Parser::parseWhileStmt() {
  SourceLocation loc = curTok.location;
  consume();
  expect(TokenKind::LParen);
  auto cond = parseExpression();
  expect(TokenKind::RParen);
  loopDepth++;
  auto body = parseStatement();
  loopDepth--;
  return std::make_unique<WhileStmt>(std::move(cond), std::move(body), loc);
}

StmtPtr Parser::parseDoWhileStmt() {
  SourceLocation loc = curTok.location;
  consume();
  loopDepth++;
  auto body = parseStatement();
  loopDepth--;
  expect(TokenKind::KwWhile);
  expect(TokenKind::LParen);
  auto cond = parseExpression();
  expect(TokenKind::RParen);
  consumeIf(TokenKind::Semicolon);
  return std::make_unique<DoWhileStmt>(std::move(body), std::move(cond), loc);
}

// [REPLACE] parseForStmt

StmtPtr Parser::parseForStmt() {
  SourceLocation loc = curTok.location;
  consume(); // consume 'for'
  expect(TokenKind::LParen);

  StmtPtr init = nullptr;
  DeclPtr varDecl1 = nullptr;
  DeclPtr varDecl2 = nullptr;

  // Check for Implicit For-In Syntax: for (val in ...) or for (val, idx in ...)
  // Criteria: Identifier followed by ',' or 'in'
  bool isImplicitForIn = curTok.is(TokenKind::Identifier) &&
                         (peekIs(TokenKind::Comma) || peekIs(TokenKind::KwIn));

  if (isImplicitForIn) {
    // 1. Parse First Variable (val / key / ch)
    std::string name1 = curTok.getSpelling().str();
    SourceLocation loc1 = curTok.location;
    consume(); // eat identifier
    // Create declaration with inferred (Any) type
    varDecl1 = std::make_unique<VariableDecl>(std::make_unique<AnyType>(loc1),
                                              name1, nullptr, loc1);

    // 2. Parse Second Variable (idx / val) - Optional
    if (consumeIf(TokenKind::Comma)) {
      if (curTok.is(TokenKind::Identifier)) {
        std::string name2 = curTok.getSpelling().str();
        SourceLocation loc2 = curTok.location;
        consume(); // eat identifier
        varDecl2 = std::make_unique<VariableDecl>(
            std::make_unique<AnyType>(loc2), name2, nullptr, loc2);
      } else {
        error("Expected identifier after comma in for-loop");
      }
    }

    expect(TokenKind::KwIn);
    ExprPtr collection = parseExpression();
    expect(TokenKind::RParen);
    StmtPtr body = parseStatement();

    return std::make_unique<ForInStmt>(std::move(varDecl1), std::move(varDecl2),
                                       std::move(collection), std::move(body),
                                       loc);
  }

  // --- Standard For-Loop (Typed Decl or Expression) ---

  // [FIX] Simplified declaration detection for standard 'for (int i = 0; ...)'
  bool isTypedDeclaration =
      curTok.isAny(TokenKind::KwInt, TokenKind::KwFloat, TokenKind::KwBool,
                   TokenKind::KwString, TokenKind::KwVoid, TokenKind::KwAny);

  if (isTypedDeclaration) {
    varDecl1 = parseVariableDecl();

    // Check if this is a Typed For-In loop: for (int x in arr)
    if (varDecl1 && consumeIf(TokenKind::KwIn)) {
      ExprPtr collection = parseExpression();
      expect(TokenKind::RParen);
      StmtPtr body = parseStatement();
      // Typed loop usually only supports 1 variable in standard syntax,
      // passed as the first arg. Second is null.
      return std::make_unique<ForInStmt>(std::move(varDecl1), nullptr,
                                         std::move(collection), std::move(body),
                                         loc);
    }

    // Standard For Loop init
    if (varDecl1) {
      init = std::make_unique<DeclStmt>(std::move(varDecl1), loc);
    }
  } else {
    // Expression Initialization
    if (curTok.isNot(TokenKind::Semicolon)) {
      ExprPtr expr = parseExpression();
      if (expr) {
        init =
            std::make_unique<ExpressionStmt>(std::move(expr), curTok.location);
      }
    }
    expect(TokenKind::Semicolon);
  }

  // Condition
  ExprPtr cond = nullptr;
  if (curTok.isNot(TokenKind::Semicolon)) {
    cond = parseExpression();
  }
  expect(TokenKind::Semicolon);

  // Increment
  ExprPtr inc = nullptr;
  if (curTok.isNot(TokenKind::RParen)) {
    inc = parseExpression();
  }
  expect(TokenKind::RParen);

  StmtPtr body = parseStatement();
  return std::make_unique<ForStmt>(std::move(init), std::move(cond),
                                   std::move(inc), std::move(body), loc);
}

StmtPtr Parser::parseSwitchStmt() {
  SourceLocation loc = curTok.location;
  consume();
  expect(TokenKind::LParen);
  auto cond = parseExpression();
  expect(TokenKind::RParen);
  expect(TokenKind::LBrace);

  std::vector<SwitchCase> cases;

  while (consumeIf(TokenKind::KwCase)) {
    std::vector<ExprPtr> values;
    do {
      values.push_back(parseExpression());
    } while (consumeIf(TokenKind::Comma));
    expect(TokenKind::Colon);

    std::vector<StmtPtr> blockStmts;
    while (curTok.isNot(TokenKind::KwCase) &&
           curTok.isNot(TokenKind::KwDefault) &&
           curTok.isNot(TokenKind::RBrace)) {
      if (auto s = parseStatement())
        blockStmts.push_back(std::move(s));
      else
        synchronize();
    }
    auto block =
        std::make_unique<BlockStmt>(std::move(blockStmts), curTok.location);
    cases.emplace_back(std::move(values), std::move(block), false);
  }

  if (consumeIf(TokenKind::KwDefault)) {
    expect(TokenKind::Colon);
    std::vector<StmtPtr> blockStmts;
    while (curTok.isNot(TokenKind::RBrace)) {
      if (auto s = parseStatement())
        blockStmts.push_back(std::move(s));
      else
        synchronize();
    }
    auto block =
        std::make_unique<BlockStmt>(std::move(blockStmts), curTok.location);
    cases.emplace_back(std::vector<ExprPtr>{}, std::move(block), true);
  }

  expect(TokenKind::RBrace);
  return std::make_unique<SwitchStmt>(std::move(cond), std::move(cases), loc);
}

StmtPtr Parser::parseReturnStmt() {
  SourceLocation loc = curTok.location;
  consume();
  ExprPtr val = nullptr;
  if (curTok.isNot(TokenKind::Semicolon))
    val = parseExpression();
  consumeIf(TokenKind::Semicolon);
  return std::make_unique<ReturnStmt>(std::move(val), loc);
}

StmtPtr Parser::parseBreakStmt() {
  SourceLocation loc = curTok.location;
  consume();
  if (loopDepth == 0)
    error("break outside loop");
  consumeIf(TokenKind::Semicolon);
  return std::make_unique<BreakStmt>(loc);
}

StmtPtr Parser::parseContinueStmt() {
  SourceLocation loc = curTok.location;
  consume();
  if (loopDepth == 0)
    error("continue outside loop");
  consumeIf(TokenKind::Semicolon);
  return std::make_unique<ContinueStmt>(loc);
}

StmtPtr Parser::parseDeferStmt() {
  SourceLocation loc = curTok.location;
  consume();
  auto stmt = parseStatement();
  return std::make_unique<DeferStmt>(std::move(stmt), loc);
}

StmtPtr Parser::parseTryCatchStmt() {
  SourceLocation loc = curTok.location;
  consume();
  auto tryBlock = parseBlock();

  DeclPtr catchVar = nullptr;
  StmtPtr catchBlock = nullptr;

  if (consumeIf(TokenKind::KwCatch)) {
    expect(TokenKind::LParen);
    TypePtr type = parseType();
    if (curTok.is(TokenKind::Identifier)) {
      std::string name = curTok.getSpelling().str();
      consume();
      catchVar = std::make_unique<VariableDecl>(std::move(type), name, nullptr,
                                                curTok.location);
    }
    expect(TokenKind::RParen);
    catchBlock = parseBlock();
  }

  StmtPtr finallyBlock = nullptr;
  if (consumeIf(TokenKind::KwFinally)) {
    finallyBlock = parseBlock();
  }

  return std::make_unique<TryCatchStmt>(
      std::move(tryBlock), std::move(catchVar), std::move(catchBlock),
      std::move(finallyBlock), loc);
}

StmtPtr Parser::parseUnsafeBlock() {
  SourceLocation loc = curTok.location;
  consume();
  expect(TokenKind::LBrace);
  std::vector<StmtPtr> stmts;
  while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
    if (auto s = parseStatement())
      stmts.push_back(std::move(s));
    else
      synchronize();
  }
  expect(TokenKind::RBrace);
  return std::make_unique<UnsafeBlockStmt>(std::move(stmts), loc);
}

StmtPtr Parser::parseLockStmt() {
  consume();
  return parseBlock();
}

// --- Expressions ---

ExprPtr Parser::parseExpression() { return parseAssignment(); }

ExprPtr Parser::parseAssignment() {
  auto left = parseTernary();
  if (!left)
    return nullptr; // Safety check

  if (curTok.isAny(TokenKind::Equal, TokenKind::PlusEqual,
                   TokenKind::MinusEqual)) {
    TokenKind op = curTok.kind;
    SourceLocation opLoc = curTok.location; // Capture operator location
    consume();
    auto right = parseAssignment();
    if (!right)
      return nullptr;
    return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                        opLoc); // Use opLoc
  }
  return left;
}

ExprPtr Parser::parseTernary() {
  auto cond = parseNullCoalescing();
  if (!cond)
    return nullptr;

  if (consumeIf(TokenKind::Question)) {
    auto trueBranch = parseExpression();
    expect(TokenKind::Colon);
    auto falseBranch = parseExpression();
    return std::make_unique<TernaryExpr>(std::move(cond), std::move(trueBranch),
                                         std::move(falseBranch),
                                         cond->getLoc());
  }
  return cond;
}

ExprPtr Parser::parseNullCoalescing() {
  auto left = parseLogicalOr();
  while (consumeIf(TokenKind::QuestionQuestion)) {
    auto right = parseLogicalOr();
    left = std::make_unique<BinaryExpr>(std::move(left),
                                        TokenKind::QuestionQuestion,
                                        std::move(right), left->getLoc());
  }
  return left;
}

ExprPtr Parser::parseLogicalOr() {
  auto left = parseLogicalAnd();
  while (
      curTok.is(TokenKind::PipePipe)) { // [FIX] Check kind instead of consumeIf
    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location; // [FIX] Capture correct op location
    consume();
    auto right = parseLogicalAnd();
    left = std::make_unique<BinaryExpr>(std::move(left), TokenKind::PipePipe,
                                        std::move(right), opLoc);
  }
  return left;
}

ExprPtr Parser::parseLogicalAnd() {
  auto left = parseBitwiseOr();
  while (curTok.is(TokenKind::AmpAmp)) { // [FIX] Check kind
    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location;
    consume();
    auto right = parseBitwiseOr();
    left = std::make_unique<BinaryExpr>(std::move(left), TokenKind::AmpAmp,
                                        std::move(right), opLoc);
  }
  return left;
}

ExprPtr Parser::parseBitwiseOr() {
  auto left = parseBitwiseXor();
  // [FIX] Check kind instead of consumeIf to capture location first
  while (curTok.is(TokenKind::Pipe)) {
    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location;
    consume();
    auto right = parseBitwiseXor();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), TokenKind::Pipe,
                                        std::move(right), opLoc);
  }
  return left;
}

ExprPtr Parser::parseBitwiseXor() {
  auto left = parseBitwiseAnd();
  while (curTok.is(TokenKind::Caret)) {
    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location;
    consume();
    auto right = parseBitwiseAnd();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), TokenKind::Caret,
                                        std::move(right), opLoc);
  }
  return left;
}

ExprPtr Parser::parseBitwiseAnd() {
  auto left = parseEquality();
  while (curTok.is(TokenKind::Amp)) {
    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location;
    consume();
    auto right = parseEquality();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), TokenKind::Amp,
                                        std::move(right), opLoc);
  }
  return left;
}

ExprPtr Parser::parseEquality() {
  auto left = parseRelational();
  while (curTok.isAny(TokenKind::EqualEqual, TokenKind::NotEqual)) {
    if (!left)
      return nullptr;
    TokenKind op = curTok.kind;
    SourceLocation opLoc = curTok.location; // [FIX] Capture opLoc
    consume();
    auto right = parseRelational();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                        opLoc);
  }
  return left;
}

ExprPtr Parser::parseRelational() {
  auto left = parseShift();
  while (true) {
    TokenKind op = curTok.kind;
    // Check for <, <=, >, >= explicitly
    if (op != TokenKind::Less && op != TokenKind::LessEqual &&
        op != TokenKind::Greater && op != TokenKind::GreaterEqual) {
      break;
    }

    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location;
    consume();
    auto right = parseShift();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                        opLoc);
  }
  return left;
}

ExprPtr Parser::parseShift() {
  auto left = parseAdditive();
  while (curTok.isAny(TokenKind::LessLess, TokenKind::GreaterGreater)) {
    if (!left)
      return nullptr;
    TokenKind op = curTok.kind;
    SourceLocation opLoc = curTok.location; // [FIX] Capture opLoc
    consume();
    auto right = parseAdditive();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                        opLoc);
  }
  return left;
}

ExprPtr Parser::parseAdditive() {
  auto left = parseMultiplicative();
  while (curTok.isAny(TokenKind::Plus, TokenKind::Minus)) {
    if (!left)
      return nullptr;
    TokenKind op = curTok.kind;
    SourceLocation opLoc = curTok.location; // Capture operator location
    consume();
    auto right = parseMultiplicative();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                        opLoc);
  }
  return left;
}

ExprPtr Parser::parseMultiplicative() {
  auto left = parsePrefix();
  while (curTok.isAny(TokenKind::Star, TokenKind::Slash, TokenKind::Percent)) {
    if (!left)
      return nullptr;
    TokenKind op = curTok.kind;
    SourceLocation opLoc = curTok.location; // [FIX] Capture opLoc
    consume();
    auto right = parsePrefix();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                        opLoc);
  }
  return left;
}

ExprPtr Parser::parsePrefix() {
  if (curTok.isAny(TokenKind::Bang, TokenKind::Minus, TokenKind::PlusPlus,
                   TokenKind::MinusMinus, TokenKind::DotDotDot)) {
    TokenKind op = curTok.kind;
    SourceLocation loc = curTok.location;
    consume();
    auto operand = parsePrefix();
    return std::make_unique<UnaryExpr>(op, std::move(operand), false, loc);
  }
  return parsePostfix();
}

ExprPtr Parser::parsePostfix() {
  auto left = parsePrimary();
  while (true) {
    if (consumeIf(TokenKind::LParen)) {
      // CallExpr
      std::vector<ExprPtr> args;
      if (curTok.isNot(TokenKind::RParen)) {
        do {
          args.push_back(parseExpression());
        } while (consumeIf(TokenKind::Comma));
      }
      expect(TokenKind::RParen);
      left = std::make_unique<CallExpr>(std::move(left), std::move(args),
                                        left->getLoc());
    } else if (consumeIf(TokenKind::Dot)) {
      // MemberExpr
      std::string name = curTok.getSpelling().str();
      expect(TokenKind::Identifier);
      left = std::make_unique<MemberExpr>(std::move(left), name, false,
                                          left->getLoc());
    }
    // [NEW] Handle Postfix Increment (x++)
    else if (consumeIf(TokenKind::PlusPlus)) {
      left = std::make_unique<UnaryExpr>(TokenKind::PlusPlus, std::move(left),
                                         true, left->getLoc());
    }
    // [NEW] Handle Postfix Decrement (x--)
    else if (consumeIf(TokenKind::MinusMinus)) {
      left = std::make_unique<UnaryExpr>(TokenKind::MinusMinus, std::move(left),
                                         true, left->getLoc());
    }
    // [NEW] Array Indexing: expr[index]
    else if (consumeIf(TokenKind::LBracket)) {
      auto index = parseExpression();
      expect(TokenKind::RBracket);
      left = std::make_unique<IndexExpr>(std::move(left), std::move(index),
                                         left->getLoc());
    } else {
      break;
    }
  }
  return left;
}

ExprPtr Parser::parsePrimary() {
  SourceLocation loc = curTok.location;
  switch (curTok.kind) {
  case TokenKind::KwNew:
    return parseNewExpr();
  case TokenKind::IntegerLiteral: {
    std::string text = curTok.getSpelling().str();
    uint64_t val = 0;
    try {
      val = std::stoull(text, nullptr, 0);
    } catch (...) {
      error("Invalid integer literal (out of range)"); // [ADDED]
    }
    NumericSuffix suffix = curTok.suffix;
    consume();
    return std::make_unique<IntegerLiteral>(val, suffix, loc);
  }
  case TokenKind::FloatLiteral: {
    std::string text = curTok.getSpelling().str();
    double val = 0.0;
    try {
      val = std::stod(text);
    } catch (...) {
      error("Invalid float literal"); // [ADDED]
    }
    NumericSuffix suffix = curTok.suffix;
    consume();
    return std::make_unique<FloatLiteral>(val, suffix, loc);
  }
  case TokenKind::CharLiteral: {
    char val = 0;
    llvm::StringRef text = curTok.getSpelling();
    if (text.size() >= 3) {
      if (text[1] == '\\' && text.size() >= 4) {
        switch (text[2]) {
        case 'n':
          val = '\n';
          break;
        case 't':
          val = '\t';
          break;
        case 'r':
          val = '\r';
          break;
        case '0':
          val = '\0';
          break;
        case '\\':
          val = '\\';
          break;
        case '\'':
          val = '\'';
          break;
        case 'x':
        case 'u':
        case 'U': { // Handle Hex and Unicode
          // Determine length: \xHH (4), \uAAAA (6), \UAAAAAAAA (10)
          size_t hexLen = (text[2] == 'x') ? 2 : (text[2] == 'u') ? 4 : 8;
          size_t prefixLen = (text[2] == 'x')   ? 3
                             : (text[2] == 'u') ? 3
                                                : 3; // text is ' \ x ... '
          // Actually index in text: ' is 0, \ is 1, x is 2. Start hex at 3.

          if (text.size() >= 3 + hexLen) {
            unsigned long long intVal = 0;
            // Simple hex parser loop
            for (size_t i = 0; i < hexLen; i++) {
              char c = text[3 + i];
              int v = (c >= '0' && c <= '9')   ? c - '0'
                      : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                      : (c >= 'A' && c <= 'F') ? c - 'A' + 10
                                               : 0;
              intVal = (intVal << 4) | v;
            }

            if (intVal > 255) {
              error("Character literal too large for 'char' type");
            }
            val = static_cast<char>(intVal);
          } else {
            error("Invalid escape sequence length");
          }
          break;
        }
        default:
          error("Invalid escape sequence in char literal");
          val = text[2];
          break;
        }
      } else {
        val = text[1];
      }
    }
    consume();
    return std::make_unique<CharLiteral>(val, loc);
  }
  case TokenKind::KwNull: {
    consume();
    return std::make_unique<NullLiteral>(loc);
  }
  case TokenKind::KwTrue: {
    consume();
    return std::make_unique<BoolLiteral>(true, loc);
  }
  case TokenKind::KwFalse: {
    consume();
    return std::make_unique<BoolLiteral>(false, loc);
  }
  case TokenKind::Identifier: {
    std::string name = curTok.getSpelling().str();
    consume();
    return std::make_unique<IdentifierExpr>(name, loc);
  }
  case TokenKind::LParen: {
    consume(); // Eat '('

    // 1. Empty Lambda: () =>
    if (curTok.is(TokenKind::RParen)) {
      consume(); // Eat ')'
      if (consumeIf(TokenKind::FatArrow))
        return parseLambdaBody({});
      error("Expected expression after empty parentheses");
      return nullptr;
    }

    // 2. Typed Lambda: (int x, ...)
    if (curTok.isAny(TokenKind::KwInt, TokenKind::KwFloat, TokenKind::KwBool,
                     TokenKind::KwString, TokenKind::KwVoid,
                     TokenKind::KwAny)) {
      std::vector<LambdaParam> params;
      do {
        TypePtr t = parseType();
        if (!t)
          break;
        std::string n;
        if (curTok.is(TokenKind::Identifier)) {
          n = curTok.getSpelling().str();
          consume();
        } else
          error("Expected parameter name");
        params.emplace_back(std::move(t), std::move(n));
      } while (consumeIf(TokenKind::Comma));
      expect(TokenKind::RParen);
      if (consumeIf(TokenKind::FatArrow))
        return parseLambdaBody(std::move(params));
      error("Expected '=>' after typed lambda parameters");
      return nullptr;
    }

    // 3. Untyped Lambda Candidate: (x) or (x, y)
    if (curTok.is(TokenKind::Identifier)) {
      if (peekIs(TokenKind::Comma)) {
        // (x, y ...) -> Definitely Lambda
        std::vector<LambdaParam> params;
        do {
          if (curTok.is(TokenKind::Identifier)) {
            params.emplace_back(nullptr, curTok.getSpelling().str());
            consume();
          } else
            error("Expected identifier");
        } while (consumeIf(TokenKind::Comma));
        expect(TokenKind::RParen);
        expect(TokenKind::FatArrow);
        return parseLambdaBody(std::move(params));
      } else if (peekIs(TokenKind::RParen)) {
        // (x) -> Lookahead for '=>'
        SourceLocation idLoc = curTok.location;
        std::string id = curTok.getSpelling().str();
        consume(); // Eat 'x', curTok is now ')'

        if (peekIs(TokenKind::FatArrow)) {
          // It's (x) => ...
          consume(); // Eat ')'
          consume(); // Eat '=>'
          std::vector<LambdaParam> params;
          params.emplace_back(nullptr, id);
          return parseLambdaBody(std::move(params));
        } else {
          expect(TokenKind::RParen);
          return std::make_unique<IdentifierExpr>(id, idLoc);
        }
      }
    }

    // 4. Fallback: Grouping Expression
    auto expr = parseExpression();
    expect(TokenKind::RParen);
    return expr;
  }
  case TokenKind::StringLiteral:
    return parseStringLiteral();
  case TokenKind::LBracket: {
    return parseArrayLiteral();
  }
  case TokenKind::TemplateString:
  case TokenKind::StringFragment:
  case TokenKind::InterpolationStart:
    return parseTemplateString();

  default:
    error("Expected expression");
    return nullptr;
  }
}

ExprPtr Parser::parseArrayLiteral() {
  SourceLocation loc = curTok.location;
  consume(); // Eat '['
  std::vector<ExprPtr> elements;

  if (curTok.isNot(TokenKind::RBracket)) {
    do {
      if (auto e = parseExpression()) {
        elements.push_back(std::move(e));
      }
    } while (consumeIf(TokenKind::Comma));
  }
  expect(TokenKind::RBracket);
  return std::make_unique<ArrayLiteral>(std::move(elements), loc);
}

ExprPtr Parser::parseTemplateString() {
  SourceLocation loc = curTok.location;
  std::vector<ExprPtr> parts;

  // Case 1: Simple template string without interpolations `hello`
  if (curTok.is(TokenKind::TemplateString)) {
    std::string val = curTok.getSpelling().str();
    if (val.size() >= 2)
      val = val.substr(1, val.size() - 2); // Remove backticks
    consume();
    return std::make_unique<StringLiteral>(std::move(val), true, loc);
  }

  // Complex case loop
  while (true) { // [FIX] Removed arbitrary 'guard' counter
    if (curTok.is(TokenKind::StringFragment)) {
      std::string val = curTok.getSpelling().str();
      parts.push_back(std::make_unique<StringLiteral>(std::move(val), true,
                                                      curTok.location));
      consume();
    } else if (consumeIf(TokenKind::InterpolationStart)) {
      auto expr = parseExpression();
      if (expr)
        parts.push_back(std::move(expr));

      if (!expect(TokenKind::InterpolationEnd))
        break;
    } else if (curTok.is(TokenKind::TemplateString)) {
      std::string val = curTok.getSpelling().str();
      if (!val.empty() && val.back() == '`')
        val.pop_back();
      parts.push_back(std::make_unique<StringLiteral>(std::move(val), true,
                                                      curTok.location));
      consume();
      break;
    } else {
      error("Malformed template string"); // [FIX] Breaks loop on error
      break;
    }
  }
  return std::make_unique<TemplateStringExpr>(std::move(parts), loc);
}

ExprPtr Parser::parseStringLiteral() {
  SourceLocation loc = curTok.location;
  std::string val = curTok.getSpelling().str();

  // Remove surrounding quotes (" or `) if present
  if (val.size() >= 2) {
    char quote = val.front();
    if (quote == '"' || quote == '`' || quote == '\'') {
      val = val.substr(1, val.size() - 2);
    }
  }

  consume();
  return std::make_unique<StringLiteral>(std::move(val), false, loc);
}

ExprPtr Parser::parseLambdaBody(std::vector<LambdaParam> params) {
  SourceLocation loc = curTok.location;
  StmtPtr body = nullptr;
  bool isExprBody = false;

  if (curTok.is(TokenKind::LBrace)) {
    // Block Body: (...) => { ... }
    body = parseBlock();
    isExprBody = false;
  } else {
    // Expression Body: (...) => expr
    ExprPtr expr = parseExpression();
    // Wrap implicit return in a ReturnStmt for the AST
    body = std::make_unique<ReturnStmt>(std::move(expr),
                                        expr ? expr->getLoc() : loc);
    isExprBody = true;
  }

  return std::make_unique<LambdaExpr>(std::move(params), std::move(body),
                                      isExprBody, loc);
}

ExprPtr Parser::parseNewExpr() {
  SourceLocation loc = curTok.location;
  consume(); // Eat 'new'

  bool isWeak = false;
  if (consumeIf(TokenKind::KwWeak)) {
    isWeak = true;
  }

  // --- Case 1: Thread Creation ---
  // Syntax: new [weak] Thread() => { ... }
  if (curTok.is(TokenKind::KwThread)) {
    consume(); // Eat 'Thread'

    // Optional: Parse arguments ()
    if (consumeIf(TokenKind::LParen)) {
      expect(TokenKind::RParen);
    }

    // Expect Fat Arrow '=>'
    if (!expect(TokenKind::FatArrow)) {
      return nullptr;
    }

    // Parse Lambda Body
    ExprPtr bodyExpr = parseLambdaBody({});
    if (auto lambda = llvm::dyn_cast<LambdaExpr>(bodyExpr.get())) {
      bodyExpr.release(); // Release ownership from ExprPtr
      std::unique_ptr<LambdaExpr> typedBody(lambda);
      return std::make_unique<ThreadExpr>(isWeak, std::move(typedBody), loc);
    } else {
      error("Expected lambda body for Thread");
      return nullptr;
    }
  }

  // --- Case 2: Standard Object Creation ---
  if (isWeak) {
    error("'weak' can only be used with 'Thread'");
  }

  // Parse the type being instantiated
  TypePtr type = parseType();
  if (!type)
    return nullptr;

  expect(TokenKind::LParen);
  std::vector<ExprPtr> args;
  if (curTok.isNot(TokenKind::RParen)) {
    do {
      if (auto arg = parseExpression()) {
        args.push_back(std::move(arg));
      }
    } while (consumeIf(TokenKind::Comma));
  }
  expect(TokenKind::RParen);

  return std::make_unique<NewExpr>(std::move(type), std::move(args), loc);
}

TypePtr Parser::parseType() {
  SourceLocation loc = curTok.location;
  TypePtr type = nullptr;

  switch (curTok.kind) {
    // --- Primitives ---
  case TokenKind::KwInt:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc);
    consume();
    break;
  case TokenKind::KwLong: // [NEW] Handles 'long' -> i64
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I64, loc);
    consume();
    break;
  case TokenKind::KwShort: // [NEW] Handles 'short' -> i16
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I16, loc);
    consume();
    break;
  case TokenKind::KwFloat:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F32, loc);
    consume();
    break;
  case TokenKind::KwDouble: // [NEW] Handles 'double' -> f64
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F64, loc);
    consume();
    break;
  case TokenKind::KwISize: // [NEW] Handles 'isize'
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::ISize, loc);
    consume();
    break;
  case TokenKind::KwUSize: // [NEW] Handles 'usize'
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::USize, loc);
    consume();
    break;
  case TokenKind::KwBool:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Bool, loc);
    consume();
    break;
  case TokenKind::KwString:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String, loc);
    consume();
    break;
  case TokenKind::KwChar:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Char, loc);
    consume();
    break;
  case TokenKind::KwVoid:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc);
    consume();
    break;
  case TokenKind::KwAny:
    type = std::make_unique<AnyType>(loc);
    consume();
    break;
  case TokenKind::KwHalf:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F16, loc);
    consume();
    break;
  case TokenKind::KwQuarter:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F8, loc);
    consume();
    break;

  // --- Modifiers ---
  case TokenKind::KwLock: {
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<LockType>(std::move(inner), loc);
    break; // [FIX] Added missing break
  }
  case TokenKind::KwView: {
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<ViewType>(std::move(inner), loc);
    break; // [FIX] Added missing break
  }
  case TokenKind::KwMut: {
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<MutType>(std::move(inner), loc);
    break; // [FIX] Added missing break
  }

  // --- Special Types ---
  case TokenKind::KwThread:
    type = std::make_unique<NamedType>(
        "Thread", std::vector<NamedType::GenericArg>{}, loc);
    consume();
    break;

  // --- Reference Type (ref T) ---
  case TokenKind::KwRef: {
    consume();
    type = std::make_unique<ReferenceType>(parseType(), loc);
    break;
  }

  // --- Map Type (table<K, V>) ---
  case TokenKind::KwTable: {
    consume();
    if (consumeIf(TokenKind::Less)) {
      TypePtr key = parseType();
      expect(TokenKind::Comma);
      TypePtr val = parseType();
      expect(TokenKind::Greater);
      type = std::make_unique<MapType>(std::move(key), std::move(val), loc);
    } else {
      error("Expected '<' after table");
    }
    break;
  }

  // --- Named Types & Generics ---
  case TokenKind::Identifier: {
    std::string name = curTok.getSpelling().str();
    consume();
    std::vector<NamedType::GenericArg> args;
    if (consumeIf(TokenKind::Less)) {
      do {
        TypePtr t = parseType();
        if (t) {
          args.push_back({std::move(t), Variance::Invariant});
        } else {
          error("Expected type argument");
          while (curTok.isNot(TokenKind::Comma) &&
                 curTok.isNot(TokenKind::Greater) &&
                 curTok.isNot(TokenKind::Eof)) {
            advance();
          }
        }
      } while (consumeIf(TokenKind::Comma));
      expect(TokenKind::Greater);
    }
    type = std::make_unique<NamedType>(name, std::move(args), loc);
    break;
  }

  // --- Function Type: (Args) => Ret ---
  case TokenKind::LParen: {
    consume();
    std::vector<TypePtr> params;
    if (curTok.isNot(TokenKind::RParen)) {
      do {
        if (auto t = parseType())
          params.push_back(std::move(t));
      } while (consumeIf(TokenKind::Comma));
    }
    expect(TokenKind::RParen);
    if (consumeIf(TokenKind::FatArrow)) {
      TypePtr ret = parseType();
      type = std::make_unique<FunctionType>(std::move(ret), std::move(params),
                                            loc);
    } else {
      error("Expected '=>' for function type");
      return nullptr;
    }
    break;
  }

  default:
    return nullptr;
  }

  // --- Suffixes (Ptr*, Array[], Nullable?) ---
  while (true) {
    if (consumeIf(TokenKind::Star)) {
      type = std::make_unique<PointerType>(std::move(type), loc);
    } else if (consumeIf(TokenKind::LBracket)) {
      ExprPtr sizeExpr = nullptr;
      if (curTok.isNot(TokenKind::RBracket)) {
        sizeExpr = parseExpression();
      }
      expect(TokenKind::RBracket);
      type = std::make_unique<ArrayType>(std::move(type), std::move(sizeExpr),
                                         loc);
    } else if (consumeIf(TokenKind::Question)) { // [FIX] Added Nullable support
      type = std::make_unique<NullableType>(std::move(type), loc);
    } else {
      break;
    }
  }
  return type;
}

} // namespace moksha
