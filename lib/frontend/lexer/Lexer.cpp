#include "moksha/Lexer/Lexer.h"
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

Lexer::Lexer(llvm::StringRef buffer)
    : bufferStart(buffer.begin()), bufferEnd(buffer.end()),
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

  const char *start = curPtr;
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

  if (*curPtr == '.') {
    isFloat = true;
    curPtr++;
    while (curPtr < bufferEnd &&
           (isdigit(static_cast<unsigned char>(*curPtr)) || *curPtr == '_'))
      curPtr++;
  } else {
    if (*curPtr == '0') {
      char next = peek(1);
      if (next == 'x' || next == 'X') {
        isHex = true;
        curPtr += 2;
        if (curPtr >= bufferEnd || (!isHexDigit(*curPtr) && *curPtr != '_'))
          return errorAt(start);
        while (curPtr < bufferEnd && (isHexDigit(*curPtr) || *curPtr == '_'))
          curPtr++;
      } else if (next == 'b' || next == 'B') {
        isBin = true;
        curPtr += 2;
        if (curPtr >= bufferEnd || (!isBinDigit(*curPtr) && *curPtr != '_'))
          return errorAt(start);
        while (curPtr < bufferEnd && (isBinDigit(*curPtr) || *curPtr == '_'))
          curPtr++;
      }
    }

    if (!isHex && !isBin) {
      while (curPtr < bufferEnd &&
             (isdigit(static_cast<unsigned char>(*curPtr)) || *curPtr == '_'))
        curPtr++;
    }

    if (!isHex && !isBin && peek() == '.') {
      if (isdigit(peek(1))) {
        isFloat = true;
        curPtr++;
        while (curPtr < bufferEnd &&
               (isdigit(static_cast<unsigned char>(*curPtr)) || *curPtr == '_'))
          curPtr++;
      }
    }
  }

  if (!isHex && !isBin && (peek() == 'e' || peek() == 'E')) {
    isFloat = true;
    curPtr++;
    if (peek() == '+' || peek() == '-')
      curPtr++;

    if (!isdigit(peek()))
      return errorAt(start);

    while (curPtr < bufferEnd &&
           (isdigit(static_cast<unsigned char>(*curPtr)) || *curPtr == '_'))
      curPtr++;
  }

  if (curPtr > start && *(curPtr - 1) == '_') {
    return errorAt(start);
  }

  Token tok(isFloat ? TokenKind::FloatLiteral : TokenKind::IntegerLiteral,
            SourceLocation::getFromPointer(start), curPtr - start);

  if (isHex)
    tok.flags |= TF_IsHex;
  if (isBin)
    tok.flags |= TF_IsBin;

  if (curPtr < bufferEnd &&
      (isAsciiIdentStart(*curPtr) || isUTF8Start(*curPtr))) {
    const char *suffixStart = curPtr;
    while (curPtr < bufferEnd &&
           (isalnum(static_cast<unsigned char>(*curPtr)) || *curPtr == '_'))
      curPtr++;
    llvm::StringRef suffixStr(suffixStart, curPtr - suffixStart);

    tok.suffix = llvm::StringSwitch<NumericSuffix>(suffixStr)
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
                     .Default(NumericSuffix::None);

    if (tok.suffix == NumericSuffix::None && !suffixStr.empty()) {
      return errorAt(start);
    }

    tok.length = curPtr - start;
  }

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
    }
    // [MODIFIED] Removed Arrow '->' check here
    else
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
    if (peek() == '|') {
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
    curPtr += 2;
    int depth = 1;

    while (curPtr < bufferEnd) {
      if (curPtr + 1 >= bufferEnd) {
        curPtr++;
        return false;
      }

      char c = *curPtr;
      char next = *(curPtr + 1);

      if (c == '/' && next == '*') {
        depth++;
        curPtr += 2;
      } else if (c == '*' && next == '/') {
        depth--;
        curPtr += 2;
        if (depth == 0)
          return true;
      } else {
        curPtr++;
      }
    }
    return false;
  }

  return false;
}

Token Lexer::scanString() {
  const char *start = nullptr;
  bool isTemplate = inTemplateString();
  bool hasEscape = false;

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

    if (c == '\\') {
      hasEscape = true;
      if (!consumeEscape())
        return errorAt(isTemplate ? fragmentStart : start);
      continue;
    }

    if (c == quote) {
      const char *tokenStart = isTemplate ? fragmentStart : start;
      curPtr++;

      if (isTemplate) {
        state = LexerState::Normal;
        fragmentStart = nullptr;
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

  return errorAt(isTemplate ? fragmentStart : start);
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
    if (isspace(static_cast<unsigned char>(*curPtr)))
      curPtr++;
    else
      break;
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
      if (!isHexDigit(curPtr[i]))
        return false;
    }
    curPtr += 2;
    return true;
  }
  case 'u': {
    curPtr++;
    if (curPtr + 4 > bufferEnd)
      return false;
    for (int i = 0; i < 4; ++i) {
      if (!isHexDigit(curPtr[i]))
        return false;
    }
    curPtr += 4;
    return true;
  }
  case 'U': {
    curPtr++;
    if (curPtr + 8 > bufferEnd)
      return false;
    for (int i = 0; i < 8; ++i) {
      if (!isHexDigit(curPtr[i]))
        return false;
    }
    curPtr += 8;
    return true;
  }
  default:
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
                       .Case("thread", TokenKind::KwThread)
                       .Case("ref", TokenKind::KwRef)
                       .Case("weak", TokenKind::KwWeak)
                       .Case("shared", TokenKind::KwShared)
                       .Case("new", TokenKind::KwNew)
                       .Case("delete", TokenKind::KwDelete)
                       .Case("unsafe", TokenKind::KwUnsafe)
                       .Case("constructor", TokenKind::KwConstructor)
                       .Case("destructor", TokenKind::KwDestructor)
                       .Case("this", TokenKind::KwThis)
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
                       .Case("any", TokenKind::KwAny)
                       .Case("true", TokenKind::KwTrue)
                       .Case("false", TokenKind::KwFalse)
                       .Case("public", TokenKind::KwPublic)
                       .Case("private", TokenKind::KwPrivate)
                       .Case("protected", TokenKind::KwProtected)
                       .Case("const", TokenKind::KwConst)
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
                       .Case("lock", TokenKind::KwLock)
                       .Case("view", TokenKind::KwView)
                       .Case("mut", TokenKind::KwMut)
                       .Default(TokenKind::Identifier);

  return Token(kind, SourceLocation::getFromPointer(start), curPtr - start);
}

} // namespace moksha
