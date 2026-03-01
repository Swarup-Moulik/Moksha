#include "moksha/Parser/Parser.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Support/Diagnostics.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

// --- Parser Implementation ---

static std::string tokenKindToString(TokenKind kind);

Parser::Parser(Lexer &lexer, ASTContext &context, llvm::SourceMgr &srcMgr,
               DiagnosticEngine &diags)
    : lexer(lexer), context(context), srcMgr(srcMgr), Diags(diags) {
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

// Helper for better error messages
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
  case TokenKind::KwUnsigned:
    return "'unsigned'";
  case TokenKind::KwConstructor:
    return "'constructor'";
  case TokenKind::KwDestructor:
    return "'destructor'";
  case TokenKind::KwThis:
    return "'this'";
  case TokenKind::KwSuper:
    return "'super'";
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
  case TokenKind::KwAs:
    return "'as'";
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
  case TokenKind::KwNoreturn:
    return "'noreturn'";
  case TokenKind::KwLock:
    return "'lock'";
  case TokenKind::KwView:
    return "'view'";
  case TokenKind::KwMut:
    return "'mut'";
  case TokenKind::KwExtern:
    return "'extern'";
  case TokenKind::KwUsing:
    return "'using'";
  case TokenKind::KwAsm:
    return "'asm'";
  case TokenKind::KwSection:
    return "'section'";
  case TokenKind::KwUsed:
    return "'used'";
  case TokenKind::KwThreadLocal:
    return "'thread_local'";

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
  case TokenKind::PipeGreater:
    return "'|>'";
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
    return "'?"
           "?"
           "'";
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

static bool hasNewlineBeforeToken(const Token &tok) {
  if (tok.getSpelling().empty())
    return false;

  // Safely scan backwards through whitespace in the source buffer
  const char *ptr = tok.getSpelling().data() - 1;
  while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') {
    if (*ptr == '\n' || *ptr == '\r')
      return true;
    ptr--;
  }
  return false;
}

// Safely parse { a, b } without hanging
void Parser::parseImportSymbolList(std::vector<std::string> &symbols) {
  while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {

    // Allow Identifiers AND type keywords like 'table' and 'list'
    if (curTok.is(TokenKind::Identifier) || curTok.is(TokenKind::KwTable)) {
      symbols.push_back(curTok.getSpelling().str());
      consume();
    } else {
      error("Expected identifier or type in import list");
      consume(); // [CRITICAL] Force advance to break infinite loops!
    }

    if (curTok.isNot(TokenKind::RBrace)) {
      if (!consumeIf(TokenKind::Comma)) {
        error("Expected ',' between imported symbols");
        consume(); // Force advance on syntax error
      }
    }
  }
  expect(TokenKind::RBrace);
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

bool Parser::expectGreater() {
  if (curTok.is(TokenKind::Greater)) {
    advance();
    return true;
  }

  //  The Right-Angle-Bracket Hack
  if (curTok.is(TokenKind::GreaterGreater)) {
    curTok.kind = TokenKind::Greater;
    return true; // Notice we DO NOT call advance() here!
  }

  // Handle '>>=' edge cases just in case
  if (curTok.is(TokenKind::GreaterGreaterEqual)) {
    curTok.kind = TokenKind::GreaterEqual;
    return true;
  }

  // Fallback to standard error reporting
  return expect(TokenKind::Greater);
}

void Parser::error(const std::string &message) {
  Diags.report(curTok.location, DiagID::err_expected_token) << message;
}

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
    case TokenKind::RBrace:
      return;
    default:
      advance();
    }
  }
}

bool Parser::isStartOfDeclaration() {
  // 1. keywords that always start a declaration
  if (curTok.isAny(
          TokenKind::KwImport, TokenKind::KwFrom, TokenKind::KwGeneric,
          TokenKind::KwEnum, TokenKind::KwClass, TokenKind::KwStruct,
          TokenKind::KwUnion, TokenKind::KwRef, TokenKind::KwThread,
          TokenKind::KwTable, TokenKind::KwUnsigned, TokenKind::KwWeak,
          TokenKind::KwAsync, TokenKind::KwShared, TokenKind::KwMacro,
          TokenKind::KwConst, TokenKind::KwStatic, TokenKind::KwPublic,
          TokenKind::KwPrivate, TokenKind::KwProtected, TokenKind::KwExtern,
          TokenKind::KwUsing, TokenKind::KwPacked, TokenKind::KwAlign,
          TokenKind::KwSection, TokenKind::KwInterrupt, TokenKind::KwNaked,
          TokenKind::KwNoreturn, TokenKind::KwNoinline, TokenKind::KwVolatile,
          TokenKind::KwLock, TokenKind::KwView, TokenKind::KwMut,
          TokenKind::KwUsed, TokenKind::KwThreadLocal)) {
    if (curTok.is(TokenKind::KwLock) &&
        (nextTok.is(TokenKind::LBrace) || nextTok.is(TokenKind::LParen))) {
      return false;
    }
    if (curTok.isAny(TokenKind::KwView, TokenKind::KwMut) &&
        nextTok.is(TokenKind::LBrace)) {
      return false;
    }
    return true;
  }

  // 2. Primitive types start declarations (int x, void foo)
  if (curTok.isAny(TokenKind::KwInt, TokenKind::KwFloat, TokenKind::KwBool,
                   TokenKind::KwString, TokenKind::KwVoid, TokenKind::KwAny,
                   TokenKind::KwChar, TokenKind::KwISize, TokenKind::KwUSize,
                   TokenKind::KwShort, TokenKind::KwLong, TokenKind::KwDouble,
                   TokenKind::KwHalf, TokenKind::KwQuarter,
                   TokenKind::KwUnsigned)) {
    if (nextTok.is(TokenKind::LParen)) {
      return false;
    }
    return true;
  }

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
    // "Type* Name" or "Type** Name" -> Declaration
    if (nextTok.is(TokenKind::Star) || nextTok.is(TokenKind::Power))
      return true;
    // "Type? Name" -> Declaration
    if (nextTok.is(TokenKind::Question))
      return true;
  }

  // 4. Prefix Pointers & References for Declarations (e.g., *mut int p = &x,
  // &mut int r = x)
  if (curTok.isAny(TokenKind::Star, TokenKind::Power, TokenKind::Amp)) {
    if (nextTok.isAny(TokenKind::KwMut, TokenKind::KwConst, TokenKind::KwLock,
                      TokenKind::KwView, TokenKind::KwInt, TokenKind::KwFloat,
                      TokenKind::KwDouble, TokenKind::KwBool,
                      TokenKind::KwString, TokenKind::KwChar,
                      TokenKind::KwShort, TokenKind::KwLong, TokenKind::KwISize,
                      TokenKind::KwUSize, TokenKind::KwHalf) ||
        nextTok.isAny(TokenKind::KwQuarter, TokenKind::KwVoid, TokenKind::KwAny,
                      TokenKind::KwUnsigned, TokenKind::KwVolatile,
                      TokenKind::Star, TokenKind::Power, TokenKind::Amp)) {
      return true;
    }
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
        if (decl->getKind() == StmtKind::VariableDecl) {
          decls.push_back(std::move(decl));
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
          "main", std::vector<FunctionDecl::Param>{}, std::move(voidType),
          std::move(body), false, false, false, false, Visibility::Default,
          startLoc);

      decls.push_back(std::move(mainFunc));
    }
  }

  return std::make_unique<ModuleDecl>("main", std::move(decls), startLoc);
}

DeclPtr Parser::parseTopLevelDecl() {
  // 1. Parse Modifiers
  Visibility vis = Visibility::Default;
  bool isStatic = false;
  bool isConst = false;
  bool isAsync = false;
  bool isShared = false;
  bool isWeak = false;
  bool isExtern = false, isInterrupt = false, isNaked = false;
  bool isPacked = false, isVolatile = false;
  bool isNoReturn = false;
  bool isNoInline = false;
  bool isUsed = false;
  bool isThreadLocal = false;
  int alignment = 0;
  std::string externLinkage = "", sectionName = "";

  while (true) {
    if (consumeIf(TokenKind::KwPublic))
      vis = Visibility::Public;
    else if (consumeIf(TokenKind::KwPrivate))
      vis = Visibility::Private;
    else if (consumeIf(TokenKind::KwProtected))
      vis = Visibility::Protected;
    else if (consumeIf(TokenKind::KwStatic))
      isStatic = true;
    else if (consumeIf(TokenKind::KwAsync))
      isAsync = true;
    else if (consumeIf(TokenKind::KwShared))
      isShared = true;
    else if (consumeIf(TokenKind::KwWeak))
      isWeak = true;
    else if (consumeIf(TokenKind::KwExtern)) {
      isExtern = true;
      if (curTok.is(TokenKind::StringLiteral)) {
        externLinkage = curTok.getSpelling().str();
        if (externLinkage.size() >= 2)
          externLinkage = externLinkage.substr(1, externLinkage.size() - 2);
        consume();
      }
    } else if (consumeIf(TokenKind::KwInterrupt))
      isInterrupt = true;
    else if (consumeIf(TokenKind::KwNaked))
      isNaked = true;
    else if (consumeIf(TokenKind::KwNoreturn))
      isNoReturn = true;
    else if (consumeIf(TokenKind::KwNoinline))
      isNoInline = true;
    else if (consumeIf(TokenKind::KwPacked))
      isPacked = true;
    else if (consumeIf(TokenKind::KwAlign)) {
      expect(TokenKind::LParen);
      if (curTok.is(TokenKind::IntegerLiteral)) {
        alignment = std::stoull(curTok.getSpelling().str());
        consume();
      } else {
        error("Expected integer alignment");
      }
      expect(TokenKind::RParen);
    } else if (consumeIf(TokenKind::KwSection)) {
      expect(TokenKind::LParen);
      if (curTok.is(TokenKind::StringLiteral)) {
        sectionName = curTok.getSpelling().str();
        if (sectionName.size() >= 2)
          sectionName = sectionName.substr(1, sectionName.size() - 2);
        consume();
      } else {
        error("Expected section name");
      }
      expect(TokenKind::RParen);
    } else if (consumeIf(TokenKind::KwUsed)) {
      isUsed = true;
    } else if (consumeIf(TokenKind::KwThreadLocal)) {
      isThreadLocal = true;
    } else
      break;
  }

  if (consumeIf(TokenKind::KwUsing)) {
    SourceLocation loc = curTok.location;
    std::string name = curTok.getSpelling().str();
    expect(TokenKind::Identifier);
    expect(TokenKind::Equal);
    TypePtr target = parseType();
    consumeIf(TokenKind::Semicolon);
    return std::make_unique<UsingDecl>(std::move(name), std::move(target), loc);
  }

  if (curTok.is(TokenKind::KwImport) || curTok.is(TokenKind::KwFrom))
    return parseImportDecl();
  if (curTok.is(TokenKind::KwMacro))
    return parseMacroDecl();
  if (curTok.is(TokenKind::KwGeneric))
    return parseGenericDecl();
  if (curTok.is(TokenKind::KwEnum))
    return parseEnumDecl();
  if (curTok.isAny(TokenKind::KwClass, TokenKind::KwStruct, TokenKind::KwRef,
                   TokenKind::KwUnion)) {
    auto decl = parseClassDecl();
    if (auto cls = dynamic_cast<ClassDecl *>(decl.get())) {
      cls->setPacked(isPacked);
      cls->setAlignment(alignment);
      cls->setSection(sectionName);
    }
    return decl;
  }

  // Parse Type (for Variables and Functions)
  TypePtr type = parseType();
  if (!type) {
    // If we parsed modifiers but found no type, it's an error
    if (vis != Visibility::Default || isStatic || isConst || isAsync) {
      error("Expected declaration after modifiers");
    }
    return nullptr;
  }

  bool isConstVar = type->is<ConstType>();
  bool isVolatileVar = type->is<VolatileType>();

  std::string name = curTok.getSpelling().str();
  expect(TokenKind::Identifier);

  DeclPtr decl;
  if (curTok.is(TokenKind::LParen)) {
    decl = parseFunctionRest(std::move(type), name, isAsync, isStatic, isWeak,
                             vis);
    if (auto fn = dynamic_cast<FunctionDecl *>(decl.get())) {
      fn->setExtern(isExtern);
      fn->setExternLinkage(externLinkage);
      fn->setInterrupt(isInterrupt);
      fn->setNaked(isNaked);
      fn->setNoReturn(isNoReturn);
      fn->setNoInline(isNoInline);
      fn->setUsed(isUsed);
    }
  } else {
    decl = parseVariableRest(std::move(type), name, isConstVar, isStatic,
                             isShared, vis);
    if (auto var = dynamic_cast<VariableDecl *>(decl.get())) {
      var->setVolatile(isVolatileVar);
      var->setExtern(isExtern);
      var->setAlignment(alignment);
      var->setSection(sectionName);
      var->setUsed(isUsed);
      var->setThreadLocal(isThreadLocal);
    }
  }
  return decl;
}

DeclPtr Parser::parseFunctionRest(TypePtr returnType, std::string name,
                                  bool isAsync, bool isStatic, bool isWeak,
                                  Visibility vis) {
  // [FIX] Capture location from the type or current token
  SourceLocation loc = returnType->getLoc();

  std::vector<FunctionDecl::Param> params;
  bool isVariadic = false;

  expect(TokenKind::LParen);
  if (curTok.isNot(TokenKind::RParen)) {
    do {
      // [FIX] Handle Variadic Argument
      if (consumeIf(TokenKind::DotDotDot)) {
        isVariadic = true;
        break;
      }

      TypePtr paramType = parseType();
      std::string paramName = "";
      SourceLocation paramLoc = curTok.location;

      if (curTok.is(TokenKind::Identifier)) {
        paramName = curTok.getSpelling().str();
        consume();
      }

      // [FIX] Explicit construction for vector push_back
      params.push_back(
          FunctionDecl::Param{paramName, std::move(paramType), paramLoc});

    } while (consumeIf(TokenKind::Comma));
  }
  expect(TokenKind::RParen);

  StmtPtr body = nullptr;
  if (consumeIf(TokenKind::Semicolon)) {
    // Bodiless function (e.g. extern printf);
  } else {
    body = parseBlock();
  }

  // [FIX] Pass all new flags (isVariadic)
  return std::make_unique<FunctionDecl>(
      name, std::move(params), std::move(returnType), std::move(body), isAsync,
      isStatic, isVariadic, isWeak, vis, loc);
}

DeclPtr Parser::parseVariableRest(TypePtr type, std::string name, bool isConst,
                                  bool isStatic, bool isShared,
                                  Visibility vis) {
  ExprPtr init = nullptr;
  int bitFieldWidth = -1;
  if (consumeIf(TokenKind::Colon)) {
    if (curTok.is(TokenKind::IntegerLiteral)) {
      bitFieldWidth = std::stoi(curTok.getSpelling().str());
      consume();
    } else {
      error("Expected integer literal for bitfield width");
    }
  } else if (consumeIf(TokenKind::Equal)) {
    init = parseExpression();
    if (!init)
      return nullptr;
  }
  consumeIf(TokenKind::Semicolon);
  auto varDecl = std::make_unique<VariableDecl>(
      std::move(type), name, std::move(init), isConst, isStatic, isShared, vis,
      type->getLoc());
  varDecl->setBitWidth(bitFieldWidth);
  return varDecl;
}

DeclPtr Parser::parseVariableDecl() {
  // 1. Initialize all possible modifiers
  Visibility vis = Visibility::Default;
  bool isStatic = false;
  bool isConst = false;
  bool isShared = false;
  bool isExtern = false;
  bool isVolatile = false;
  bool isUsed = false;
  bool isThreadLocal = false;
  int alignment = 0;
  std::string sectionName = "";

  // 2. Parse modifiers in any order
  while (true) {
    if (consumeIf(TokenKind::KwPublic))
      vis = Visibility::Public;
    else if (consumeIf(TokenKind::KwPrivate))
      vis = Visibility::Private;
    else if (consumeIf(TokenKind::KwProtected))
      vis = Visibility::Protected;
    else if (consumeIf(TokenKind::KwStatic))
      isStatic = true;
    else if (consumeIf(TokenKind::KwShared))
      isShared = true;
    else if (consumeIf(TokenKind::KwExtern)) {
      isExtern = true;
      if (curTok.is(TokenKind::StringLiteral))
        consume();
    } else if (consumeIf(TokenKind::KwUsed))
      isUsed = true;
    else if (consumeIf(TokenKind::KwThreadLocal))
      isThreadLocal = true;
    else if (consumeIf(TokenKind::KwAlign)) {
      expect(TokenKind::LParen);
      if (curTok.is(TokenKind::IntegerLiteral)) {
        alignment = std::stoull(curTok.getSpelling().str());
        consume();
      } else {
        error("Expected integer alignment");
      }
      expect(TokenKind::RParen);
    } else if (consumeIf(TokenKind::KwSection)) {
      expect(TokenKind::LParen);
      if (curTok.is(TokenKind::StringLiteral)) {
        sectionName = curTok.getSpelling().str();
        if (sectionName.size() >= 2)
          sectionName = sectionName.substr(1, sectionName.size() - 2);
        consume();
      } else {
        error("Expected section name");
      }
      expect(TokenKind::RParen);
    } else {
      // No more modifiers, break out of the loop
      break;
    }
  }

  // 3. Parse Type
  TypePtr type = parseType();
  if (!type) {
    if (vis != Visibility::Default || isStatic || isConst || isVolatile ||
        isThreadLocal) {
      error("Expected type after modifiers");
    }
    return nullptr;
  }

  bool isConstVar = type->is<ConstType>();
  bool isVolatileVar = type->is<VolatileType>();

  // 4. Parse Name
  std::string name = curTok.getSpelling().str();
  expect(TokenKind::Identifier);

  // 5. Construct the base AST Node
  auto decl = parseVariableRest(std::move(type), name, isConstVar, isStatic,
                                isShared, vis);

  // 6. Apply all the additional parsed properties to the VariableDecl
  if (auto var = dynamic_cast<VariableDecl *>(decl.get())) {
    var->setVolatile(isVolatileVar);
    var->setExtern(isExtern);
    var->setAlignment(alignment);
    var->setSection(sectionName);
    var->setUsed(isUsed);
    var->setThreadLocal(isThreadLocal);
  }

  return decl;
}

StmtPtr Parser::parseVariableStmt() {
  SourceLocation loc = curTok.location;
  DeclPtr decl = parseVariableDecl();
  if (!decl)
    return nullptr;
  return std::make_unique<DeclStmt>(std::move(decl), loc);
}

DeclPtr Parser::parseImportDecl() {
  SourceLocation loc = curTok.location;
  std::string moduleName;
  std::vector<std::string> symbols;

  // Helper to strip quotes
  auto stripQuotes = [](std::string s) {
    if (s.size() >= 2 &&
        (s.front() == '"' || s.front() == '`' || s.front() == '\''))
      return s.substr(1, s.size() - 2);
    return s;
  };

  // Pattern 1: from "std/collections" import { table, list }
  if (consumeIf(TokenKind::KwFrom)) {
    if (curTok.is(TokenKind::StringLiteral)) {
      moduleName = stripQuotes(curTok.getSpelling().str());
      consume();
    }
    expect(TokenKind::KwImport);
    expect(TokenKind::LBrace);
    parseImportSymbolList(symbols);
  } else if (consumeIf(TokenKind::KwImport)) {
    // Pattern 2: import { table, list } from "std/collections"
    if (curTok.is(TokenKind::LBrace)) {
      consume(); // Eat '{'
      parseImportSymbolList(symbols);
      expect(TokenKind::KwFrom);
      if (curTok.is(TokenKind::StringLiteral)) {
        moduleName = stripQuotes(curTok.getSpelling().str());
        consume();
      }
    }
    // Pattern 3: import "std/io" (Whole Module)
    else if (curTok.is(TokenKind::StringLiteral)) {
      moduleName = stripQuotes(curTok.getSpelling().str());
      consume();
    } else {
      error("Expected '{' or string literal after 'import'");
    }
  }

  consumeIf(TokenKind::Semicolon);
  return std::make_unique<ImportDecl>(moduleName, symbols, loc);
}

DeclPtr Parser::parseGenericDecl() {
  SourceLocation loc = curTok.location;
  expect(TokenKind::KwGeneric);

  std::string name = "";
  std::vector<std::string> typeParams;

  // Check for Syntax: generic Box<T>
  if (curTok.is(TokenKind::Identifier)) {
    name = curTok.getSpelling().str();
    consume();
  }

  // Parse Type Parameters: <T, U>
  if (consumeIf(TokenKind::Less)) {
    do {
      if (curTok.is(TokenKind::Identifier)) {
        typeParams.push_back(curTok.getSpelling().str());
        consume();
      } else {
        error("Expected type parameter name");
      }
    } while (consumeIf(TokenKind::Comma));
    expectGreater();
  }

  // If a name was provided and we see '{' or '(', it's a generic class/struct
  // definition
  if (!name.empty() &&
      (curTok.is(TokenKind::LBrace) || curTok.is(TokenKind::LParen))) {

    std::vector<std::string> parentNames;
    if (consumeIf(TokenKind::LParen)) {
      do {
        if (curTok.is(TokenKind::Identifier)) {
          parentNames.push_back(curTok.getSpelling().str());
          consume();
        } else {
          error("Expected parent class name");
        }
      } while (consumeIf(TokenKind::Comma));

      expect(TokenKind::RParen);
    }

    expect(TokenKind::LBrace);
    std::vector<DeclPtr> members;
    while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
      // 1. Parse Member Modifiers (vis, static, etc.)
      Visibility memVis = Visibility::Default;
      bool isStatic = false, isAsync = false, isShared = false, isWeak = false;
      while (true) {
        if (consumeIf(TokenKind::KwPublic))
          memVis = Visibility::Public;
        else if (consumeIf(TokenKind::KwPrivate))
          memVis = Visibility::Private;
        else if (consumeIf(TokenKind::KwProtected))
          memVis = Visibility::Protected;
        else if (consumeIf(TokenKind::KwStatic))
          isStatic = true;
        else if (consumeIf(TokenKind::KwAsync))
          isAsync = true;
        else if (consumeIf(TokenKind::KwShared))
          isShared = true;
        else if (consumeIf(TokenKind::KwWeak))
          isWeak = true;
        else
          break;
      }

      // Add constructor/destructor parsing to Generic classes!
      if (curTok.isAny(TokenKind::KwConstructor, TokenKind::KwDestructor)) {
        TokenKind kind = curTok.kind;
        std::string memName =
            (kind == TokenKind::KwConstructor) ? "constructor" : "destructor";
        SourceLocation mLoc = curTok.location;
        consume(); // eat constructor/destructor

        auto voidType =
            std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, mLoc);
        members.push_back(parseFunctionRest(std::move(voidType), memName, false,
                                            false, false, memVis));
        continue;
      }

      // 2. Parse Member Type and Name
      TypePtr memberType = parseType();
      if (!memberType) {
        synchronize();
        continue;
      }

      bool isConstVar = memberType->is<ConstType>();
      bool isVolatileVar = memberType->is<VolatileType>();

      std::string memName = curTok.getSpelling().str();
      expect(TokenKind::Identifier);

      if (curTok.is(TokenKind::LParen)) {
        members.push_back(parseFunctionRest(std::move(memberType), memName,
                                            isAsync, isStatic, isWeak, memVis));
      } else {
        auto varDecl =
            parseVariableRest(std::move(memberType), memName, isConstVar,
                              isStatic, isShared, memVis);
        if (auto var = dynamic_cast<VariableDecl *>(varDecl.get())) {
          var->setVolatile(isVolatileVar);
        }
        members.push_back(std::move(varDecl));
      }
    }
    expect(TokenKind::RBrace);

    // Pass parentName to the ClassDecl constructor so Generic classes can
    // inherit!
    auto classDecl = std::make_unique<ClassDecl>(
        name, parentNames, std::move(members), false, AggregateKind::Class,
        Visibility::Default, loc);
    return std::make_unique<GenericDecl>(name, std::move(typeParams),
                                         std::move(classDecl), loc);
  }

  // Fallback: Support "generic <T> class Box" by wrapping the next declaration
  DeclPtr innerDecl = parseTopLevelDecl();
  if (!innerDecl)
    return nullptr;

  return std::make_unique<GenericDecl>(
      innerDecl->getName(), std::move(typeParams), std::move(innerDecl), loc);
}

DeclPtr Parser::parseClassDecl() {
  bool isRef = consumeIf(TokenKind::KwRef);
  SourceLocation loc = curTok.location;

  AggregateKind aggKind = AggregateKind::Class;
  if (curTok.is(TokenKind::KwStruct)) {
    aggKind = AggregateKind::Struct;
  } else if (curTok.is(TokenKind::KwUnion)) {
    aggKind = AggregateKind::Union;
  }

  if (curTok.isAny(TokenKind::KwClass, TokenKind::KwStruct,
                   TokenKind::KwUnion)) {
    consume();
  } else {
    error("Expected 'class', 'struct', or 'union'");
    return nullptr;
  }

  std::string name = curTok.getSpelling().str();
  if (!expect(TokenKind::Identifier)) {
    return nullptr;
  }

  // Multiple Inheritance Parsing
  std::vector<std::string> parentNames;
  if (consumeIf(TokenKind::LParen)) {
    do {
      if (curTok.is(TokenKind::Identifier)) {
        parentNames.push_back(curTok.getSpelling().str());
        consume();
      } else {
        error("Expected parent class name");
      }
    } while (consumeIf(TokenKind::Comma)); // Loop as long as there are commas!

    expect(TokenKind::RParen); // Strictly require the closing parenthesis
  }

  expect(TokenKind::LBrace);
  std::vector<DeclPtr> members;

  while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
    // 1. Parse Member Modifiers
    Visibility memVis = Visibility::Default;
    bool isStatic = false;
    bool isConst = false;
    bool isAsync = false;
    bool isShared = false;
    bool isWeak = false;
    int alignment = 0;
    std::string sectionName = "";
    bool isVolatile = false;
    bool isThreadLocal = false;

    while (true) {
      if (consumeIf(TokenKind::KwPublic))
        memVis = Visibility::Public;
      else if (consumeIf(TokenKind::KwPrivate))
        memVis = Visibility::Private;
      else if (consumeIf(TokenKind::KwProtected))
        memVis = Visibility::Protected;
      else if (consumeIf(TokenKind::KwStatic))
        isStatic = true;
      else if (consumeIf(TokenKind::KwAsync))
        isAsync = true;
      else if (consumeIf(TokenKind::KwShared))
        isShared = true;
      else if (consumeIf(TokenKind::KwWeak))
        isWeak = true;
      else if (consumeIf(TokenKind::KwThreadLocal))
        isThreadLocal = true;
      else if (consumeIf(TokenKind::KwAlign)) {
        expect(TokenKind::LParen);
        if (curTok.is(TokenKind::IntegerLiteral)) {
          alignment = std::stoull(curTok.getSpelling().str());
          consume();
        } else {
          error("Expected integer alignment");
        }
        expect(TokenKind::RParen);
      } else if (consumeIf(TokenKind::KwSection)) {
        expect(TokenKind::LParen);
        if (curTok.is(TokenKind::StringLiteral)) {
          sectionName = curTok.getSpelling().str();
          if (sectionName.size() >= 2)
            sectionName = sectionName.substr(1, sectionName.size() - 2);
          consume();
        } else {
          error("Expected section name");
        }
        expect(TokenKind::RParen);
      } else
        break;
    }

    if (curTok.isAny(TokenKind::KwConstructor, TokenKind::KwDestructor)) {
      TokenKind kind = curTok.kind;
      std::string memName =
          (kind == TokenKind::KwConstructor) ? "constructor" : "destructor";
      SourceLocation mLoc = curTok.location;
      consume(); // eat constructor/destructor

      // Implicitly returns void
      auto voidType =
          std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, mLoc);
      members.push_back(parseFunctionRest(std::move(voidType), memName, false,
                                          false, false, memVis));
      continue;
    }

    TypePtr memberType = parseType();
    if (!memberType) {
      synchronize();
      continue;
    }

    bool isConstVar = memberType->is<ConstType>();
    bool isVolatileVar = memberType->is<VolatileType>();

    std::string memName;
    if (consumeIf(TokenKind::KwOperator)) {
      memName = "operator";
      switch (curTok.kind) {
      case TokenKind::Plus:
        memName += "+";
        consume();
        break;
      case TokenKind::Minus:
        memName += "-";
        consume();
        break;
      case TokenKind::Star:
        memName += "*";
        consume();
        break;
      case TokenKind::Slash:
        memName += "/";
        consume();
        break;
      case TokenKind::EqualEqual:
        memName += "==";
        consume();
        break;
      case TokenKind::NotEqual:
        memName += "!=";
        consume();
        break;
      case TokenKind::Less:
        memName += "<";
        consume();
        break;
      case TokenKind::Greater:
        memName += ">";
        consume();
        break;
      case TokenKind::LessEqual:
        memName += "<=";
        consume();
        break;
      case TokenKind::GreaterEqual:
        memName += ">=";
        consume();
        break;
      case TokenKind::LBracket:
        memName += "[";
        consume();
        if (consumeIf(TokenKind::RBracket))
          memName += "]";
        break;
      default:
        error("Invalid operator for overloading");
        break;
      }
    } else {
      memName = curTok.getSpelling().str();
      expect(TokenKind::Identifier);
    }

    if (curTok.is(TokenKind::LParen)) {
      if (isConstVar)
        error("'const' on methods not supported yet");
      auto fnDecl = parseFunctionRest(std::move(memberType), memName, isAsync,
                                      isStatic, isWeak, memVis);
      if (auto fn = dynamic_cast<FunctionDecl *>(fnDecl.get())) {
        fn->setSection(sectionName);
      }
      members.push_back(std::move(fnDecl));
    } else {
      if (isAsync)
        error("'async' on fields not supported");
      auto varDecl = parseVariableRest(std::move(memberType), memName,
                                       isConstVar, isStatic, isShared, memVis);
      if (auto var = dynamic_cast<VariableDecl *>(varDecl.get())) {
        var->setVolatile(isVolatileVar);
        var->setAlignment(alignment);
        var->setSection(sectionName);
        var->setThreadLocal(isThreadLocal);
      }
      members.push_back(std::move(varDecl));
    }
  }
  expect(TokenKind::RBrace);

  return std::make_unique<ClassDecl>(name, parentNames, std::move(members),
                                     isRef, aggKind, Visibility::Default, loc);
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

DeclPtr Parser::parseMacroDecl() {
  SourceLocation loc = curTok.location;
  consume(); // eat 'macro'

  std::string name = curTok.getSpelling().str();
  expect(TokenKind::Identifier);

  bool isFunctionMacro = false;
  std::vector<std::string> params;

  if (consumeIf(TokenKind::LParen)) {
    isFunctionMacro = true;
    if (curTok.isNot(TokenKind::RParen)) {
      do {
        if (curTok.is(TokenKind::Identifier)) {
          params.push_back(curTok.getSpelling().str());
          consume();
        }
      } while (consumeIf(TokenKind::Comma));
    }
    expect(TokenKind::RParen);
  }

  std::vector<StmtPtr> body;
  if (curTok.is(TokenKind::LBrace)) {
    consume(); // eat '{'
    while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
      if (auto s = parseStatement())
        body.push_back(std::move(s));
      else
        synchronize();
    }
    expect(TokenKind::RBrace);
  } else {
    auto expr = parseExpression();
    body.push_back(std::make_unique<ExpressionStmt>(std::move(expr), loc));
  }

  return std::make_unique<MacroDecl>(name, std::move(params), std::move(body),
                                     isFunctionMacro, loc);
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
  case TokenKind::KwThrow:
    return parseThrowStmt();
  case TokenKind::KwUnsafe:
    return parseUnsafeBlock();
  case TokenKind::KwLock:
    if (nextTok.is(TokenKind::LBrace) || nextTok.is(TokenKind::LParen)) {
      return parseLockStmt();
    }
    // Fallthrough to declaration logic
    if (isStartOfDeclaration()) {
      SourceLocation loc = curTok.location;
      DeclPtr decl = parseTopLevelDecl();
      if (decl)
        return std::make_unique<DeclStmt>(std::move(decl), loc);
      return nullptr;
    }
    return nullptr;

  case TokenKind::KwView:
  case TokenKind::KwMut:
    if (nextTok.is(TokenKind::LBrace)) {
      return parseLockStmt();
    }
    // Fallthrough to declaration logic
    if (isStartOfDeclaration()) {
      SourceLocation loc = curTok.location;
      DeclPtr decl = parseTopLevelDecl();
      if (decl) {
        return std::make_unique<DeclStmt>(std::move(decl), loc);
      }
      return nullptr;
    }
    return nullptr;
  case TokenKind::KwAsm: {
    SourceLocation loc = curTok.location;
    consume();
    expect(TokenKind::LParen);
    std::string asmStr = "";
    std::string constraints = "";
    if (curTok.is(TokenKind::StringLiteral)) {
      asmStr = curTok.getSpelling().str();
      if (asmStr.size() >= 2)
        asmStr = asmStr.substr(1, asmStr.size() - 2);
      consume();
    } else {
      error("Expected string literal");
    }
    if (consumeIf(TokenKind::Comma)) {
      if (curTok.is(TokenKind::StringLiteral)) {
        constraints = curTok.getSpelling().str();
        if (constraints.size() >= 2)
          constraints = constraints.substr(1, constraints.size() - 2);
        consume();
      } else {
        error("Expected string literal for inline asm constraints");
      }
    }
    expect(TokenKind::RParen);
    consumeIf(TokenKind::Semicolon);
    return std::make_unique<AsmStmt>(std::move(asmStr), std::move(constraints),
                                     loc);
  }
  default: {
    // Variable Declaration
    if (isStartOfDeclaration()) {
      SourceLocation loc = curTok.location;
      DeclPtr decl = parseTopLevelDecl();
      if (decl) {
        return std::make_unique<DeclStmt>(std::move(decl), loc);
      }
      return nullptr;
    }
    // Ambiguity Check
    if (curTok.is(TokenKind::Identifier) && peekIs(TokenKind::Identifier)) {
      return parseVariableStmt();
    }
    SourceLocation loc = curTok.location;
    // Expression Statement
    ExprPtr expr = parseExpression();

    // Check for null to ensure we don't create an invalid statement
    if (!expr) {
      return nullptr; // Caller (parseBlock) will call synchronize()
    }

    consumeIf(TokenKind::Semicolon);
    return std::make_unique<ExpressionStmt>(std::move(expr), loc);
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

  bool isTypedDeclaration = isStartOfDeclaration();

  if (isTypedDeclaration) {
    varDecl1 = parseVariableDecl();

    // Check if this is a Typed For-In loop: for (int x in arr)
    if (varDecl1 && consumeIf(TokenKind::KwIn)) {
      ExprPtr collection = parseExpression();
      expect(TokenKind::RParen);
      StmtPtr body = parseStatement();
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
  consume(); // Eat 'switch'
  expect(TokenKind::LParen);
  auto cond = parseExpression();
  expect(TokenKind::RParen);
  expect(TokenKind::LBrace);
  loopDepth++;
  std::vector<SwitchCase> cases;
  while (consumeIf(TokenKind::KwCase)) {
    std::vector<ExprPtr> values;
    do {
      auto val = parseExpression();

      if (curTok.is(TokenKind::Colon) &&
          nextTok.isAny(TokenKind::IntegerLiteral, TokenKind::FloatLiteral,
                        TokenKind::StringLiteral, TokenKind::CharLiteral)) {
        consume(); // Eat range separator ':'
        auto endVal = parseExpression();
        val = std::make_unique<BinaryExpr>(std::move(val), TokenKind::Colon,
                                           std::move(endVal), curTok.location);
      }

      values.push_back(std::move(val));
    } while (consumeIf(TokenKind::Comma));

    expect(TokenKind::Colon); // Case terminator

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
  loopDepth--;
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
  SourceLocation loc = curTok.location;
  consume(); // Eat 'lock' / 'view' / 'mut'

  ExprPtr target = nullptr;
  if (consumeIf(TokenKind::LParen)) {
    target = parseExpression();
    expect(TokenKind::RParen);
  }

  auto body = parseBlock();

  // Return a real LockStmt instead of just throwing the target away!
  return std::make_unique<LockStmt>(std::move(target), std::move(body), loc);
}

StmtPtr Parser::parseThrowStmt() {
  SourceLocation loc = curTok.location;
  consume(); // Eat 'throw'
  auto expr = parseExpression();
  consumeIf(TokenKind::Semicolon); // Optional semicolon
  return std::make_unique<ThrowStmt>(std::move(expr), loc);
}

// --- Expressions ---

ExprPtr Parser::parseExpression() { return parseAssignment(); }

ExprPtr Parser::parseAssignment() {
  auto left = parseTernary();
  if (!left)
    return nullptr; // Safety check

  if (curTok.isAny(
          TokenKind::Equal, TokenKind::PlusEqual, TokenKind::MinusEqual,
          TokenKind::StarEqual, TokenKind::SlashEqual, TokenKind::PercentEqual,
          TokenKind::AmpEqual, TokenKind::PipeEqual, TokenKind::CaretEqual,
          TokenKind::LessLessEqual, TokenKind::GreaterGreaterEqual)) {
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

ExprPtr Parser::parsePipe() {
  auto left = parseNullCoalescing();
  if (!left)
    return nullptr;

  while (curTok.is(TokenKind::PipeGreater)) {
    SourceLocation opLoc = curTok.location;
    consume(); // Eat '|>'

    auto right = parsePostfix();
    if (!right)
      return nullptr;

    // 2. Recursive Search: Find the 'functional' target in a postfix chain
    // This allows us to reach 'processNode()' inside 'processNode()?.next'
    Expr *target = right.get();
    while (true) {
      if (auto *mem = dynamic_cast<MemberExpr *>(target))
        target = const_cast<Expr *>(mem->getObject());
      else if (auto *idx = dynamic_cast<IndexExpr *>(target))
        target = const_cast<Expr *>(idx->getArray());
      else
        break;
    }

    // 3. Transformation Logic
    if (auto *call = dynamic_cast<CallExpr *>(target)) {
      // Case A: Right side contains a function call.
      // Inject LHS as the first argument.
      call->insertFirstArg(std::move(left));
      left = std::move(right);
    } else if (dynamic_cast<IdentifierExpr *>(target) ||
               dynamic_cast<MemberExpr *>(target)) {
      // Case B: Right side is a bare identifier or member (e.g., data |> print)
      // We wrap the entire 'right' expression as the callee of a new CallExpr.
      std::vector<ExprPtr> newArgs;
      newArgs.push_back(std::move(left));

      left = std::make_unique<CallExpr>(std::move(right), std::move(newArgs),
                                        opLoc);
    } else {
      error("Right side of '|>' must be a function call, identifier, or member "
            "access");
      return nullptr;
    }
  }

  return left;
}

ExprPtr Parser::parseTernary() {
  auto cond = parseLogicalOr();
  if (!cond)
    return nullptr;

  if (consumeIf(TokenKind::Question)) {
    auto trueBranch = parseTernary();
    expect(TokenKind::Colon);
    auto falseBranch = parseTernary();
    return std::make_unique<TernaryExpr>(std::move(cond), std::move(trueBranch),
                                         std::move(falseBranch),
                                         cond->getLoc());
  }
  return cond;
}

ExprPtr Parser::parseNullCoalescing() {
  auto left = parseShift();
  while (consumeIf(TokenKind::QuestionQuestion)) {
    auto right = parseShift();
    left = std::make_unique<BinaryExpr>(std::move(left),
                                        TokenKind::QuestionQuestion,
                                        std::move(right), left->getLoc());
  }
  return left;
}

ExprPtr Parser::parseLogicalOr() {
  auto left = parseLogicalAnd();
  while (curTok.is(TokenKind::PipePipe)) {
    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location;
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
    if (hasNewlineBeforeToken(curTok)) {
      break;
    }
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
  auto left = parsePipe();
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
    auto right = parsePipe();
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
    if (hasNewlineBeforeToken(curTok)) {
      break;
    }
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
  auto left = parsePower();
  while (curTok.isAny(TokenKind::Star, TokenKind::Slash, TokenKind::Percent)) {
    if (curTok.is(TokenKind::Star)) {
      if (isStartOfDeclaration()) {
        break;
      }
      if (hasNewlineBeforeToken(curTok)) {
        break;
      }
    }
    if (!left)
      return nullptr;
    TokenKind op = curTok.kind;
    SourceLocation opLoc = curTok.location;
    consume();
    auto right = parsePower();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                        opLoc);
  }
  return left;
}

ExprPtr Parser::parsePower() {
  auto left = parsePrefix();

  // Right-Associative: a ** b ** c -> a ** (b ** c)
  if (curTok.is(TokenKind::Power)) {
    if (isStartOfDeclaration()) {
      return left;
    }
    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location;
    consume(); // Eat '**'

    auto right = parsePower(); // Recursion!
    if (!right)
      return nullptr;

    return std::make_unique<BinaryExpr>(std::move(left), TokenKind::Power,
                                        std::move(right), opLoc);
  }
  return left;
}

ExprPtr Parser::parsePrefix() {
  if (curTok.is(TokenKind::KwAwait)) {
    SourceLocation loc = curTok.location;
    consume();
    auto operand = parsePrefix(); // Recursive to allow 'await await x'
    return std::make_unique<AwaitExpr>(std::move(operand), loc);
  }

  // Existing Unary Ops
  if (curTok.isAny(TokenKind::Bang, TokenKind::Minus, TokenKind::Tilde,
                   TokenKind::PlusPlus, TokenKind::MinusMinus,
                   TokenKind::DotDotDot, TokenKind::Amp, TokenKind::Star)) {
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
    if (!left)
      return nullptr;
    if (curTok.is(TokenKind::LParen)) {
      if (hasNewlineBeforeToken(curTok)) {
        break;
      }
      consume();
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
    } else if (curTok.is(TokenKind::Dot) || curTok.is(TokenKind::QuestionDot)) {
      bool isOptional = (curTok.kind == TokenKind::QuestionDot);
      consume(); // Eat the . or ?.

      std::string name = curTok.getSpelling().str();
      expect(TokenKind::Identifier);
      left = std::make_unique<MemberExpr>(std::move(left), name, isOptional,
                                          left->getLoc());
    } else if (consumeIf(TokenKind::PlusPlus)) {
      left = std::make_unique<UnaryExpr>(TokenKind::PlusPlus, std::move(left),
                                         true, left->getLoc());
    }
    // Handle Postfix Decrement (x--)
    else if (consumeIf(TokenKind::MinusMinus)) {
      left = std::make_unique<UnaryExpr>(TokenKind::MinusMinus, std::move(left),
                                         true, left->getLoc());
    } else if (curTok.is(TokenKind::LBracket)) {
      if (hasNewlineBeforeToken(curTok)) {
        break;
      }
      consume();
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
  case TokenKind::KwCast: {
    consume();
    if (!expect(TokenKind::Less))
      return nullptr;
    TypePtr type = parseType();
    if (!type)
      return nullptr;
    expectGreater();
    if (!expect(TokenKind::LParen))
      return nullptr;
    ExprPtr expr = parseExpression();
    if (!expr)
      return nullptr;
    expect(TokenKind::RParen);
    return std::make_unique<CastExpr>(std::move(type), std::move(expr), loc);
  }
  case TokenKind::IntegerLiteral: {
    std::string text = curTok.getSpelling().str();

    // [FIX 1] Strip underscores so '1_000_000' becomes '1000000'
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());

    uint64_t val = 0;
    try {
      // [FIX 2] Handle binary '0b' prefix manually
      if (text.size() >= 2 && text[0] == '0' &&
          (text[1] == 'b' || text[1] == 'B')) {
        val = std::stoull(text.substr(2), nullptr, 2);
      } else {
        // Base 0 automatically handles hex (0x) and decimal
        val = std::stoull(text, nullptr, 0);
      }
    } catch (...) {
      Diags.report(curTok.location, DiagID::err_type_mismatch)
          << "Integer literal '" << text << "' is out of range for i64";
    }
    NumericSuffix suffix = curTok.suffix;
    consume();
    return std::make_unique<IntegerLiteral>(val, suffix, loc);
  }
  case TokenKind::FloatLiteral: {
    std::string text = curTok.getSpelling().str();

    // [FIX 3] Strip underscores for floats too (e.g., 1_000.50)
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());

    double val = 0.0;
    try {
      val = std::stod(text);
    } catch (...) {
      error("Invalid float literal");
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
          const size_t prefixOffset = 3;

          if (text.size() >= prefixOffset + hexLen) {
            unsigned long long intVal = 0;
            // Simple hex parser loop
            for (size_t i = 0; i < hexLen; i++) {
              char c = text[prefixOffset + i];
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
  case TokenKind::KwInt:
  case TokenKind::KwFloat:
  case TokenKind::KwDouble:
  case TokenKind::KwBool:
  case TokenKind::KwString:
  case TokenKind::KwChar:
  case TokenKind::KwShort:
  case TokenKind::KwLong:
  case TokenKind::KwISize:
  case TokenKind::KwUSize:
  case TokenKind::KwHalf:
  case TokenKind::KwQuarter:
  case TokenKind::KwAny: {
    std::string name = curTok.getSpelling().str();
    consume(); // Eat the type keyword
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
                     TokenKind::KwString, TokenKind::KwVoid, TokenKind::KwAny,
                     TokenKind::KwChar, TokenKind::KwDouble, TokenKind::KwHalf,
                     TokenKind::KwQuarter, TokenKind::KwShort,
                     TokenKind::KwLong, TokenKind::KwUSize,
                     TokenKind::KwISize)) {

      // Ensure an identifier actually follows before assuming it's a
      // lambda
      if (nextTok.is(TokenKind::Identifier)) {
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
      // If there is NO identifier (e.g. `(int(x))`), we fall through to the
      // grouping fallback!
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
  case TokenKind::LBrace:
    return parseMapLiteral();
  case TokenKind::TemplateString:
  case TokenKind::StringFragment:
  case TokenKind::InterpolationStart:
    return parseTemplateString();
  case TokenKind::KwThis: {
    consume();
    return std::make_unique<ThisExpr>(loc);
  }
  case TokenKind::KwSuper: {
    consume();
    return std::make_unique<SuperExpr>(loc);
  }
  case TokenKind::Error:
    consume();
    return nullptr;
  case TokenKind::KwSizeof: {
    consume(); // Eat 'sizeof'
    expect(TokenKind::LParen);

    // Sizeof usually takes an expression (or type). We parse it as an
    // expression so the Sema pass can resolve its byte size.
    auto expr = parseExpression();

    expect(TokenKind::RParen);

    // Note: Ensure you define `SizeOfExpr` in `Expr.h` / `Expr.cpp`!
    return std::make_unique<SizeOfExpr>(std::move(expr), loc);
  }
  default:
    error("Expected expression");
    return nullptr;
  }
}

ExprPtr Parser::parseArrayLiteral() {
  SourceLocation loc = curTok.location;
  consume(); // Eat '['
  std::vector<ExprPtr> elements;
  while (curTok.isNot(TokenKind::RBracket) && curTok.isNot(TokenKind::Eof)) {
    // Add support for spread operator
    if (curTok.is(TokenKind::DotDotDot)) {
      consume(); // Eat '...'
      ExprPtr subExpr = parseExpression();
      elements.push_back(std::make_unique<UnaryExpr>(
          TokenKind::DotDotDot, std::move(subExpr), false, loc));
    } else {
      elements.push_back(parseExpression());
    }

    if (!consumeIf(TokenKind::Comma))
      break;
  }
  expect(TokenKind::RBracket);
  return std::make_unique<ArrayLiteral>(std::move(elements), loc);
}

ExprPtr Parser::parseMapLiteral() {
  SourceLocation loc = curTok.location;
  consume(); // Eat '{'

  std::vector<std::pair<ExprPtr, ExprPtr>> entries;

  if (curTok.isNot(TokenKind::RBrace)) {
    do {
      ExprPtr key = parseExpression();
      expect(TokenKind::Colon);
      ExprPtr val = parseExpression();
      entries.emplace_back(std::move(key), std::move(val));
    } while (consumeIf(TokenKind::Comma));
  }

  expect(TokenKind::RBrace);
  return std::make_unique<MapLiteral>(std::move(entries), loc);
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
  if (curTok.is(TokenKind::KwThread)) {
    consume(); // Eat 'Thread'

    // Match the syntax: Thread(() => { ... })
    if (!expect(TokenKind::LParen))
      return nullptr;

    auto expr =
        parseExpression(); // This will correctly parse the lambda () => { ... }
    if (!expr)
      return nullptr;

    // Verify it's actually a lambda
    if (auto lambda = llvm::dyn_cast<LambdaExpr>(expr.get())) {
      expr.release(); // Transfer ownership from the generic ExprPtr
      std::unique_ptr<LambdaExpr> typedBody(lambda);

      if (!expect(TokenKind::RParen))
        return nullptr;

      return std::make_unique<ThreadExpr>(isWeak, std::move(typedBody), loc);
    } else {
      error("Expected lambda expression inside Thread constructor");
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
  bool isVolatile = consumeIf(TokenKind::KwVolatile);
  bool isConst = consumeIf(TokenKind::KwConst);
  bool isUnsigned = consumeIf(TokenKind::KwUnsigned);

  switch (curTok.kind) {
  // Prefix Pointers & References
  case TokenKind::Amp: {
    SourceLocation ampLoc = curTok.location;
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<ReferenceType>(std::move(inner), ampLoc);
    break;
  }
  // Prefix Pointers
  case TokenKind::Star: {
    SourceLocation starLoc = curTok.location;
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<PointerType>(std::move(inner), starLoc);
    break;
  }
  case TokenKind::Power: { // Handles **mut int
    SourceLocation starLoc = curTok.location;
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<PointerType>(
        std::make_unique<PointerType>(std::move(inner), starLoc), starLoc);
    break;
  }
  // --- Primitives ---
  case TokenKind::KwInt:
    // If 'unsigned' was present, map to U32 instead of I32
    type = std::make_unique<PrimitiveType>(
        isUnsigned ? PrimitiveType::Scalar::U32 : PrimitiveType::Scalar::I32,
        loc);
    consume();
    break;
  case TokenKind::KwShort:
    type = std::make_unique<PrimitiveType>(
        isUnsigned ? PrimitiveType::Scalar::U16 : PrimitiveType::Scalar::I16,
        loc);
    consume();
    break;
  case TokenKind::KwLong:
    type = std::make_unique<PrimitiveType>(
        isUnsigned ? PrimitiveType::Scalar::U64 : PrimitiveType::Scalar::I64,
        loc);
    consume();
    break;
  case TokenKind::KwChar:
    // char is usually u8 or i8; apply logic as needed
    type = std::make_unique<PrimitiveType>(
        isUnsigned ? PrimitiveType::Scalar::U8 : PrimitiveType::Scalar::Char,
        loc);
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
  case TokenKind::KwVoid:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc);
    consume();
    break;
  case TokenKind::KwNull:
    type = std::make_unique<NullType>(loc);
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
      expectGreater();
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
      expectGreater();
    }
    type = std::make_unique<NamedType>(name, std::move(args), loc);
    break;
  }

  // --- Function Type: (Args) => Ret ---
  case TokenKind::KwInterrupt:
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
                                            false, false, loc);
    } else {
      error("Expected '=>' for function type");
      return nullptr;
    }
    break;
  }

  default:
    return nullptr;
  }

  if (isConst) {
    type = std::make_unique<ConstType>(std::move(type), loc);
  }
  if (isVolatile) {
    type = std::make_unique<VolatileType>(std::move(type), loc);
  }

  // --- Suffixes (Ptr*, Array[], Nullable?) ---
  while (true) {
    SourceLocation loc = curTok.location;

    // Case 1: int?[] (Nullable Array)
    if (curTok.is(TokenKind::Question) && peekIs(TokenKind::LBracket)) {
      consume(); // Eat prefix '?'

      std::vector<ExprPtr> sizes;
      do {
        consume(); // Eat '['
        ExprPtr sizeExpr =
            (curTok.isNot(TokenKind::RBracket)) ? parseExpression() : nullptr;
        expect(TokenKind::RBracket);
        sizes.push_back(std::move(sizeExpr));
      } while (curTok.is(TokenKind::LBracket));

      bool trailingQuestion = consumeIf(TokenKind::Question);

      auto it = sizes.rbegin();
      // Apply all INNER array dimensions first
      while (it != sizes.rend() - 1) {
        type =
            std::make_unique<ArrayType>(std::move(type), std::move(*it), loc);
        ++it;
      }

      // The trailing '?' makes the OUTERMOST elements nullable
      if (trailingQuestion) {
        type = std::make_unique<NullableType>(std::move(type), loc);
      }

      // Apply the OUTERMOST dimension
      type = std::make_unique<ArrayType>(std::move(type), std::move(*it), loc);

      // Final wrap: Apply the prefix '?' to make the entire array structure
      // nullable
      type = std::make_unique<NullableType>(std::move(type), loc);

    }
    // Case 2: int[]? (Non-nullable array of nullables)
    else if (curTok.is(TokenKind::LBracket)) {
      std::vector<ExprPtr> sizes;
      do {
        consume(); // Eat '['
        ExprPtr sizeExpr =
            (curTok.isNot(TokenKind::RBracket)) ? parseExpression() : nullptr;
        expect(TokenKind::RBracket);
        sizes.push_back(std::move(sizeExpr));
      } while (curTok.is(TokenKind::LBracket));

      bool trailingQuestion = consumeIf(TokenKind::Question);

      auto it = sizes.rbegin();
      // Apply all INNER array dimensions first
      while (it != sizes.rend() - 1) {
        type =
            std::make_unique<ArrayType>(std::move(type), std::move(*it), loc);
        ++it;
      }

      // The trailing '?' makes the OUTERMOST elements nullable
      if (trailingQuestion) {
        type = std::make_unique<NullableType>(std::move(type), loc);
      }

      // Apply the OUTERMOST dimension
      type = std::make_unique<ArrayType>(std::move(type), std::move(*it), loc);
    }
    // Case 3: Standard T? (Primitive nullable)
    else if (consumeIf(TokenKind::Question)) {
      type = std::make_unique<NullableType>(std::move(type), loc);
    }
    // Case 4: Pointers (int*)
    else if (consumeIf(TokenKind::Star)) {
      type = std::make_unique<PointerType>(std::move(type), loc);
    } else {
      break;
    }
  }
  return type;
}

} // namespace moksha
