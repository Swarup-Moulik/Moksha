#pragma once

#include "moksha/Support/SourceLocation.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ConvertUTF.h"
#include <algorithm>
#include <cstdint>
#include <vector>

namespace moksha {

class DiagnosticEngine;

enum class TokenKind {
  Eof,
  Error,
  Comment,

  // --- Identifiers ---
  Identifier,

  // --- Literals ---
  IntegerLiteral,
  FloatLiteral,
  DecimalLiteral,
  StringLiteral,
  TemplateString,
  CharLiteral,

  StringFragment,
  InterpolationStart,
  InterpolationEnd,

  // --- Keywords ---
  KwClass,
  KwOperator,
  KwThread,
  KwRef,
  KwWeak,
  KwShared,
  KwNew,
  KwDelete,
  KwUnsafe,
  KwUnsigned,
  KwConstructor,
  KwDestructor,
  KwThis,
  KwSuper,
  KwNull,
  KwIf,
  KwElse,
  KwWhile,
  KwDo,
  KwFor,
  KwIn,
  KwSwitch,
  KwCase,
  KwDefault,
  KwReturn,
  KwBreak,
  KwContinue,
  KwDefer,
  KwWith,
  KwTry,
  KwCatch,
  KwThrow,
  KwFinally,
  KwVoid,
  KwInt,
  KwFloat,
  KwDouble,
  KwBool,
  KwString,
  KwChar,
  KwShort,
  KwLong,
  KwISize,
  KwUSize,
  KwHalf,
  KwQuarter,
  KwDecimal,
  KwAny,
  KwAs,
  KwTrue,
  KwFalse,
  KwPublic,
  KwPrivate,
  KwProtected,
  KwPromise,
  KwConst,
  KwCast,
  KwStatic,
  KwAsync,
  KwAwait,
  KwImport,
  KwFrom,
  KwMacro,
  KwTable,
  KwEnum,
  KwGeneric,
  KwStruct,
  KwUnion,
  KwPacked,
  KwVolatile,
  KwAlign,
  KwSizeof,
  KwInterrupt,
  KwNaked,
  KwNoreturn,
  KwNoinline,
  KwInline,
  KwPure,
  KwCold,
  KwLock,
  KwView,
  KwMut,
  KwVirtual,
  KwOverride,
  KwInput,
  KwClosure,
  KwMove,

  // --- OS & Low-Level Additions ---
  KwExtern,
  KwUsing, // Replaces typedef
  KwAsm,
  KwSection, // For section(".data")
  KwUsed,
  KwThreadLocal,

  // --- Operators ---
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  Power,
  PlusPlus,
  MinusMinus,
  Equal,
  PlusEqual,
  MinusEqual,
  StarEqual,
  SlashEqual,
  PercentEqual,
  Amp,
  Pipe,
  Caret,
  Tilde,
  AmpEqual,
  PipeEqual,
  CaretEqual,
  LessLess,
  GreaterGreater,
  LessLessEqual,
  GreaterGreaterEqual,
  AmpAmp,
  PipePipe,
  PipeGreater,
  Bang,
  EqualEqual,
  NotEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  FatArrow,
  Arrow,
  Dot,
  DotDotDot,
  QuestionDot,
  QuestionQuestion,
  Question,
  Colon,
  Comma,
  Semicolon,
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket
};

enum class NumericSuffix : uint8_t {
  None,
  i8,
  i16,
  i32,
  i64,
  isize,
  u8,
  u16,
  u32,
  u64,
  usize,
  f8,
  f16,
  f32,
  f64,
  d
};

enum TokenFlag : uint32_t {
  TF_None = 0,
  TF_IsHex = 1 << 0,
  TF_IsBin = 1 << 1,
  TF_IsEscaped = 1 << 2,
  TF_IsSynthesized = 1 << 3,
};

class Token {
public:
  TokenKind kind;
  SourceLocation location;
  uint32_t length;

  NumericSuffix suffix = NumericSuffix::None;
  uint32_t flags = TF_None;

  Token() : kind(TokenKind::Error), length(0) {}
  Token(TokenKind kind, SourceLocation loc, uint32_t len)
      : kind(kind), location(loc), length(len) {}

  llvm::StringRef getSpelling() const {
    return llvm::StringRef(location.getPointer(), length);
  }

  bool is(TokenKind k) const { return kind == k; }
  bool isNot(TokenKind k) const { return kind != k; }

  // [FIX] Variadic template for isAny
  template <typename... T> bool isAny(TokenKind k, T... args) const {
    return is(k) || isAny(args...);
  }

  bool isAny(TokenKind k) const { return is(k); }
};

class Lexer {
public:
  explicit Lexer(llvm::StringRef buffer, DiagnosticEngine &Diags);

  Token next();
  SourceLocation getLoc() const;

private:
  DiagnosticEngine &Diags;
  const char *bufferStart;
  const char *bufferEnd;
  const char *curPtr;
  const char *fragmentStart = nullptr;

  Token scanIdentifier();
  Token scanNumber();
  Token scanString();
  Token scanChar();
  Token scanOperator();

  void skipWhitespace();
  bool skipComment();
  bool advanceUTF8();
  bool consumeEscape();

  char peek(int n = 0) const;

  Token errorAt(const char *start) {
    size_t len = std::max<size_t>(1, curPtr - start);
    return Token(TokenKind::Error, SourceLocation::getFromPointer(start), len);
  }

  enum class LexerState { Normal, TemplateString };

  struct InterpolationState {
    int braceBalance = 0;
  };

  LexerState state = LexerState::Normal;
  std::vector<InterpolationState> interpolationStack;

  bool inTemplateString() const { return state == LexerState::TemplateString; }
};

} // namespace moksha
