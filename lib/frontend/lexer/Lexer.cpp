#include "moksha/Lexer/Lexer.h"
#include "moksha/Support/Diagnostics.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ConvertUTF.h"
#include "llvm/Support/ErrorHandling.h"
#include <algorithm>
#include <cassert>
#include <cctype>

namespace moksha {

// --- Helper Functions ---

static bool isHexDigit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
         (c >= 'A' && c <= 'F');
}

static bool isBinDigit(char c) { return c == '0' || c == '1'; }

// --- Lexer Implementation ---

static bool isAsciiIdentStart(char c) {
  return std::isalpha(static_cast<unsigned char>(c)) || c == '_';
}

static bool isAsciiIdentBody(char c) {
  return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

static bool isUTF8Start(char c) {
  return static_cast<unsigned char>(c) >= 0x80;
}

Lexer::Lexer(llvm::StringRef buffer, DiagnosticEngine &Diags)
    : Diags(Diags), bufferStart(buffer.begin()), bufferEnd(buffer.end()),
      curPtr(buffer.begin()) {
  state = LexerState::Normal;
}

SourceLocation Lexer::getLoc() const {
  return SourceLocation::getFromPointer(curPtr);
}

char Lexer::peek(int n) const {
  if (curPtr + n >= bufferEnd)
    return 0;
  return *(curPtr + n);
}

Token Lexer::next() {
  if (inTemplateString() && interpolationStack.empty()) {
    if (curPtr >= bufferEnd)
      return errorAt(fragmentStart ? fragmentStart : bufferStart);
    return scanString();
  }

  while (true) {
    skipWhitespace();

    if (curPtr >= bufferEnd)
      return Token(TokenKind::Eof, getLoc(), 0);

    if (*curPtr == '/') {
      char nextChar = peek(1);
      if (nextChar == '/' || nextChar == '*') {
        if (!skipComment()) {
          return errorAt(curPtr);
        }
        continue;
      }
    }
    break;
  }

  // const char *start = curPtr;
  char c = *curPtr;

  if (isAsciiIdentStart(c) || isUTF8Start(c))
    return scanIdentifier();
  if (isdigit(static_cast<unsigned char>(c)))
    return scanNumber();

  if (c == '.' && isdigit(peek(1))) {
    return scanNumber();
  }

  if (c == '"' || c == '`')
    return scanString();
  if (c == '\'')
    return scanChar();

  return scanOperator();
}

Token Lexer::scanNumber() {
  const char *start = curPtr;
  bool isFloat = false;
  bool isHex = false;
  bool isBin = false;
  bool isOct = false;
  bool hadLexicalError = false;

  // 1. Initial Prefix and Type Detection
  if (*curPtr == '.') {
    isFloat = true;
    curPtr++;
    while (curPtr < bufferEnd && (isdigit(*curPtr) || *curPtr == '_'))
      curPtr++;
  } else if (*curPtr == '0') {
    char next = peek(1);
    if (next == 'x' || next == 'X') {
      isHex = true;
      curPtr += 2;
      // [FIX] Consume all alphanumeric characters to prevent token splitting
      while (curPtr < bufferEnd) {
        if (isHexDigit(*curPtr) || *curPtr == '_') {
          curPtr++;
        } else if (isalpha(*curPtr)) {
          // We hit a letter that isn't A-F (like 'u' or 'i' for suffixes)
          // Break so Step 3 (Suffix Processing) can handle it
          break;
        } else {
          break; // Stop on punctuation or spaces
        }
      }
    } else if (next == 'b' || next == 'B') {
      isBin = true;
      curPtr += 2;
      while (curPtr < bufferEnd) {
        if (isBinDigit(*curPtr) || *curPtr == '_') {
          curPtr++;
        } else if (isalpha(*curPtr)) {
          // We hit a letter for a suffix (like 'u' or 'i')
          break;
        } else {
          break;
        }
      }
    } else if (next == 'o' || next == 'O') { // [FIX 1] Handle Octal Prefix
      isOct = true;
      curPtr += 2;
      while (curPtr < bufferEnd) {
        if ((*curPtr >= '0' && *curPtr <= '7') || *curPtr == '_') {
          curPtr++;
        } else if (isalpha(*curPtr)) {
          break;
        } else {
          break;
        }
      }
    }
  }

  // 2. Decimal / Float Body Scanning
  if (!isHex && !isBin && !isOct) {
    while (curPtr < bufferEnd && (isdigit(*curPtr) || *curPtr == '_'))
      curPtr++;

    if (peek() == '.') {
      if (isdigit(peek(1))) {
        isFloat = true;
        curPtr++;
        while (curPtr < bufferEnd && (isdigit(*curPtr) || *curPtr == '_'))
          curPtr++;

        // [FIX] Catch multiple decimal points: 1.2.3
        if (peek() == '.') {
          Diags.report(getLoc(), DiagID::err_unexpected_char)
              << "multiple decimal points in float literal";
          hadLexicalError = true;
          while (curPtr < bufferEnd &&
                 (isdigit(*curPtr) || *curPtr == '.' || *curPtr == '_'))
            curPtr++;
        }
      } else if (!isAsciiIdentStart(peek(1))) {
        // [FIX 2] Handle trailing dot (e.g. `10.;`)
        isFloat = true;
        curPtr++;
      }
    }

    // Exponent Handling: 1.0e+10
    if (peek() == 'e' || peek() == 'E') {
      isFloat = true;
      curPtr++;
      if (peek() == '+' || peek() == '-')
        curPtr++;
      if (!isdigit(peek())) {
        // [FIX] Explicitly report missing exponent digits
        Diags.report(getLoc(), DiagID::err_unexpected_char)
            << "missing digits after exponent";
        hadLexicalError = true;
      }
      while (curPtr < bufferEnd && (isdigit(*curPtr) || *curPtr == '_'))
        curPtr++;
    }
  }

  // 3. Suffix Processing
  NumericSuffix suffix = NumericSuffix::None;
  if (curPtr < bufferEnd &&
      (isAsciiIdentStart(*curPtr) || isUTF8Start(*curPtr))) {
    const char *suffixStart = curPtr;
    while (curPtr < bufferEnd && (isalnum(*curPtr) || *curPtr == '_'))
      curPtr++;

    llvm::StringRef suffixStr(suffixStart, curPtr - suffixStart);
    suffix = llvm::StringSwitch<NumericSuffix>(suffixStr)
                 .Case("i8", NumericSuffix::i8)
                 .Case("u8", NumericSuffix::u8)
                 .Case("i16", NumericSuffix::i16)
                 .Case("u16", NumericSuffix::u16)
                 .Case("i32", NumericSuffix::i32)
                 .Case("u32", NumericSuffix::u32)
                 .Case("i64", NumericSuffix::i64)
                 .Case("u64", NumericSuffix::u64)
                 .Case("isize", NumericSuffix::isize)
                 .Case("usize", NumericSuffix::usize)
                 .Case("f8", NumericSuffix::f8)
                 .Case("f16", NumericSuffix::f16)
                 .Case("f32", NumericSuffix::f32)
                 .Case("f64", NumericSuffix::f64)
                 .Case("d", NumericSuffix::d)
                 .Default(NumericSuffix::None);

    if (suffix == NumericSuffix::None && !suffixStr.empty()) {
      Diags.report(SourceLocation::getFromPointer(suffixStart),
                   DiagID::err_unexpected_char)
          << "invalid numeric suffix '" << suffixStr << "'";
      hadLexicalError = true;
    }
  }

  // Handle trailing underscores (e.g., 123_)
  if (curPtr > start && *(curPtr - 1) == '_' && suffix == NumericSuffix::None) {
    Diags.report(getLoc(), DiagID::err_unexpected_char)
        << "number cannot end with underscore";
    hadLexicalError = true;
  }

  // Catch consecutive underscores anywhere in the parsed numeric token
  for (const char *p = start; p < curPtr - 1; ++p) {
    if (*p == '_' && *(p + 1) == '_') {
      Diags.report(SourceLocation::getFromPointer(p),
                   DiagID::err_unexpected_char)
          << "number cannot contain consecutive underscores";
      hadLexicalError = true;
      break; // Only report once per number to avoid spamming the console
    }
  }

  if (hadLexicalError)
    return errorAt(start);

  TokenKind kind;
  if (suffix == NumericSuffix::d) {
    kind = TokenKind::DecimalLiteral;
  } else {
    kind = isFloat ? TokenKind::FloatLiteral : TokenKind::IntegerLiteral;
  }

  Token tok(kind, SourceLocation::getFromPointer(start), curPtr - start);
  tok.suffix = suffix;
  if (isHex)
    tok.flags |= TF_IsHex;
  if (isBin)
    tok.flags |= TF_IsBin;
  return tok;
}

Token Lexer::scanOperator() {
  const char *start = curPtr;
  char c = *curPtr++;
  TokenKind kind = TokenKind::Error;

  switch (c) {
  case '{':
    kind = TokenKind::LBrace;
    if (!interpolationStack.empty()) {
      interpolationStack.back().braceBalance++;
    }
    break;

  case '}':
    kind = TokenKind::RBrace;
    if (!interpolationStack.empty()) {
      if (interpolationStack.back().braceBalance == 0) {
        interpolationStack.pop_back();
        state = LexerState::TemplateString;
        fragmentStart = curPtr;
        return Token(TokenKind::InterpolationEnd,
                     SourceLocation::getFromPointer(start), curPtr - start);
      } else {
        interpolationStack.back().braceBalance--;
      }
    }
    break;

  case '+':
    if (peek() == '+') {
      curPtr++;
      kind = TokenKind::PlusPlus;
    } else if (peek() == '=') {
      curPtr++;
      kind = TokenKind::PlusEqual;
    } else
      kind = TokenKind::Plus;
    break;
  case '-':
    if (peek() == '-') {
      curPtr++;
      kind = TokenKind::MinusMinus;
    } else if (peek() == '=') {
      curPtr++;
      kind = TokenKind::MinusEqual;
    } else if (peek() == '>') {
      curPtr++;
      kind = TokenKind::Arrow;
    } else
      kind = TokenKind::Minus;
    break;
  case '*':
    if (peek() == '*') {
      curPtr++;
      kind = TokenKind::Power;
    } else if (peek() == '=') {
      curPtr++;
      kind = TokenKind::StarEqual;
    } else
      kind = TokenKind::Star;
    break;
  case '/':
    if (peek() == '=') {
      curPtr++;
      kind = TokenKind::SlashEqual;
    } else
      kind = TokenKind::Slash;
    break;
  case '%':
    if (peek() == '=') {
      curPtr++;
      kind = TokenKind::PercentEqual;
    } else
      kind = TokenKind::Percent;
    break;
  case '<':
    if (peek() == '<') {
      curPtr++;
      if (peek() == '=') {
        curPtr++;
        kind = TokenKind::LessLessEqual;
      } else
        kind = TokenKind::LessLess;
    } else if (peek() == '=') {
      curPtr++;
      kind = TokenKind::LessEqual;
    } else
      kind = TokenKind::Less;
    break;
  case '>':
    if (peek() == '>') {
      curPtr++;
      if (peek() == '=') {
        curPtr++;
        kind = TokenKind::GreaterGreaterEqual;
      } else
        kind = TokenKind::GreaterGreater;
    } else if (peek() == '=') {
      curPtr++;
      kind = TokenKind::GreaterEqual;
    } else
      kind = TokenKind::Greater;
    break;
  case '=':
    if (peek() == '=') {
      curPtr++;
      kind = TokenKind::EqualEqual;
    } else if (peek() == '>') { // [KEPT] Fat Arrow =>
      curPtr++;
      kind = TokenKind::FatArrow;
    } else
      kind = TokenKind::Equal;
    break;
  case '!':
    if (peek() == '=') {
      curPtr++;
      kind = TokenKind::NotEqual;
    } else
      kind = TokenKind::Bang;
    break;
  case '&':
    if (peek() == '&') {
      curPtr++;
      kind = TokenKind::AmpAmp;
    } else if (peek() == '=') {
      curPtr++;
      kind = TokenKind::AmpEqual;
    } else
      kind = TokenKind::Amp;
    break;
  case '|':
    if (peek() == '>') {
      curPtr++;
      kind = TokenKind::PipeGreater;
    } else if (peek() == '|') {
      curPtr++;
      kind = TokenKind::PipePipe;
    } else if (peek() == '=') {
      curPtr++;
      kind = TokenKind::PipeEqual;
    } else
      kind = TokenKind::Pipe;
    break;
  case '^':
    if (peek() == '=') {
      curPtr++;
      kind = TokenKind::CaretEqual;
    } else
      kind = TokenKind::Caret;
    break;
  case '~':
    kind = TokenKind::Tilde;
    break;
  case '?':
    if (peek() == '?') {
      curPtr++;
      kind = TokenKind::QuestionQuestion;
    } else if (peek() == '.') {
      curPtr++;
      kind = TokenKind::QuestionDot;
    } else
      kind = TokenKind::Question;
    break;
  case '.':
    if (peek() == '.' && peek(1) == '.') {
      curPtr += 2; // Consume the extra two dots
      kind = TokenKind::DotDotDot;
    } else {
      kind = TokenKind::Dot;
    }
    break;
  case ',':
    kind = TokenKind::Comma;
    break;
  case ';':
    kind = TokenKind::Semicolon;
    break;
  case ':':
    kind = TokenKind::Colon;
    break;
  case '(':
    kind = TokenKind::LParen;
    break;
  case ')':
    kind = TokenKind::RParen;
    break;
  case '[':
    kind = TokenKind::LBracket;
    break;
  case ']':
    kind = TokenKind::RBracket;
    break;

  default:
    kind = TokenKind::Error;
    break;
  }
  return Token(kind, SourceLocation::getFromPointer(start), curPtr - start);
}

bool Lexer::skipComment() {
  if (peek(1) == '/') {
    curPtr += 2;
    while (curPtr < bufferEnd && *curPtr != '\n' && *curPtr != '\r')
      curPtr++;
    return true;
  }

  if (peek(1) == '*') {
    const char *commentStart = curPtr;
    curPtr += 2;
    int depth = 1;

    while (curPtr < bufferEnd) {
      // Support nested comments: /* /* ... */ */
      if (*curPtr == '/' && peek(1) == '*') {
        depth++;
        curPtr += 2;
      } else if (*curPtr == '*' && peek(1) == '/') {
        depth--;
        curPtr += 2;
        if (depth == 0)
          return true; // Successfully skipped
      } else {
        curPtr++;
      }
    }
    Diags.report(SourceLocation::getFromPointer(commentStart),
                 DiagID::err_unexpected_char)
        << "unterminated block comment";
    return false;
  }

  return false;
}

Token Lexer::scanString() {
  const char *start = nullptr;
  bool isTemplate = inTemplateString();
  bool hasEscape = false;
  bool hadError = false;

  if (isTemplate && fragmentStart == nullptr) {
    fragmentStart = curPtr;
  }

  if (!isTemplate) {
    start = curPtr;
    char quote = *curPtr++;
    if (quote == '`') {
      isTemplate = true;
      state = LexerState::TemplateString;
      fragmentStart = curPtr;
      start = nullptr;
    }
  }

  char quote = isTemplate ? '`' : '"';

  while (curPtr < bufferEnd) {
    char c = *curPtr;

    if (!isTemplate && (c == '\n' || c == '\r')) {
      Diags.report(getLoc(), DiagID::err_unexpected_char)
          << "unterminated string literal";
      return errorAt(start);
    }

    if (c == '\\') {
      hasEscape = true;
      if (!consumeEscape()) {
        hadError = true;
      }
      continue;
    }

    if (c == quote) {
      const char *tokenStart = isTemplate ? fragmentStart : start;
      curPtr++;

      if (isTemplate) {
        state = LexerState::Normal;
        fragmentStart = nullptr;
      }

      if (hadError) {
        return errorAt(tokenStart);
      }

      Token t(isTemplate ? TokenKind::TemplateString : TokenKind::StringLiteral,
              SourceLocation::getFromPointer(tokenStart), curPtr - tokenStart);
      if (hasEscape)
        t.flags |= TF_IsEscaped;
      return t;
    }

    if (isTemplate && c == '$' && peek(1) == '{') {
      if (curPtr == fragmentStart) {
        const char *interpStart = curPtr;
        curPtr += 2;
        state = LexerState::Normal;
        interpolationStack.push_back({0});
        return Token(TokenKind::InterpolationStart,
                     SourceLocation::getFromPointer(interpStart), 2);
      } else {
        Token t(TokenKind::StringFragment,
                SourceLocation::getFromPointer(fragmentStart),
                curPtr - fragmentStart);
        if (hasEscape)
          t.flags |= TF_IsEscaped;
        fragmentStart = curPtr;
        return t;
      }
    }

    curPtr++;
  }
  const char *errStart = isTemplate ? fragmentStart : start;
  if (isTemplate) {
    state = LexerState::Normal;
    fragmentStart = nullptr;
  }
  Diags.report(SourceLocation::getFromPointer(errStart),
               DiagID::err_unexpected_char)
      << (isTemplate ? "unterminated template string"
                     : "unterminated string literal");
  return errorAt(errStart);
}

Token Lexer::scanChar() {
  const char *start = curPtr;
  curPtr++;

  if (peek() == '\\') {
    if (!consumeEscape())
      return errorAt(start);
  } else {
    if (!advanceUTF8())
      return errorAt(start);
  }

  if (peek() != '\'')
    return errorAt(start);

  curPtr++;
  return Token(TokenKind::CharLiteral, SourceLocation::getFromPointer(start),
               curPtr - start);
}

void Lexer::skipWhitespace() {
  while (curPtr < bufferEnd) {
    char c = *curPtr;
    if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r') {
      curPtr++;
    } else if (c == '\n') {
      curPtr++; // Handle newlines
    } else {
      break;
    }
  }
}

bool Lexer::advanceUTF8() {
  const unsigned char *uPtr = reinterpret_cast<const unsigned char *>(curPtr);
  unsigned len = llvm::getNumBytesForUTF8(*uPtr);

  if (len > 0 && (curPtr + len <= bufferEnd) &&
      llvm::isLegalUTF8Sequence(uPtr, uPtr + len)) {
    curPtr += len;
    return true;
  }
  return false;
}

bool Lexer::consumeEscape() {
  const char *escapeStart = curPtr;
  curPtr++;
  if (curPtr >= bufferEnd)
    return false;

  char c = *curPtr;
  switch (c) {
  case 'n':
  case 'r':
  case 't':
  case '0':
  case '\\':
  case '\'':
  case '"':
  case '`':
  case '$':
    curPtr++;
    return true;
  case 'x': {
    curPtr++;
    if (curPtr + 2 > bufferEnd)
      return false;
    for (int i = 0; i < 2; ++i) {
      if (!isHexDigit(curPtr[i])) {
        Diags.report(getLoc(), DiagID::err_unexpected_char)
            << "invalid hex digit in escape";
        return false;
      }
    }
    curPtr += 2;
    return true;
  }
  case 'u': {
    curPtr++;

    // [FIX 3] Handle Rust-style \u{...}
    if (curPtr < bufferEnd && *curPtr == '{') {
      curPtr++; // skip '{'
      while (curPtr < bufferEnd && *curPtr != '}') {
        if (!isHexDigit(*curPtr)) {
          Diags.report(getLoc(), DiagID::err_unexpected_char)
              << "invalid hex digit in unicode escape";
          return false;
        }
        curPtr++;
      }
      if (curPtr >= bufferEnd || *curPtr != '}') {
        Diags.report(getLoc(), DiagID::err_unexpected_char)
            << "unterminated unicode escape sequence";
        return false;
      }
      curPtr++; // skip '}'
      return true;
    }

    // Fallback to standard 4-digit \uXXXX
    if (curPtr + 4 > bufferEnd)
      return false;
    for (int i = 0; i < 4; ++i) {
      if (!isHexDigit(curPtr[i])) {
        Diags.report(getLoc(), DiagID::err_unexpected_char)
            << "invalid hex digit in escape";
        return false;
      }
    }
    curPtr += 4;
    return true;
  }
  case 'U': {
    curPtr++;
    if (curPtr + 8 > bufferEnd)
      return false;
    for (int i = 0; i < 8; ++i) {
      if (!isHexDigit(curPtr[i])) {
        Diags.report(getLoc(), DiagID::err_unexpected_char)
            << "invalid hex digit in escape";
        return false;
      }
    }
    curPtr += 8;
    return true;
  }
  default:
    Diags.report(SourceLocation::getFromPointer(escapeStart),
                 DiagID::err_unexpected_char)
        << "invalid escape sequence";
    return false;
  }
}

Token Lexer::scanIdentifier() {
  const char *start = curPtr;

  // --- Identifier start ---
  if (isAsciiIdentStart(*curPtr)) {
    ++curPtr;
  } else if (isUTF8Start(*curPtr)) {
    if (!advanceUTF8())
      return errorAt(start);
  } else {
    return errorAt(start);
  }

  // --- Identifier body ---
  while (curPtr < bufferEnd) {
    char c = *curPtr;

    if (isAsciiIdentBody(c)) {
      ++curPtr;
    } else if (isUTF8Start(c)) {
      if (!advanceUTF8())
        return errorAt(start);
    } else {
      break;
    }
  }

  llvm::StringRef spelling(start, curPtr - start);

  TokenKind kind = llvm::StringSwitch<TokenKind>(spelling)
                       .Case("class", TokenKind::KwClass)
                       .Case("operator", TokenKind::KwOperator)
                       .Case("thread", TokenKind::KwThread)
                       .Case("ref", TokenKind::KwRef)
                       .Case("weak", TokenKind::KwWeak)
                       .Case("shared", TokenKind::KwShared)
                       .Case("new", TokenKind::KwNew)
                       .Case("delete", TokenKind::KwDelete)
                       .Case("unsafe", TokenKind::KwUnsafe)
                       .Case("unsigned", TokenKind::KwUnsigned)
                       .Case("constructor", TokenKind::KwConstructor)
                       .Case("destructor", TokenKind::KwDestructor)
                       .Case("this", TokenKind::KwThis)
                       .Case("super", TokenKind::KwSuper)
                       .Case("null", TokenKind::KwNull)
                       .Case("if", TokenKind::KwIf)
                       .Case("else", TokenKind::KwElse)
                       .Case("while", TokenKind::KwWhile)
                       .Case("do", TokenKind::KwDo)
                       .Case("for", TokenKind::KwFor)
                       .Case("in", TokenKind::KwIn)
                       .Case("switch", TokenKind::KwSwitch)
                       .Case("case", TokenKind::KwCase)
                       .Case("default", TokenKind::KwDefault)
                       .Case("return", TokenKind::KwReturn)
                       .Case("break", TokenKind::KwBreak)
                       .Case("continue", TokenKind::KwContinue)
                       .Case("defer", TokenKind::KwDefer)
                       .Case("with", TokenKind::KwWith)
                       .Case("try", TokenKind::KwTry)
                       .Case("catch", TokenKind::KwCatch)
                       .Case("throw", TokenKind::KwThrow)
                       .Case("finally", TokenKind::KwFinally)
                       .Case("void", TokenKind::KwVoid)
                       .Case("int", TokenKind::KwInt)
                       .Case("short", TokenKind::KwShort)
                       .Case("long", TokenKind::KwLong)
                       .Case("isize", TokenKind::KwISize)
                       .Case("usize", TokenKind::KwUSize)
                       .Case("float", TokenKind::KwFloat)
                       .Case("double", TokenKind::KwDouble)
                       .Case("bool", TokenKind::KwBool)
                       .Case("string", TokenKind::KwString)
                       .Case("char", TokenKind::KwChar)
                       .Case("half", TokenKind::KwHalf)
                       .Case("quarter", TokenKind::KwQuarter)
                       .Case("decimal", TokenKind::KwDecimal)
                       .Case("any", TokenKind::KwAny)
                       .Case("as", TokenKind::KwAs)
                       .Case("true", TokenKind::KwTrue)
                       .Case("false", TokenKind::KwFalse)
                       .Case("public", TokenKind::KwPublic)
                       .Case("private", TokenKind::KwPrivate)
                       .Case("protected", TokenKind::KwProtected)
                       .Case("promise", TokenKind::KwPromise)
                       .Case("const", TokenKind::KwConst)
                       .Case("cast", TokenKind::KwCast)
                       .Case("bitcast", TokenKind::KwBitcast)
                       .Case("static", TokenKind::KwStatic)
                       .Case("async", TokenKind::KwAsync)
                       .Case("await", TokenKind::KwAwait)
                       .Case("import", TokenKind::KwImport)
                       .Case("from", TokenKind::KwFrom)
                       .Case("macro", TokenKind::KwMacro)
                       .Case("enum", TokenKind::KwEnum)
                       .Case("table", TokenKind::KwTable)
                       .Case("generic", TokenKind::KwGeneric)
                       .Case("struct", TokenKind::KwStruct)
                       .Case("union", TokenKind::KwUnion)
                       .Case("packed", TokenKind::KwPacked)
                       .Case("volatile", TokenKind::KwVolatile)
                       .Case("align", TokenKind::KwAlign)
                       .Case("sizeof", TokenKind::KwSizeof)
                       .Case("interrupt", TokenKind::KwInterrupt)
                       .Case("naked", TokenKind::KwNaked)
                       .Case("noreturn", TokenKind::KwNoreturn)
                       .Case("noinline", TokenKind::KwNoinline)
                       .Case("inline", TokenKind::KwInline)
                       .Case("pure", TokenKind::KwPure)
                       .Case("cold", TokenKind::KwCold)
                       .Case("lock", TokenKind::KwLock)
                       .Case("view", TokenKind::KwView)
                       .Case("mut", TokenKind::KwMut)
                       .Case("virtual", TokenKind::KwVirtual)
                       .Case("override", TokenKind::KwOverride)
                       .Case("input", TokenKind::KwInput)
                       .Case("closure", TokenKind::KwClosure)
                       .Case("move", TokenKind::KwMove)
                       .Case("out", TokenKind::KwOut)
                       .Case("inout", TokenKind::KwInout)
                       .Case("clobber", TokenKind::KwClobber)
                       .Case("extern", TokenKind::KwExtern)
                       .Case("using", TokenKind::KwUsing)
                       .Case("asm", TokenKind::KwAsm)
                       .Case("section", TokenKind::KwSection)
                       .Case("used", TokenKind::KwUsed)
                       .Case("thread_local", TokenKind::KwThreadLocal)
                       .Default(TokenKind::Identifier);

  return Token(kind, SourceLocation::getFromPointer(start), curPtr - start);
}

} // namespace moksha
