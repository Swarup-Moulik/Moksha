#include "moksha/Parser/Parser.h"
#include "moksha/AST/ASTContext.h"
#include "moksha/AST/Expr.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"
#include "moksha/Support/Diagnostics.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/raw_ostream.h"

namespace moksha {

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

/** @brief Helper for better error messages */
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
  case TokenKind::DecimalLiteral:
    return "decimal literal";
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

  // Keywords
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
  case TokenKind::KwDecimal:
    return "'decimal'";
  case TokenKind::KwAny:
    return "'any'";
  case TokenKind::KwPromise:
    return "'promise'";
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
  case TokenKind::KwThread:
    return "'thread'";
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
  case TokenKind::KwNoinline:
    return "'noinline'";
  case TokenKind::KwInline:
    return "'inline'";
  case TokenKind::KwPure:
    return "'pure'";
  case TokenKind::KwCold:
    return "'cold'";
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
  case TokenKind::KwClosure:
    return "'closure'";
  case TokenKind::KwOut:
    return "'out'";
  case TokenKind::KwInout:
    return "'inout'";
  case TokenKind::KwClobber:
    return "'clobber'";
  case TokenKind::KwBitcast:
    return "'bitcast'";

  // Operators
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

// Helper to evaluate escape sequences into true memory bytes
static std::string unescapeString(llvm::StringRef raw) {
  std::string result;
  for (size_t i = 0; i < raw.size(); ++i) {
    if (raw[i] == '\\' && i + 1 < raw.size()) {
      char escape = raw[i + 1];
      switch (escape) {
      case 'n':
        result += '\n';
        i++;
        break;
      case 'r':
        result += '\r';
        i++;
        break;
      case 't':
        result += '\t';
        i++;
        break;
      case '0':
        result += '\0';
        i++;
        break;
      case '\\':
        result += '\\';
        i++;
        break;
      case '\'':
        result += '\'';
        i++;
        break;
      case '"':
        result += '"';
        i++;
        break;
      case '`':
        result += '`';
        i++;
        break;
      case '$':
        result += '$';
        i++;
        break;
      case 'x':
        if (i + 3 < raw.size()) {
          std::string hexStr = raw.substr(i + 2, 2).str();
          try {
            result += static_cast<char>(std::stoi(hexStr, nullptr, 16));
          } catch (...) {
          }
          i += 3;
        } else {
          result += raw[i];
        }
        break;
      case 'u':
      case 'U': {
        uint32_t codePoint = 0;
        size_t consumed = 0;

        // Variable-length Rust-style: \u{1F600}
        if (escape == 'u' && i + 2 < raw.size() && raw[i + 2] == '{') {
          size_t endBrace = raw.find('}', i + 3);
          if (endBrace != llvm::StringRef::npos) {
            std::string hexStr = raw.substr(i + 3, endBrace - (i + 3)).str();
            try {
              codePoint = std::stoul(hexStr, nullptr, 16);
            } catch (...) {
            }
            consumed = (endBrace - i); // Bytes to advance
          }
        }
        // Fixed-length: \uXXXX (4 hex)
        else if (escape == 'u' && i + 5 < raw.size()) {
          std::string hexStr = raw.substr(i + 2, 4).str();
          try {
            codePoint = std::stoul(hexStr, nullptr, 16);
          } catch (...) {
          }
          consumed = 5;
        }
        // Fixed-length: \UXXXXXXXX (8 hex)
        else if (escape == 'U' && i + 9 < raw.size()) {
          std::string hexStr = raw.substr(i + 2, 8).str();
          try {
            codePoint = std::stoul(hexStr, nullptr, 16);
          } catch (...) {
          }
          consumed = 9;
        }

        if (consumed > 0) {
          char utf8Buf[4];
          char *ptr = utf8Buf;
          if (llvm::ConvertCodePointToUTF8(codePoint, ptr)) {
            result.append(utf8Buf, ptr - utf8Buf);
          }
          i += consumed;
        } else {
          result += raw[i];
        }
        break;
      }
      default:
        result += escape;
        i++;
        break;
      }
    } else {
      result += raw[i];
    }
  }
  return result;
}

// Safely parse { a, b } without hanging
void Parser::parseImportSymbolList(std::vector<std::string> &symbols) {
  while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
    if (curTok.is(TokenKind::Identifier) || curTok.is(TokenKind::KwTable)) {
      symbols.push_back(curTok.getSpelling().str());
      consume();
    } else {
      error("Expected identifier or type in import list");
      consume();
    }

    if (curTok.isNot(TokenKind::RBrace)) {
      if (!consumeIf(TokenKind::Comma)) {
        error("Expected ',' between imported symbols");
        consume();
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
    return true;
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
  if (curTok.is(TokenKind::Eof))
    return;

  if (curTok.is(TokenKind::Semicolon)) {
    advance();
    return;
  }

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
          TokenKind::KwInline, TokenKind::KwPure, TokenKind::KwCold,
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
    if (curTok.is(TokenKind::KwAsync) && nextTok.is(TokenKind::KwLock)) {
      return false;
    }
    return true;
  }

  // 1.5. Closure types ALWAYS start a declaration, even with parentheses
  if (curTok.is(TokenKind::KwClosure)) {
    return true;
  }

  // Function Pointer Types: (Type) => ReturnType
  if (curTok.is(TokenKind::LParen)) {
    const char *ptr = curTok.getSpelling().data();
    int depth = 0;
    while (*ptr != '\0' && *ptr != ';' && *ptr != '\n') {
      if (*ptr == '(') {
        depth++;
      } else if (*ptr == ')') {
        depth--;
        if (depth == 0) {
          ptr++;
          while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n')
            ptr++;
          // If '=>' follows, it's a function pointer declaration
          if (*ptr == '=' && *(ptr + 1) == '>') {
            return true;
          } else {
            return false; // It's just a grouped expression (e.g., (1 + 2))
          }
        }
      }
      ptr++;
    }
  }

  // 2. Primitive types start declarations (int x, void foo)
  if (curTok.isAny(TokenKind::KwInt, TokenKind::KwFloat, TokenKind::KwBool,
                   TokenKind::KwString, TokenKind::KwVoid, TokenKind::KwAny,
                   TokenKind::KwChar, TokenKind::KwISize, TokenKind::KwUSize,
                   TokenKind::KwShort, TokenKind::KwLong, TokenKind::KwDouble,
                   TokenKind::KwHalf, TokenKind::KwQuarter,
                   TokenKind::KwUnsigned, TokenKind::KwDecimal,
                   TokenKind::KwPromise)) {
    if (nextTok.is(TokenKind::LParen)) {
      return false;
    }
    return true;
  }

  // 3. Identifier Ambiguity: "MyType x" (Decl) vs "myVar = 5" (Stmt)
  if (curTok.is(TokenKind::Identifier)) {
    // We need a manual lookahead to handle namespace dots: "module.Type name"
    const char *ptr = curTok.getSpelling().data();

    // Skip the current identifier
    while ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') ||
           *ptr == '_' || (*ptr >= '0' && *ptr <= '9'))
      ptr++;

    // Skip any ".Identifier" chains
    while (true) {
      const char *temp = ptr;
      while (*temp == ' ' || *temp == '\t' || *temp == '\r' || *temp == '\n')
        temp++;
      if (*temp == '.') {
        temp++; // skip '.'
        while (*temp == ' ' || *temp == '\t' || *temp == '\r' || *temp == '\n')
          temp++;
        if ((*temp >= 'a' && *temp <= 'z') || (*temp >= 'A' && *temp <= 'Z') ||
            *temp == '_') {
          while ((*temp >= 'a' && *temp <= 'z') ||
                 (*temp >= 'A' && *temp <= 'Z') || *temp == '_' ||
                 (*temp >= '0' && *temp <= '9'))
            temp++;
          ptr = temp; // Accept the segment
          continue;
        }
      }
      break;
    }

    // Now `ptr` points just after the (possibly namespaced) type name.
    // Skip whitespace
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n')
      ptr++;

    // If what follows is an identifier (variable name), it's a declaration!
    if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') ||
        *ptr == '_') {
      return true;
    }

    // "Type<...> Name" -> Declaration
    if (*ptr == '<') {
      int depth = 0;
      while (*ptr != '\0' && *ptr != ';' && *ptr != '\n') {
        if (*ptr == '<')
          depth++;
        else if (*ptr == '>') {
          depth--;
          if (depth == 0) {
            ptr++;
            while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n')
              ptr++;
            if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') ||
                *ptr == '_')
              return true;
            else
              return false;
          }
        }
        ptr++;
      }
      return false;
    }

    // "Type[] Name", "Type? Name", "Type* Name" -> Declaration vs Operator
    // ambiguity
    while (*ptr != '\0') {
      if (*ptr == '[') {
        int depth = 0;
        while (*ptr != '\0') {
          if (*ptr == '[')
            depth++;
          else if (*ptr == ']')
            depth--;
          ptr++;
          if (depth == 0)
            break; // Reached the matching ']'
        }
      } else if (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') {
        ptr++; // Skip whitespace
      } else if (*ptr == '?' || *ptr == '*') {
        ptr++; // Skip nullable or pointer suffix
      } else if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') ||
                 *ptr == '_') {
        return true; // Followed by a variable name -> It's a Declaration!
      } else {
        return false;
      }
    }
    return false;
  }

  // 4. Prefix Pointers & References for Declarations
  if (curTok.isAny(TokenKind::Star, TokenKind::Power, TokenKind::Amp)) {
    const char *ptr = curTok.getSpelling().data();

    // Skip all prefix operators (*, &, whitespace)
    while (*ptr != '\0' && (*ptr == '*' || *ptr == '&' || *ptr == ' ' ||
                            *ptr == '\t' || *ptr == '\r' || *ptr == '\n')) {
      ptr++;
    }

    // Now `ptr` points to the start of the underlying type or identifier.
    std::string word;
    while ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') ||
           *ptr == '_' || (*ptr >= '0' && *ptr <= '9')) {
      word += *ptr;
      ptr++;
    }

    if (word == "mut" || word == "const" || word == "lock" || word == "view" ||
        word == "int" || word == "float" || word == "double" ||
        word == "bool" || word == "string" || word == "char" ||
        word == "short" || word == "long" || word == "isize" ||
        word == "usize" || word == "half" || word == "quarter" ||
        word == "void" || word == "any" || word == "unsigned" ||
        word == "volatile" || word == "decimal") {
      return true;
    }

    // If it's a custom type, we expect another identifier (the variable name)
    // after it. Skip template args if any
    if (*ptr == '<') {
      int depth = 0;
      while (*ptr != '\0' && *ptr != ';' && *ptr != '\n') {
        if (*ptr == '<')
          depth++;
        else if (*ptr == '>') {
          depth--;
          if (depth == 0) {
            ptr++;
            break;
          }
        }
        ptr++;
      }
    }

    // Skip trailing whitespace after the potential type name
    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' || *ptr == '\n') {
      ptr++;
    }

    // If the next character is a letter or underscore, it's a variable name ->
    // Declaration!
    if ((*ptr >= 'a' && *ptr <= 'z') || (*ptr >= 'A' && *ptr <= 'Z') ||
        *ptr == '_') {
      return true;
    }

    return false;
  }

  return false;
}

// Top Level

std::unique_ptr<ModuleDecl> Parser::parseModule() {
  std::vector<DeclPtr> decls;
  std::vector<StmtPtr> scriptStatements;
  SourceLocation startLoc = curTok.location;

  while (curTok.isNot(TokenKind::Eof)) {
    if (isStartOfDeclaration()) {
      auto newDecls = parseTopLevelDecls();
      for (auto &decl : newDecls) {
        if (decl) {
          decls.push_back(std::move(decl));
        }
      }
    } else {
      if (auto stmt = parseStatement()) {
        scriptStatements.push_back(std::move(stmt));
      } else {
        synchronize();
      }
    }
  }

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

      // Ensure imported scripts don't collide with the root file's main()
      static int scriptCounter = 0;
      std::string scriptFuncName =
          (scriptCounter == 0)
              ? "main"
              : "__moksha_imported_script_" + std::to_string(scriptCounter);
      scriptCounter++;

      auto mainFunc = std::make_unique<FunctionDecl>(
          scriptFuncName, std::vector<FunctionDecl::Param>{},
          std::move(voidType), std::move(body), false, false, false, false,
          Visibility::Default, startLoc);

      decls.push_back(std::move(mainFunc));
    }
  }

  return std::make_unique<ModuleDecl>("main", std::move(decls), startLoc);
}

std::vector<DeclPtr> Parser::parseTopLevelDecls() {
  // 1. Parse Modifiers
  Visibility vis = Visibility::Default;
  bool isStatic = false;
  bool isConst = false;
  bool isAsync = false;
  bool isShared = false;
  bool isWeak = false;
  bool isExtern = false, isInterrupt = false, isNaked = false;
  bool isPacked = false;
  bool isNoReturn = false;
  bool isNoInline = false;
  bool isUsed = false;
  bool isInline = false;
  bool isPure = false;
  bool isCold = false;
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
    else if (consumeIf(TokenKind::KwInline))
      isInline = true;
    else if (consumeIf(TokenKind::KwPure))
      isPure = true;
    else if (consumeIf(TokenKind::KwCold))
      isCold = true;
    else if (consumeIf(TokenKind::KwPacked))
      isPacked = true;
    else if (consumeIf(TokenKind::KwAlign)) {
      expect(TokenKind::LParen);
      if (curTok.is(TokenKind::IntegerLiteral)) {
        try {
          alignment = std::stoull(curTok.getSpelling().str());
        } catch (const std::out_of_range &) {
          error("Alignment value is too large");
          alignment = 0;
        }
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
    std::vector<DeclPtr> ret;
    ret.push_back(
        std::make_unique<UsingDecl>(std::move(name), std::move(target), loc));
    return ret;
  }

  if (curTok.is(TokenKind::KwImport) || curTok.is(TokenKind::KwFrom)) {
    std::vector<DeclPtr> ret;
    if (auto d = parseImportDecl())
      ret.push_back(std::move(d));
    return ret;
  }
  if (curTok.is(TokenKind::KwMacro)) {
    std::vector<DeclPtr> ret;
    if (auto d = parseMacroDecl())
      ret.push_back(std::move(d));
    return ret;
  }
  if (curTok.is(TokenKind::KwGeneric)) {
    std::vector<DeclPtr> ret;
    if (auto d = parseGenericDecl())
      ret.push_back(std::move(d));
    return ret;
  }
  if (curTok.is(TokenKind::KwEnum)) {
    std::vector<DeclPtr> ret;
    if (auto d = parseEnumDecl())
      ret.push_back(std::move(d));
    return ret;
  }
  if (curTok.isAny(TokenKind::KwClass, TokenKind::KwStruct, TokenKind::KwRef,
                   TokenKind::KwUnion)) {
    std::vector<DeclPtr> ret;
    auto decl = parseClassDecl();
    if (auto cls = llvm::dyn_cast_or_null<ClassDecl>(decl.get())) {
      cls->setPacked(isPacked);
      cls->setAlignment(alignment);
      cls->setSection(sectionName);
    }
    if (decl)
      ret.push_back(std::move(decl));
    return ret;
  }

  // Variables & Functions
  TypePtr type = parseType();
  if (!type) {
    if (vis != Visibility::Default || isStatic || isConst || isAsync)
      error("Expected declaration after modifiers");
    return {};
  }

  bool isConstVar = type->isImmutable();
  bool isVolatileVar = type->is<VolatileType>();

  std::string name = curTok.getSpelling().str();
  expect(TokenKind::Identifier);

  std::vector<DeclPtr> decls;

  // Handle Function
  if (curTok.is(TokenKind::LParen)) {
    auto decl = parseFunctionRest(std::move(type), name, isAsync, isStatic,
                                  isWeak, vis);
    if (auto fn = llvm::dyn_cast_or_null<FunctionDecl>(decl.get())) {
      fn->setExtern(isExtern);
      fn->setExternLinkage(externLinkage);
      if (!externLinkage.empty())
        fn->setABI(externLinkage);
      fn->setInterrupt(isInterrupt);
      fn->setNaked(isNaked);
      fn->setNoReturn(isNoReturn);
      fn->setNoInline(isNoInline);
      fn->setInline(isInline);
      fn->setPure(isPure);
      fn->setCold(isCold);
      fn->setUsed(isUsed);
      fn->setSection(sectionName);
    }
    if (decl)
      decls.push_back(std::move(decl));
    return decls;
  }

  // Handle Variables
  while (true) {
    TypePtr clonedType = type->clone();
    bool isARCWeak = false;

    // Disambiguate Weak Linkage vs Weak ARC Reference
    if (isWeak) {
      if (clonedType->is<PointerType>() || clonedType->is<ReferenceType>() ||
          clonedType->is<NamedType>()) {
        isARCWeak = true;
        clonedType = std::make_unique<NullableType>(
            std::make_unique<WeakType>(std::move(clonedType),
                                       clonedType->getLoc()),
            clonedType->getLoc());
      }
    }

    auto decl = parseVariableRest(std::move(clonedType), name, isConstVar,
                                  isStatic, isShared, vis);

    if (auto var = llvm::dyn_cast_or_null<VariableDecl>(decl.get())) {
      var->setVolatile(isVolatileVar);
      var->setExtern(isExtern);
      if (isWeak && !isARCWeak) {
        var->setWeakVar(true);
      }
      var->setAlignment(alignment);
      var->setSection(sectionName);
      var->setUsed(isUsed);
      var->setThreadLocal(isThreadLocal);
    }

    if (decl)
      decls.push_back(std::move(decl));

    if (consumeIf(TokenKind::Comma)) {
      name = curTok.getSpelling().str();
      expect(TokenKind::Identifier);
    } else {
      // OPTIONAL SEMICOLON CHECK
      if (consumeIf(TokenKind::Semicolon)) {
        break; // Explicit Semicolon
      } else if (curTok.is(TokenKind::KwIn)) {
        break;
      } else if (hasNewlineBeforeToken(curTok) || curTok.is(TokenKind::Eof) ||
                 curTok.is(TokenKind::RBrace)) {
        break;
      } else {
        error("Expected ';' or newline after declaration");
        break;
      }
    }
  }
  return decls;
}

DeclPtr Parser::parseFunctionRest(TypePtr returnType, std::string name,
                                  bool isAsync, bool isStatic, bool isWeak,
                                  Visibility vis) {
  //  Capture location from the type or current token
  SourceLocation loc = returnType->getLoc();

  std::vector<FunctionDecl::Param> params;
  bool isVariadic = false;

  expect(TokenKind::LParen);
  if (curTok.isNot(TokenKind::RParen)) {
    do {
      //  Handle Variadic Argument
      if (consumeIf(TokenKind::DotDotDot)) {
        isVariadic = true;
        break;
      }

      TypePtr paramType = parseType();
      std::string paramName = "";
      SourceLocation paramLoc = curTok.location;
      ExprPtr defaultVal = nullptr;

      if (curTok.is(TokenKind::Identifier)) {
        paramName = curTok.getSpelling().str();
        consume();
      }

      // Check for default value assignment
      if (consumeIf(TokenKind::Equal)) {
        defaultVal = parseExpression();
      }

      params.push_back(FunctionDecl::Param{paramName, std::move(paramType),
                                           paramLoc, std::move(defaultVal)});

    } while (consumeIf(TokenKind::Comma));
  }
  expect(TokenKind::RParen);

  bool isViewMethod = false;
  while (curTok.isAny(TokenKind::KwView, TokenKind::KwMut)) {
    if (consumeIf(TokenKind::KwView)) {
      isViewMethod = true;
    } else if (consumeIf(TokenKind::KwMut)) {
      // Methods are mutable by default, but we safely consume 'mut' if provided
    }
  }

  StmtPtr body = nullptr;
  if (consumeIf(TokenKind::Semicolon)) {
    // Bodiless function (e.g. extern printf);
  } else {
    body = parseBlock();
  }

  auto funcDecl = std::make_unique<FunctionDecl>(
      name, std::move(params), std::move(returnType), std::move(body), isAsync,
      isStatic, isVariadic, isWeak, vis, loc);

  funcDecl->setViewMethod(isViewMethod);
  return funcDecl;
}

DeclPtr Parser::parseVariableRest(TypePtr type, std::string name, bool isConst,
                                  bool isStatic, bool isShared,
                                  Visibility vis) {
  SourceLocation loc = type->getLoc();
  ExprPtr init = nullptr;
  int bitFieldWidth = -1;

  if (consumeIf(TokenKind::Colon)) {
    if (curTok.is(TokenKind::IntegerLiteral)) {
      try {
        bitFieldWidth = std::stoi(curTok.getSpelling().str());
      } catch (const std::out_of_range &) {
        error("Bitfield width is too large");
        bitFieldWidth = -1;
      }
      consume();
    } else {
      error("Expected integer literal for bitfield width");
    }
  } else if (consumeIf(TokenKind::Equal)) {
    init = parseExpression();
    if (!init)
      return nullptr;
  }

  if (!init && type->is<NullableType>()) {
    init = std::make_unique<NullLiteral>(loc);
  }

  auto varDecl =
      std::make_unique<VariableDecl>(std::move(type), name, std::move(init),
                                     isConst, isStatic, isShared, vis, loc);

  if (bitFieldWidth != -1)
    varDecl->setBitfield(bitFieldWidth);
  else
    varDecl->setBitWidth(-1);

  return varDecl;
}

std::vector<DeclPtr> Parser::parseVariableDecls() {
  Visibility vis = Visibility::Default;
  bool isStatic = false, isShared = false, isExtern = false;
  bool isUsed = false, isThreadLocal = false;
  int alignment = 0;
  std::string sectionName = "";

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
        try {
          alignment = std::stoull(curTok.getSpelling().str());
        } catch (const std::out_of_range &) {
          error("Alignment value is too large");
          alignment = 0;
        }
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
      break;
    }
  }

  TypePtr type = parseType();
  if (!type)
    return {};

  bool isConstVar = type->isImmutable();
  bool isVolatileVar = type->is<VolatileType>();

  std::vector<DeclPtr> decls;

  while (true) {
    std::string name = curTok.getSpelling().str();
    expect(TokenKind::Identifier);

    TypePtr clonedType = type->clone();
    auto decl = parseVariableRest(std::move(clonedType), name, isConstVar,
                                  isStatic, isShared, vis);

    if (auto var = llvm::dyn_cast_or_null<VariableDecl>(decl.get())) {
      var->setVolatile(isVolatileVar);
      var->setExtern(isExtern);
      var->setAlignment(alignment);
      var->setSection(sectionName);
      var->setUsed(isUsed);
      var->setThreadLocal(isThreadLocal);
    }

    if (decl)
      decls.push_back(std::move(decl));

    if (consumeIf(TokenKind::Comma)) {
      // Allow inline re-typing for mixed declarations (e.g., int idx, int item)
      if (isStartOfDeclaration()) {
        if (TypePtr nextType = parseType()) {
          type = std::move(nextType);
          isConstVar = type->isImmutable();
          isVolatileVar = type->is<VolatileType>();
        }
      }
      continue;
    } else {
      // OPTIONAL SEMICOLON CHECK
      if (consumeIf(TokenKind::Semicolon)) {
        break; // Explicit Semicolon
      } else if (curTok.is(TokenKind::KwIn)) {
        break;
      } else if (hasNewlineBeforeToken(curTok) || curTok.is(TokenKind::Eof) ||
                 curTok.is(TokenKind::RBrace)) {
        break; // Implicit Semicolon
      } else {
        error("Expected ';' or newline after declaration");
        break;
      }
    }
  }
  return decls;
}

DeclPtr Parser::parseImportDecl() {
  SourceLocation loc = curTok.location;
  std::string moduleName;
  std::string aliasName = "";
  std::vector<std::string> symbols;

  /** @brief Helper to strip quotes from a string literal */
  auto stripQuotes = [](std::string s) {
    if (s.size() >= 2 &&
        (s.front() == '"' || s.front() == '`' || s.front() == '\''))
      return s.substr(1, s.size() - 2);
    return s;
  };

  // Pattern 1: from "std/collections" import { table, list }
  if (consumeIf(TokenKind::KwFrom)) {
    if (curTok.is(TokenKind::StringLiteral) ||
        curTok.is(TokenKind::Identifier)) {
      moduleName = stripQuotes(curTok.getSpelling().str());
      consume();
    } else {
      error("Expected string literal or identifier after 'from'");
    }
    expect(TokenKind::KwImport);
    expect(TokenKind::LBrace);
    parseImportSymbolList(symbols);
  } else if (consumeIf(TokenKind::KwImport)) {
    // Pattern 2: import { table, list } from "std/collections"
    if (curTok.is(TokenKind::LBrace)) {
      consume();
      parseImportSymbolList(symbols);
      expect(TokenKind::KwFrom);
      if (curTok.is(TokenKind::StringLiteral) ||
          curTok.is(TokenKind::Identifier)) {
        moduleName = stripQuotes(curTok.getSpelling().str());
        consume();
      } else {
        error("Expected string literal or identifier after 'from'");
      }
    }
    // Pattern 3 & 4: import "std/io" OR import test [as t]
    else if (curTok.is(TokenKind::StringLiteral) ||
             curTok.is(TokenKind::Identifier)) {
      moduleName = stripQuotes(curTok.getSpelling().str());
      consume();

      // Check for the 'as' alias syntax
      if (consumeIf(TokenKind::KwAs)) {
        if (curTok.is(TokenKind::Identifier)) {
          aliasName = curTok.getSpelling().str();
          consume();
        } else {
          error("Expected identifier after 'as'");
        }
      }
    } else {
      error("Expected '{', identifier, or string literal after 'import'");
    }
  }

  consumeIf(TokenKind::Semicolon);

  return std::make_unique<ImportDecl>(moduleName, aliasName, symbols, loc);
}

DeclPtr Parser::parseGenericDecl() {
  SourceLocation loc = curTok.location;
  expect(TokenKind::KwGeneric);

  std::string name = "";

  // Check for Syntax: generic Box<T>
  if (curTok.is(TokenKind::Identifier)) {
    name = curTok.getSpelling().str();
    consume();
  }

  std::vector<GenericDecl::GenericParam> typeParams;

  // Parse Type Parameters: <T, U>
  if (consumeIf(TokenKind::Less)) {
    do {
      bool isShared = consumeIf(TokenKind::KwShared);
      if (curTok.is(TokenKind::Identifier)) {
        typeParams.push_back(
            {curTok.getSpelling().str(), isShared, curTok.location});
        consume();
      } else {
        error("Expected type parameter name");
      }
    } while (consumeIf(TokenKind::Comma));
    expectGreater();
  }

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

      if (curTok.isAny(TokenKind::KwConstructor, TokenKind::KwDestructor)) {
        TokenKind kind = curTok.kind;
        std::string memName =
            (kind == TokenKind::KwConstructor) ? "constructor" : "destructor";
        SourceLocation mLoc = curTok.location;
        consume();

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

      bool isConstVar = memberType->isImmutable();
      bool isVolatileVar = memberType->is<VolatileType>();

      std::string memName = curTok.getSpelling().str();
      expect(TokenKind::Identifier);

      if (curTok.is(TokenKind::LParen)) {
        members.push_back(parseFunctionRest(std::move(memberType), memName,
                                            isAsync, isStatic, isWeak, memVis));
      } else {
        if (isWeak) {
          memberType = std::make_unique<NullableType>(
              std::make_unique<WeakType>(std::move(memberType),
                                         memberType->getLoc()),
              memberType->getLoc());
        }
        auto varDecl =
            parseVariableRest(std::move(memberType), memName, isConstVar,
                              isStatic, isShared, memVis);
        if (auto var = llvm::dyn_cast_or_null<VariableDecl>(varDecl.get())) {
          var->setVolatile(isVolatileVar);
        }
        members.push_back(std::move(varDecl));
      }
    }
    expect(TokenKind::RBrace);

    auto classDecl = std::make_unique<ClassDecl>(
        name, parentNames, std::move(members), false, AggregateKind::Class,
        Visibility::Default, loc);
    return std::make_unique<GenericDecl>(name, std::move(typeParams),
                                         std::move(classDecl), loc);
  }

  auto innerDecls = parseTopLevelDecls();
  if (innerDecls.empty() || !innerDecls[0])
    return nullptr;

  return std::make_unique<GenericDecl>(innerDecls[0]->getName(),
                                       std::move(typeParams),
                                       std::move(innerDecls[0]), loc);
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
        return nullptr;
      }
    } while (consumeIf(TokenKind::Comma));

    if (!expect(TokenKind::RParen))
      return nullptr;
  }

  if (!expect(TokenKind::LBrace))
    return nullptr;

  std::vector<DeclPtr> members;

  while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
    Visibility memVis = Visibility::Default;
    bool isStatic = false;
    bool isAsync = false;
    bool isShared = false;
    bool isWeak = false;
    int alignment = 0;
    std::string sectionName = "";
    bool isThreadLocal = false;
    bool isVirtual = false;
    bool isOverride = false;
    bool isUsed = false;
    bool isNoInline = false;
    bool isInline = false;
    bool isPure = false;
    bool isCold = false;
    bool isNaked = false;
    bool isNoReturn = false;

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
          try {
            alignment = std::stoull(curTok.getSpelling().str());
          } catch (const std::out_of_range &) {
            error("Alignment value is too large");
            alignment = 0; // Safe fallback
          }
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
      } else if (consumeIf(TokenKind::KwVirtual)) {
        isVirtual = true;
      } else if (consumeIf(TokenKind::KwOverride)) {
        isOverride = true;
      } else if (consumeIf(TokenKind::KwUsed))
        isUsed = true;
      else if (consumeIf(TokenKind::KwNoinline))
        isNoInline = true;
      else if (consumeIf(TokenKind::KwInline))
        isInline = true;
      else if (consumeIf(TokenKind::KwPure))
        isPure = true;
      else if (consumeIf(TokenKind::KwCold))
        isCold = true;
      else
        break;
    }

    if (curTok.isAny(TokenKind::KwConstructor, TokenKind::KwDestructor)) {
      TokenKind kind = curTok.kind;
      std::string memName =
          (kind == TokenKind::KwConstructor) ? "constructor" : "destructor";
      SourceLocation mLoc = curTok.location;
      consume();

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

    bool isConstVar = memberType->isImmutable();
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
      if (auto fn = llvm::dyn_cast_or_null<FunctionDecl>(fnDecl.get())) {
        fn->setSection(sectionName);
        fn->setVirtual(isVirtual);
        fn->setOverride(isOverride);
        fn->setNaked(isNaked);
        fn->setNoReturn(isNoReturn);
        fn->setNoInline(isNoInline);
        fn->setInline(isInline);
        fn->setPure(isPure);
        fn->setCold(isCold);
        fn->setUsed(isUsed);
      }
      members.push_back(std::move(fnDecl));
    } else {
      if (isVirtual || isOverride) {
        error("Variables cannot be marked 'virtual' or 'override'");
      }
      if (isAsync)
        error("'async' on fields not supported");

      TypePtr baseType = memberType->clone();

      if (isWeak) {
        memberType = std::make_unique<NullableType>(
            std::make_unique<WeakType>(std::move(memberType),
                                       memberType->getLoc()),
            memberType->getLoc());
      }
      auto varDecl = parseVariableRest(std::move(memberType), memName,
                                       isConstVar, isStatic, isShared, memVis);
      if (auto var = llvm::dyn_cast_or_null<VariableDecl>(varDecl.get())) {
        var->setVolatile(isVolatileVar);
        var->setAlignment(alignment);
        var->setSection(sectionName);
        var->setThreadLocal(isThreadLocal);
      }
      members.push_back(std::move(varDecl));

      while (consumeIf(TokenKind::Comma)) {
        memName = curTok.getSpelling().str();
        expect(TokenKind::Identifier);

        TypePtr nextType = baseType->clone();
        if (isWeak) {
          nextType = std::make_unique<NullableType>(
              std::make_unique<WeakType>(std::move(nextType),
                                         nextType->getLoc()),
              nextType->getLoc());
        }

        auto nextVarDecl =
            parseVariableRest(std::move(nextType), memName, isConstVar,
                              isStatic, isShared, memVis);
        if (auto var =
                llvm::dyn_cast_or_null<VariableDecl>(nextVarDecl.get())) {
          var->setVolatile(isVolatileVar);
          var->setAlignment(alignment);
          var->setSection(sectionName);
          var->setThreadLocal(isThreadLocal);
        }
        members.push_back(std::move(nextVarDecl));
      }
      consumeIf(TokenKind::Semicolon);
    }
  }
  expect(TokenKind::RBrace);

  return std::make_unique<ClassDecl>(name, parentNames, std::move(members),
                                     isRef, aggKind, Visibility::Default, loc);
}

DeclPtr Parser::parseEnumDecl() {
  SourceLocation loc = curTok.location;
  consume();

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
  consume();

  std::string name = curTok.getSpelling().str();
  if (!expect(TokenKind::Identifier))
    return nullptr;

  bool isFunctionMacro = false;
  std::vector<std::string> params;

  if (consumeIf(TokenKind::LParen)) {
    isFunctionMacro = true;
    if (curTok.isNot(TokenKind::RParen)) {
      do {
        if (curTok.is(TokenKind::Identifier)) {
          params.push_back(curTok.getSpelling().str());
          consume();
        } else {
          error("Expected identifier in macro parameters");
          while (curTok.isNot(TokenKind::Comma) &&
                 curTok.isNot(TokenKind::RParen) &&
                 curTok.isNot(TokenKind::Eof)) {
            consume();
          }
        }
      } while (consumeIf(TokenKind::Comma));
    }

    // If we can't find a closing parenthesis, gracefully jump to the body
    if (!expect(TokenKind::RParen)) {
      while (curTok.isNot(TokenKind::LBrace) && curTok.isNot(TokenKind::Eof)) {
        consume();
      }
    }
  }

  std::vector<StmtPtr> body;
  if (curTok.is(TokenKind::LBrace)) {
    consume();
    while (curTok.isNot(TokenKind::RBrace) && curTok.isNot(TokenKind::Eof)) {
      if (auto s = parseStatement())
        body.push_back(std::move(s));
      else
        synchronize();
    }
    expect(TokenKind::RBrace);
  } else {
    auto expr = parseExpression();
    if (expr) {
      body.push_back(std::make_unique<ExpressionStmt>(std::move(expr), loc));
    }
  }

  return std::make_unique<MacroDecl>(name, std::move(params), std::move(body),
                                     isFunctionMacro, loc);
}

// Statements

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
    if (isStartOfDeclaration()) {
      SourceLocation loc = curTok.location;
      auto decls = parseTopLevelDecls();
      if (decls.size() == 1) {
        return std::make_unique<DeclStmt>(std::move(decls[0]), loc);
      } else {
        std::vector<StmtPtr> blockStmts;
        for (auto &d : decls)
          blockStmts.push_back(
              std::make_unique<DeclStmt>(std::move(d), d->getLoc()));
        return std::make_unique<BlockStmt>(std::move(blockStmts), loc);
      }
    }
    return nullptr;

  case TokenKind::KwView:
  case TokenKind::KwMut:
    if (nextTok.is(TokenKind::LBrace)) {
      return parseLockStmt();
    }
    if (isStartOfDeclaration()) {
      SourceLocation loc = curTok.location;
      auto decls = parseTopLevelDecls();
      if (decls.size() == 1) {
        return std::make_unique<DeclStmt>(std::move(decls[0]), loc);
      } else {
        std::vector<StmtPtr> blockStmts;
        for (auto &d : decls)
          blockStmts.push_back(
              std::make_unique<DeclStmt>(std::move(d), d->getLoc()));
        return std::make_unique<BlockStmt>(std::move(blockStmts), loc);
      }
    }
    return nullptr;
  case TokenKind::KwAsync:
    if (nextTok.is(TokenKind::KwLock)) {
      return parseLockStmt();
    }
    [[fallthrough]];
  default: {
    // Variable Declaration
    if (isStartOfDeclaration()) {
      SourceLocation loc = curTok.location;
      auto decls = parseTopLevelDecls();
      if (decls.size() == 1) {
        return std::make_unique<DeclStmt>(std::move(decls[0]), loc);
      } else {
        std::vector<StmtPtr> blockStmts;
        for (auto &d : decls)
          blockStmts.push_back(
              std::make_unique<DeclStmt>(std::move(d), d->getLoc()));
        return std::make_unique<BlockStmt>(std::move(blockStmts), loc);
      }
    }
    // Ambiguity Check
    if (curTok.is(TokenKind::Identifier) && peekIs(TokenKind::Identifier)) {
      SourceLocation loc = curTok.location;
      auto decls = parseVariableDecls();
      if (decls.size() == 1) {
        return std::make_unique<DeclStmt>(std::move(decls[0]), loc);
      } else {
        std::vector<StmtPtr> blockStmts;
        for (auto &d : decls)
          blockStmts.push_back(
              std::make_unique<DeclStmt>(std::move(d), d->getLoc()));
        return std::make_unique<BlockStmt>(std::move(blockStmts), loc);
      }
    }
    // Expression Statement
    SourceLocation loc = curTok.location;
    ExprPtr expr = parseExpression();
    if (!expr)
      return nullptr;

    // Optional semicolon for expressions
    if (!consumeIf(TokenKind::Semicolon) && !hasNewlineBeforeToken(curTok) &&
        curTok.isNot(TokenKind::Eof) && curTok.isNot(TokenKind::RBrace)) {
      error("Expected ';' or newline after expression");
    }
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
  consume();
  expect(TokenKind::LParen);

  if (curTok.is(TokenKind::Identifier) &&
      (nextTok.is(TokenKind::KwIn) || nextTok.is(TokenKind::Comma))) {

    SourceLocation varLoc = curTok.location;
    std::string name1 = curTok.getSpelling().str();
    consume();

    std::string name2 = "";
    if (consumeIf(TokenKind::Comma)) {
      if (curTok.is(TokenKind::Identifier)) {
        name2 = curTok.getSpelling().str();
        consume();
      } else {
        error("Expected second identifier in for-in loop");
      }
    }

    if (consumeIf(TokenKind::KwIn)) {
      ExprPtr collection = parseExpression();
      expect(TokenKind::RParen);

      DeclPtr finalIndex = nullptr;
      DeclPtr finalVar = nullptr;

      auto type1 = std::make_unique<AnyType>(varLoc);
      if (!name2.empty()) {
        auto type2 = std::make_unique<AnyType>(varLoc);
        finalIndex = std::make_unique<VariableDecl>(
            std::move(type1), name1, nullptr, false, false, false,
            Visibility::Default, varLoc);
        finalVar = std::make_unique<VariableDecl>(std::move(type2), name2,
                                                  nullptr, false, false, false,
                                                  Visibility::Default, varLoc);
      } else {
        finalVar = std::make_unique<VariableDecl>(std::move(type1), name1,
                                                  nullptr, false, false, false,
                                                  Visibility::Default, varLoc);
      }

      loopDepth++;
      StmtPtr body = parseStatement();
      loopDepth--;

      return std::make_unique<ForInStmt>(
          std::move(finalVar), std::move(finalIndex), std::move(collection),
          std::move(body), loc);
    } else {
      error("Expected 'in' in for-in loop");
    }
  }

  StmtPtr init = nullptr;
  bool isTypedDeclaration = isStartOfDeclaration();

  if (isTypedDeclaration) {
    auto decls = parseVariableDecls();

    // Check if this is a Typed For-In loop: for (int x in arr)
    if ((decls.size() == 1 || decls.size() == 2) &&
        consumeIf(TokenKind::KwIn)) {
      DeclPtr finalVar = nullptr;
      DeclPtr finalIndex = nullptr;

      if (decls.size() == 2) {
        finalIndex = std::move(decls[0]);
        finalVar = std::move(decls[1]);
      } else {
        finalVar = std::move(decls[0]);
      }

      ExprPtr collection = parseExpression();
      expect(TokenKind::RParen);

      loopDepth++;
      StmtPtr body = parseStatement();
      loopDepth--;

      return std::make_unique<ForInStmt>(
          std::move(finalVar), std::move(finalIndex), std::move(collection),
          std::move(body), loc);
    }

    // Standard For Loop init
    if (!decls.empty()) {
      if (decls.size() == 1) {
        init = std::make_unique<DeclStmt>(std::move(decls[0]), loc);
      } else {
        std::vector<StmtPtr> blockStmts;
        for (auto &d : decls) {
          blockStmts.push_back(
              std::make_unique<DeclStmt>(std::move(d), d->getLoc()));
        }
        init = std::make_unique<BlockStmt>(std::move(blockStmts), loc);
      }
    }
  } else {
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

  loopDepth++;
  StmtPtr body = parseStatement();
  loopDepth--;

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
  loopDepth++;
  std::vector<SwitchCase> cases;
  while (consumeIf(TokenKind::KwCase)) {
    std::vector<ExprPtr> values;
    do {
      auto val = parseExpression();

      if (curTok.is(TokenKind::Colon) &&
          nextTok.isAny(TokenKind::IntegerLiteral, TokenKind::FloatLiteral,
                        TokenKind::StringLiteral, TokenKind::CharLiteral)) {
        consume();
        auto endVal = parseExpression();
        val = std::make_unique<BinaryExpr>(std::move(val), TokenKind::Colon,
                                           std::move(endVal), curTok.location);
      }

      values.push_back(std::move(val));
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

  std::vector<CatchClause> catches;

  while (consumeIf(TokenKind::KwCatch)) {
    SourceLocation catchLoc = curTok.location;
    DeclPtr catchVar = nullptr;

    expect(TokenKind::LParen);
    TypePtr type = parseType();
    if (curTok.is(TokenKind::Identifier)) {
      std::string name = curTok.getSpelling().str();
      consume();
      catchVar = std::make_unique<VariableDecl>(std::move(type), name, nullptr,
                                                curTok.location);
    }
    expect(TokenKind::RParen);

    StmtPtr catchBlock = parseBlock();
    catches.emplace_back(std::move(catchVar), std::move(catchBlock), catchLoc);
  }

  StmtPtr finallyBlock = nullptr;
  if (consumeIf(TokenKind::KwFinally)) {
    finallyBlock = parseBlock();
  }

  return std::make_unique<TryCatchStmt>(std::move(tryBlock), std::move(catches),
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
  bool isAsync = false;
  if (consumeIf(TokenKind::KwAsync)) {
    isAsync = true;
  }
  consume();
  ExprPtr target = nullptr;
  if (consumeIf(TokenKind::LParen)) {
    target = parseExpression();
    expect(TokenKind::RParen);
  }
  auto body = parseBlock();
  return std::make_unique<LockStmt>(std::move(target), std::move(body), isAsync,
                                    loc);
}

StmtPtr Parser::parseThrowStmt() {
  SourceLocation loc = curTok.location;
  consume();
  auto expr = parseExpression();
  consumeIf(TokenKind::Semicolon);
  return std::make_unique<ThrowStmt>(std::move(expr), loc);
}

// Expressions

ExprPtr Parser::parseExpression() { return parseAssignment(); }

ExprPtr Parser::parseAssignment() {
  auto left = parseTernary();
  if (!left)
    return nullptr;

  if (curTok.isAny(
          TokenKind::Equal, TokenKind::PlusEqual, TokenKind::MinusEqual,
          TokenKind::StarEqual, TokenKind::SlashEqual, TokenKind::PercentEqual,
          TokenKind::AmpEqual, TokenKind::PipeEqual, TokenKind::CaretEqual,
          TokenKind::LessLessEqual, TokenKind::GreaterGreaterEqual)) {
    TokenKind op = curTok.kind;
    SourceLocation opLoc = curTok.location;
    consume();
    auto right = parseAssignment();
    if (!right)
      return nullptr;
    return std::make_unique<BinaryExpr>(std::move(left), op, std::move(right),
                                        opLoc);
  }
  return left;
}

ExprPtr Parser::parsePipe() {
  auto left = parseNullCoalescing();
  if (!left)
    return nullptr;

  while (curTok.is(TokenKind::PipeGreater)) {
    SourceLocation opLoc = curTok.location;
    consume();

    auto right = parsePostfix();
    if (!right)
      return nullptr;

    Expr *target = right.get();
    while (true) {
      if (auto *mem = llvm::dyn_cast_or_null<MemberExpr>(target))
        target = const_cast<Expr *>(mem->getObject());
      else if (auto *idx = llvm::dyn_cast_or_null<IndexExpr>(target))
        target = const_cast<Expr *>(idx->getArray());
      else
        break;
    }

    if (auto *call = llvm::dyn_cast_or_null<CallExpr>(target)) {
      call->insertFirstArg(std::move(left));
      left = std::move(right);
    } else if (llvm::dyn_cast_or_null<IdentifierExpr>(right.get())) {
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
    if (!trueBranch)
      return nullptr;
    if (!expect(TokenKind::Colon))
      return nullptr;
    auto falseBranch = parseTernary();
    if (!falseBranch)
      return nullptr;

    SourceLocation loc = cond->getLoc();
    return std::make_unique<TernaryExpr>(std::move(cond), std::move(trueBranch),
                                         std::move(falseBranch), loc);
  }
  return cond;
}

ExprPtr Parser::parseNullCoalescing() {
  auto left = parseShift();
  while (curTok.is(TokenKind::QuestionQuestion)) {
    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location;
    consume();
    auto right = parseShift();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(
        std::move(left), TokenKind::QuestionQuestion, std::move(right), opLoc);
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
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), TokenKind::PipePipe,
                                        std::move(right), opLoc);
  }
  return left;
}

ExprPtr Parser::parseLogicalAnd() {
  auto left = parseBitwiseOr();
  while (curTok.is(TokenKind::AmpAmp)) {
    if (!left)
      return nullptr;
    SourceLocation opLoc = curTok.location;
    consume();
    auto right = parseBitwiseOr();
    if (!right)
      return nullptr;
    left = std::make_unique<BinaryExpr>(std::move(left), TokenKind::AmpAmp,
                                        std::move(right), opLoc);
  }
  return left;
}

ExprPtr Parser::parseBitwiseOr() {
  auto left = parseBitwiseXor();
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
    SourceLocation opLoc = curTok.location;
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
    SourceLocation opLoc = curTok.location;
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
    SourceLocation opLoc = curTok.location;
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
    consume();

    auto right = parsePower();
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
    auto operand = parsePrefix();
    return std::make_unique<AwaitExpr>(std::move(operand), loc);
  }

  if (curTok.is(TokenKind::Amp)) {
    bool isClosureCapture =
        nextTok.is(TokenKind::LParen) || nextTok.is(TokenKind::KwMut) ||
        (nextTok.is(TokenKind::Identifier) && nextTok.getSpelling() == "mut");

    if (isClosureCapture) {
      return parsePostfix();
    }
  }

  if (curTok.is(TokenKind::Power)) {
    SourceLocation loc = curTok.location;
    consume();
    auto operand = parsePrefix();
    if (!operand)
      return nullptr;

    auto inner = std::make_unique<UnaryExpr>(TokenKind::Star,
                                             std::move(operand), false, loc);
    return std::make_unique<UnaryExpr>(TokenKind::Star, std::move(inner), false,
                                       loc);
  }

  if (curTok.isAny(TokenKind::Bang, TokenKind::Minus, TokenKind::Tilde,
                   TokenKind::PlusPlus, TokenKind::MinusMinus,
                   TokenKind::DotDotDot, TokenKind::Amp, TokenKind::Star,
                   TokenKind::KwShared)) {
    TokenKind op = curTok.kind;
    SourceLocation loc = curTok.location;
    consume();
    auto operand = parsePrefix();
    if (!operand)
      return nullptr;
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
      consume();

      std::string name = curTok.getSpelling().str();
      expect(TokenKind::Identifier);
      left = std::make_unique<MemberExpr>(std::move(left), name, isOptional,
                                          left->getLoc());
    } else if (consumeIf(TokenKind::PlusPlus)) {
      left = std::make_unique<UnaryExpr>(TokenKind::PlusPlus, std::move(left),
                                         true, left->getLoc());
    } else if (consumeIf(TokenKind::MinusMinus)) {
      left = std::make_unique<UnaryExpr>(TokenKind::MinusMinus, std::move(left),
                                         true, left->getLoc());
    } else if (curTok.is(TokenKind::LBracket) ||
               (curTok.is(TokenKind::Question) &&
                nextTok.is(TokenKind::LBracket))) {

      bool isOptional = false;
      if (curTok.is(TokenKind::Question)) {
        isOptional = true;
        consume();
      }

      if (hasNewlineBeforeToken(curTok)) {
        break;
      }
      consume();

      auto index = parseExpression();
      expect(TokenKind::RBracket);
      left = std::make_unique<IndexExpr>(std::move(left), std::move(index),
                                         isOptional, left->getLoc());
    } else {
      break;
    }
  }
  return left;
}

ExprPtr Parser::parsePrimary() {
  SourceLocation loc = curTok.location;

  // Intercept Closure Capture Modifiers
  CaptureMode capMode = CaptureMode::Snapshot; // Default is value copy
  bool hasCaptureModifier = false;

  // Safely handle 'move'
  if (curTok.is(TokenKind::KwMove) ||
      (curTok.is(TokenKind::Identifier) && curTok.getSpelling() == "move")) {
    advance();
    capMode = CaptureMode::Move;
    hasCaptureModifier = true;
  }
  // Handle '&' (View) and '&mut' (Mut)
  else if (curTok.is(TokenKind::Amp)) {
    advance(); // consume '&'
    if (curTok.is(TokenKind::KwMut)) {
      advance(); // consume 'mut'
      capMode = CaptureMode::Mut;
    } else {
      capMode = CaptureMode::View;
    }
    hasCaptureModifier = true;
  }

  if (hasCaptureModifier && curTok.isNot(TokenKind::LParen)) {
    Diags.report(curTok.location, DiagID::err_expected_token)
        << "'(' for lambda parameter list after capture modifier";
    return nullptr;
  }

  switch (curTok.kind) {
  case TokenKind::KwAsync: {
    consume();

    if (curTok.is(TokenKind::LParen)) {
      auto expr = parsePrimary();
      if (expr && expr->getKind() == ExprKind::LambdaExpr) {
        static_cast<LambdaExpr *>(expr.get())->setAsync(true);
        return expr;
      } else if (expr) {
        error("Expected lambda expression after 'async'");
        return expr;
      }
    }
    error("Expected '(' for async lambda expression");
    return nullptr;
  }
  case TokenKind::KwInput:
    return parseInputExpr();
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
  case TokenKind::KwBitcast: {
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
    return std::make_unique<BitcastExpr>(std::move(type), std::move(expr), loc);
  }
  case TokenKind::IntegerLiteral: {
    std::string text = curTok.getSpelling().str();

    // Strip underscores so '1_000_000' becomes '1000000'
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());

    uint64_t val = 0;
    bool isHexBinOct = false;
    try {
      if (text.size() >= 2 && text[0] == '0' &&
          (text[1] == 'b' || text[1] == 'B')) {
        val = std::stoull(text.substr(2), nullptr, 2);
        isHexBinOct = true;
      } else if (text.size() >= 2 && text[0] == '0' &&
                 (text[1] == 'o' || text[1] == 'O')) {
        val = std::stoull(text.substr(2), nullptr, 8);
        isHexBinOct = true;
      } else {
        if (text.size() >= 2 && text[0] == '0' &&
            (text[1] == 'x' || text[1] == 'X')) {
          isHexBinOct = true;
        }
        // Base 0 automatically handles hex (0x) and decimal
        val = std::stoull(text, nullptr, 0);
      }

      if (!isHexBinOct && curTok.suffix == NumericSuffix::None) {
        if (val > 9223372036854775808ULL) {
          Diags.report(curTok.location, DiagID::err_type_mismatch)
              << "Integer literal '" << text << "' is out of range for i64";
        }
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

    // Strip underscores for floats too (e.g., 1_000.50)
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
  case TokenKind::DecimalLiteral: {
    std::string text = curTok.getSpelling().str();

    // Strip underscores for decimals (e.g., 1_000.50d)
    text.erase(std::remove(text.begin(), text.end(), '_'), text.end());

    SourceLocation loc = curTok.location;
    consume();
    return std::make_unique<DecimalLiteral>(std::move(text), loc);
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
  case TokenKind::KwAny:
  case TokenKind::KwPromise: {
    std::string name = curTok.getSpelling().str();
    consume();
    return std::make_unique<IdentifierExpr>(name, loc);
  }
  case TokenKind::LParen: {
    consume();

    // 1. Empty Lambda: () => { ... }
    if (curTok.is(TokenKind::RParen)) {
      consume();
      if (consumeIf(TokenKind::FatArrow)) {
        return parseLambdaBody({}, capMode);
      }
      error("Expected expression after empty parentheses");
      return nullptr;
    }

    // 2. Typed Lambda: (int x, ...) => { ... } OR (Dog d, ...) => { ... }
    bool isTypedLambda = false;

    // Check for built-in primitive types
    if (curTok.isAny(TokenKind::KwInt, TokenKind::KwFloat, TokenKind::KwBool,
                     TokenKind::KwString, TokenKind::KwVoid, TokenKind::KwAny,
                     TokenKind::KwChar, TokenKind::KwDouble, TokenKind::KwHalf,
                     TokenKind::KwQuarter, TokenKind::KwShort,
                     TokenKind::KwLong, TokenKind::KwUSize,
                     TokenKind::KwISize)) {
      if (nextTok.is(TokenKind::Identifier)) {
        isTypedLambda = true;
      }
    }
    // Check for custom types: 'Dog d' (Ident+Ident), 'Box<T>' (Ident+<), or
    // 'Dog?' (Ident+?)
    else if (curTok.is(TokenKind::Identifier)) {
      if (nextTok.is(TokenKind::Identifier) ||
          nextTok.is(TokenKind::Question)) {
        isTypedLambda = true;
      } else if (nextTok.is(TokenKind::Less)) {
        const char *ptr = nextTok.getSpelling().data();
        int depth = 0;
        while (*ptr != '\0' && *ptr != ')' && *ptr != '\n') {
          if (*ptr == '<')
            depth++;
          else if (*ptr == '>') {
            depth--;
            if (depth == 0) {
              ptr++;
              while (*ptr == ' ' || *ptr == '\t' || *ptr == '\r' ||
                     *ptr == '\n')
                ptr++;
              if ((*ptr >= 'a' && *ptr <= 'z') ||
                  (*ptr >= 'A' && *ptr <= 'Z') || *ptr == '_') {
                isTypedLambda = true;
              }
              break;
            }
          }
          ptr++;
        }
      }
    }

    if (isTypedLambda) {
      std::vector<LambdaParam> params;
      do {
        TypePtr t = parseType();
        if (!t)
          break;

        std::string n;
        if (curTok.is(TokenKind::Identifier)) {
          n = curTok.getSpelling().str();
          consume();
        } else {
          error("Expected parameter name");
        }

        ExprPtr defVal = nullptr;
        if (consumeIf(TokenKind::Equal)) {
          defVal = parseExpression();
        }

        params.emplace_back(std::move(t), std::move(n), std::move(defVal));
      } while (consumeIf(TokenKind::Comma));

      expect(TokenKind::RParen);

      if (consumeIf(TokenKind::FatArrow)) {
        return parseLambdaBody(std::move(params), capMode);
      }
      error("Expected '=>' after typed lambda parameters");
      return nullptr;
    }

    auto expr = parseExpression();

    // TRAP: Catch JS/C#-style untyped lambdas like (x => ...) or (x, y => ...)
    if (curTok.is(TokenKind::FatArrow) || curTok.is(TokenKind::Comma)) {
      error("Invalid lambda syntax. Moksha requires typed parameters (e.g., "
            "'(int x) => { ... }')");

      while (curTok.isNot(TokenKind::RParen) && curTok.isNot(TokenKind::Eof)) {
        consume();
      }
      consumeIf(TokenKind::RParen);
      return nullptr;
    }

    expect(TokenKind::RParen);

    if (hasCaptureModifier) {
      Diags.report(loc, DiagID::err_expected_token)
          << "closure capture modifier can only be applied to lambda "
             "expressions";
      return nullptr;
    }
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
  case TokenKind::KwAsm:
    return parseAsmExpr();
  case TokenKind::Error:
    consume();
    return nullptr;
  case TokenKind::KwSizeof: {
    consume();
    expect(TokenKind::LParen);
    auto expr = parseExpression();
    expect(TokenKind::RParen);
    return std::make_unique<SizeOfExpr>(std::move(expr), loc);
  }
  default:
    error("Expected expression");
    return nullptr;
  }
}

ExprPtr Parser::parseArrayLiteral() {
  SourceLocation loc = curTok.location;
  consume();
  std::vector<ExprPtr> elements;
  while (curTok.isNot(TokenKind::RBracket) && curTok.isNot(TokenKind::Eof)) {
    if (curTok.is(TokenKind::DotDotDot)) {
      consume();
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
  consume();

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
      val = val.substr(1, val.size() - 2);

    val = unescapeString(val);

    consume();
    return std::make_unique<StringLiteral>(std::move(val), true, loc);
  }

  // Complex case loop
  while (true) {
    if (curTok.is(TokenKind::StringFragment)) {
      std::string val = curTok.getSpelling().str();

      val = unescapeString(val);

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

      val = unescapeString(val);

      parts.push_back(std::make_unique<StringLiteral>(std::move(val), true,
                                                      curTok.location));
      consume();
      break;
    } else {
      error("Malformed template string");
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

  val = unescapeString(val);
  consume();
  return std::make_unique<StringLiteral>(std::move(val), false, loc);
}

ExprPtr Parser::parseLambdaBody(std::vector<LambdaParam> params,
                                CaptureMode mode) {
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
                                      isExprBody, mode, loc);
}

ExprPtr Parser::parseNewExpr() {
  SourceLocation loc = curTok.location;
  consume();

  bool isWeak = false;
  if (consumeIf(TokenKind::KwWeak)) {
    isWeak = true;
  }

  // Case 1: Thread Creation
  if (consumeIf(TokenKind::KwThread)) {
    // Match the syntax: Thread(() => { ... })
    if (!expect(TokenKind::LParen))
      return nullptr;

    auto taskExpr = parseExpression();
    if (!taskExpr)
      return nullptr;

    if (taskExpr->getKind() == ExprKind::LambdaExpr) {
      auto lambda = std::unique_ptr<LambdaExpr>(
          static_cast<LambdaExpr *>(taskExpr.release()));

      if (!expect(TokenKind::RParen))
        return nullptr;

      return std::make_unique<ThreadExpr>(isWeak, std::move(lambda), loc);
    } else {
      error("Thread instantiation requires a closure/lambda expression.");
      return nullptr;
    }
  }

  // Case 2: Standard Object Creation
  TypePtr type = parseType();
  if (!type)
    return nullptr;

  if (isWeak) {
    type = std::make_unique<WeakType>(std::move(type), loc);
  }

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

ExprPtr Parser::parseInputExpr() {
  SourceLocation loc = curTok.location;
  consume();
  expect(TokenKind::LParen);

  ExprPtr prompt = parseExpression();
  if (!prompt) {
    error("input() requires a mandatory prompt string argument");
  }

  expect(TokenKind::RParen);
  return std::make_unique<InputExpr>(std::move(prompt), loc);
}

ExprPtr Parser::parseAsmExpr() {
  SourceLocation loc = curTok.location;
  consume();

  TypePtr parsedRetType = nullptr;
  if (consumeIf(TokenKind::Less)) {
    parsedRetType = parseType();
    expectGreater();
  }

  expect(TokenKind::LParen);
  std::string asmStr = "";
  if (curTok.is(TokenKind::StringLiteral)) {
    asmStr = curTok.getSpelling().str();
    if (asmStr.size() >= 2)
      asmStr = asmStr.substr(1, asmStr.size() - 2);
    consume();
  } else {
    error("Expected string literal for asm block");
  }
  expect(TokenKind::RParen);

  std::vector<AsmExpr::AsmOperand> outputs;
  std::vector<AsmExpr::AsmOperand> inputs;
  std::vector<AsmExpr::AsmOperand> inouts;
  std::vector<std::string> clobbers;
  bool isVolatile = false;

  // Process trailing directives: out(), in(), inout(), clobber(), volatile
  while (curTok.isAny(TokenKind::KwOut, TokenKind::KwIn, TokenKind::KwInout,
                      TokenKind::KwClobber, TokenKind::KwVolatile)) {

    if (consumeIf(TokenKind::KwVolatile)) {
      isVolatile = true;
      continue;
    }

    if (consumeIf(TokenKind::KwClobber)) {
      expect(TokenKind::LParen);
      if (curTok.isNot(TokenKind::RParen)) {
        do {
          if (curTok.is(TokenKind::StringLiteral)) {
            std::string clob = curTok.getSpelling().str();
            if (clob.size() >= 2)
              clob = clob.substr(1, clob.size() - 2);
            clobbers.push_back(std::move(clob));
            consume();
          } else {
            error("Expected string literal for clobber register");
          }
        } while (consumeIf(TokenKind::Comma));
      }
      expect(TokenKind::RParen);
      continue;
    }

    // Handle in(), out(), inout()
    TokenKind opKind = curTok.kind;
    consume();

    expect(TokenKind::LParen);
    if (curTok.isNot(TokenKind::RParen)) {
      do {
        std::string constraint = "";
        if (curTok.is(TokenKind::StringLiteral)) {
          constraint = curTok.getSpelling().str();
          if (constraint.size() >= 2)
            constraint = constraint.substr(1, constraint.size() - 2);
          consume();
        } else {
          error("Expected string literal for asm constraint");
        }

        expect(TokenKind::LParen);
        ExprPtr expr = parseExpression();
        expect(TokenKind::RParen);

        if (opKind == TokenKind::KwOut) {
          outputs.emplace_back(std::move(constraint), std::move(expr));
        } else if (opKind == TokenKind::KwIn) {
          inputs.emplace_back(std::move(constraint), std::move(expr));
        } else if (opKind == TokenKind::KwInout) {
          inouts.emplace_back(std::move(constraint), std::move(expr));
        }

      } while (consumeIf(TokenKind::Comma)); // Allows: in("r"(a), "m"(b))
    }
    expect(TokenKind::RParen);
  }

  const Type *safeTypePtr = nullptr;
  if (parsedRetType) {
    safeTypePtr = context.saveType(std::move(parsedRetType));
  }

  auto expr = std::make_unique<AsmExpr>(
      std::move(asmStr), std::move(outputs), std::move(inputs),
      std::move(inouts), std::move(clobbers), isVolatile, nullptr, loc);
  expr->setType(safeTypePtr);
  return expr;
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
  case TokenKind::Power: {
    SourceLocation starLoc = curTok.location;
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<PointerType>(
        std::make_unique<PointerType>(std::move(inner), starLoc), starLoc);
    break;
  }
  // Primitives
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
    type = std::make_unique<PrimitiveType>(
        isUnsigned ? PrimitiveType::Scalar::U8 : PrimitiveType::Scalar::Char,
        loc);
    consume();
    break;
  case TokenKind::KwFloat:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F32, loc);
    consume();
    break;
  case TokenKind::KwDouble:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F64, loc);
    consume();
    break;
  case TokenKind::KwISize:
    type = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::ISize, loc);
    consume();
    break;
  case TokenKind::KwUSize:
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
  case TokenKind::KwDecimal: {
    consume();

    unsigned int precision = 0;
    unsigned int scale = 0;

    if (expect(TokenKind::Less)) {
      if (curTok.is(TokenKind::IntegerLiteral)) {
        try {
          precision = std::stoul(curTok.getSpelling().str());
        } catch (const std::out_of_range &) {
          error("Decimal precision is too large");
          precision = 0;
        }
        consume();
      } else {
        error("Expected integer literal for decimal precision");
      }

      expect(TokenKind::Comma);

      if (curTok.is(TokenKind::IntegerLiteral)) {
        try {
          scale = std::stoul(curTok.getSpelling().str());
        } catch (const std::out_of_range &) {
          error("Decimal scale is too large");
          scale = 0;
        }
        consume();
      } else {
        error("Expected integer literal for decimal scale");
      }

      expectGreater();
    }

    type = std::make_unique<DecimalType>(precision, scale, loc);
    break;
  }

  // Modifiers
  case TokenKind::KwLock: {
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<LockType>(std::move(inner), loc);
    break;
  }
  case TokenKind::KwView: {
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<ViewType>(std::move(inner), loc);
    break;
  }
  case TokenKind::KwMut: {
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;
    type = std::make_unique<MutType>(std::move(inner), loc);
    break;
  }
  case TokenKind::KwWeak: {
    consume();
    TypePtr inner = parseType();
    if (!inner)
      return nullptr;

    type = std::make_unique<NullableType>(
        std::make_unique<WeakType>(std::move(inner), loc), loc);
    break;
  }

  // Special Types
  case TokenKind::KwPromise: {
    consume();
    if (consumeIf(TokenKind::Less)) {
      TypePtr inner = parseType();
      if (!inner)
        return nullptr;
      expectGreater();
      type = std::make_unique<PromiseType>(std::move(inner), loc);
    } else {
      error("Expected '<' after promise");
    }
    break;
  }

  case TokenKind::KwClosure:
    type = parseClosureType();
    break;

  case TokenKind::KwThread:
    type = std::make_unique<NamedType>(
        "Thread", std::vector<NamedType::GenericArg>{}, loc);
    consume();
    break;

  // Reference Type (ref T)
  case TokenKind::KwRef: {
    consume();
    type = std::make_unique<ReferenceType>(parseType(), loc);
    break;
  }

    // Map Type (table<K, V>)
  case TokenKind::KwTable: {
    consume();
    if (consumeIf(TokenKind::Less)) {
      TypePtr key = parseType();
      expect(TokenKind::Comma);
      TypePtr val = parseType();
      expectGreater();
      type = std::make_unique<MapType>(std::move(key), std::move(val), loc);
    } else {
      // Implicit table<any, any> for Python/JS style dictionaries
      type = std::make_unique<MapType>(std::make_unique<AnyType>(loc),
                                       std::make_unique<AnyType>(loc), loc);
    }
    break;
  }

  // Named Types & Generics
  case TokenKind::Identifier: {
    std::string name = curTok.getSpelling().str();
    consume();
    while (consumeIf(TokenKind::Dot)) {
      if (curTok.is(TokenKind::Identifier)) {
        name += "." + curTok.getSpelling().str();
        consume();
      } else {
        error("Expected identifier after '.' in type name");
        break;
      }
    }
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

  // Function Type: (Args) => Ret
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

  // Suffixes (Ptr*, Array[], Nullable?)
  while (true) {
    SourceLocation loc = curTok.location;

    // Case 1: Standard T? (Nullable Wrapper)
    if (consumeIf(TokenKind::Question)) {
      type = std::make_unique<NullableType>(std::move(type), loc);
    }
    // Case 2: Array/Slice Dimensions T[]
    else if (curTok.is(TokenKind::LBracket)) {
      std::vector<ExprPtr> sizes;
      do {
        consume();
        ExprPtr sizeExpr =
            (curTok.isNot(TokenKind::RBracket)) ? parseExpression() : nullptr;
        expect(TokenKind::RBracket);
        sizes.push_back(std::move(sizeExpr));
      } while (curTok.is(TokenKind::LBracket));

      auto it = sizes.rbegin();
      while (it != sizes.rend()) {
        if (*it == nullptr) {
          type = std::make_unique<SliceType>(std::move(type), loc);
        } else {
          type =
              std::make_unique<ArrayType>(std::move(type), std::move(*it), loc);
        }
        ++it;
      }
    }
    // Case 3: Pointers (int*)
    else if (consumeIf(TokenKind::Star)) {
      type = std::make_unique<PointerType>(std::move(type), loc);
    }
    // Case 4: Double Pointers parsed as suffix (int**)
    else if (consumeIf(TokenKind::Power)) {
      type = std::make_unique<PointerType>(
          std::make_unique<PointerType>(std::move(type), loc), loc);
    } else {
      break;
    }
  }
  return type;
}

TypePtr Parser::parseClosureType() {
  SourceLocation loc = curTok.location;
  consume();
  expect(TokenKind::LParen);

  std::vector<TypePtr> paramTypes;
  if (curTok.isNot(TokenKind::RParen)) {
    do {
      if (TypePtr t = parseType()) {
        paramTypes.push_back(std::move(t));
      }
    } while (consumeIf(TokenKind::Comma));
  }
  expect(TokenKind::RParen);

  TypePtr returnType = nullptr;
  // Parse '->' (Arrow) to define the return type
  if (consumeIf(TokenKind::Arrow)) {
    returnType = parseType();
  } else {
    // Default to void if omitted: closure()
    returnType = std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void,
                                                 curTok.location);
  }

  return std::make_unique<ClosureType>(std::move(returnType),
                                       std::move(paramTypes), loc);
}

} // namespace moksha
