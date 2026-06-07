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
  // --- Control & Diagnostics ---
  Eof,     // End of file / input stream
  Error,   // Lexer error (invalid character/sequence)
  Comment, // Line (`//`) or Block (`/* */`) comment

  // --- Identifiers ---
  Identifier, // Variable, function, or type names (e.g., `myVar`)

  // --- Literals ---
  IntegerLiteral, // Whole numbers (e.g., `42`, `0xFF`)
  FloatLiteral,   // IEEE 754 floating point (e.g., `3.14`, `1e10`)
  DecimalLiteral, // Exact base-10 decimals (e.g., `1.23d`)
  StringLiteral,  // Standard string (e.g., `"Hello"`)
  TemplateString, // Full template string (e.g., `\`Value: ${x}\``)
  CharLiteral,    // Single character (e.g., `'a'`)

  // --- String Interpolation Components ---
  StringFragment,     // Text between interpolations inside a template string
  InterpolationStart, // `${` marking the start of an expression in a string
  InterpolationEnd,   // `}` marking the end of a string interpolation

  // --- Core Keywords ---
  KwClass,       // `class` (Stack-allocated value type)
  KwOperator,    // `operator` (Operator overloading)
  KwThread,      // `thread` (OS-level thread creation)
  KwRef,         // `ref` (Heap-allocated reference type / pass-by-reference)
  KwWeak,        // `weak` (Non-owning ARC reference)
  KwShared,      // `shared` (Explicit ARC alias creation)
  KwNew,         // `new` (Heap allocation / Object instantiation)
  KwDelete,      // `delete` (Manual memory deallocation, for future)
  KwUnsafe,      // `unsafe` (Bypass memory/borrow safety checks)
  KwUnsigned,    // `unsigned` (Unsigned integer modifier)
  KwConstructor, // `constructor` (Object initializer)
  KwDestructor,  // `destructor` (Object cleanup/RAII hook)
  KwThis,        // `this` (Current instance pointer)
  KwSuper,       // `super` (Parent class instance)
  KwNull,        // `null` (Null pointer/reference)

  // --- Control Flow ---
  KwIf,       // `if`
  KwElse,     // `else`
  KwWhile,    // `while`
  KwDo,       // `do`
  KwFor,      // `for`
  KwIn,       // `in` (For-in loop iteration also inline asm contraint)
  KwSwitch,   // `switch` (Pattern matching / jump tables)
  KwCase,     // `case`
  KwDefault,  // `default`
  KwReturn,   // `return`
  KwBreak,    // `break`
  KwContinue, // `continue`
  KwDefer,    // `defer` (LIFO block execution on scope exit)
  KwWith,     // `with` (Context management)

  // --- Exceptions ---
  KwTry,     // `try`
  KwCatch,   // `catch`
  KwThrow,   // `throw` (Zero-cost exception trigger)
  KwFinally, // `finally`

  // --- Primitive Types ---
  KwVoid,    // `void` (No return type)
  KwInt,     // `int` (32-bit signed)
  KwFloat,   // `float` (32-bit float)
  KwDouble,  // `double` (64-bit float)
  KwBool,    // `bool`
  KwString,  // `string`
  KwChar,    // `char` (8-bit ASCII/UTF-8 byte)
  KwShort,   // `short` (16-bit signed)
  KwLong,    // `long` (64-bit signed)
  KwISize,   // `isize` (Pointer-sized signed integer)
  KwUSize,   // `usize` (Pointer-sized unsigned integer)
  KwHalf,    // `half` (16-bit AI float / f16)
  KwQuarter, // `quarter` (8-bit AI float / f8 E5M2)
  KwDecimal, // `decimal` (Fixed-precision base-10)
  KwAny,     // `any` (Dynamic type / void* equivalent)

  // --- Type Operations ---
  KwAs,    // `as` (Import Alias)
  KwTrue,  // `true`
  KwFalse, // `false`

  // --- Modifiers & Visibility ---
  KwPublic,    // `public`
  KwPrivate,   // `private`
  KwProtected, // `protected`
  KwPromise,   // `promise` (Async task handle)
  KwConst,     // `const` (Immutable value)
  KwCast,      // `cast` (Functional/Template type conversion)
  KwBitcast,   // `bitcast` (Raw memory reinterpretation)
  KwStatic,    // `static` (Global/Class-level lifetime)

  // --- Concurrency & Modules ---
  KwAsync,  // `async` (Coroutine state-machine modifier)
  KwAwait,  // `await` (Coroutine suspension point)
  KwImport, // `import` (Module inclusion)
  KwFrom,   // `from` (Module path specifier)
  KwMacro,  // `macro` (AST-level hygienic macro)

  // --- Advanced Data Structures ---
  KwTable,   // `table` (Hash map collection)
  KwEnum,    // `enum`
  KwGeneric, // `generic` (Type templates)
  KwStruct,  // `struct` (C-compatible data layout)
  KwUnion,   // `union` (C-compatible memory overlay)

  // --- Low-Level / Systems / Memory ---
  KwPacked,    // `packed` (Remove padding from structs)
  KwVolatile,  // `volatile` (Prevent optimizer reordering/elision)
  KwAlign,     // `align` (Force memory boundary alignment)
  KwSizeof,    // `sizeof` (Compile-time size evaluation)
  KwInterrupt, // `interrupt` (ISR calling convention)
  KwNaked,     // `naked` (No compiler prologue/epilogue)
  KwNoreturn,  // `noreturn` (Function never returns to caller)
  KwNoinline,  // `noinline` (Prevent inlining)
  KwInline,    // `inline` (Force/suggest inlining)
  KwPure,      // `pure` (No side effects, memoizable)
  KwCold,      // `cold` (Optimize for rarely executed paths)

  // --- Borrow Checker & Qualifiers ---
  KwLock, // `lock` (Thread-synchronized memory qualifier)
  KwView, // `view` (Immutable borrow)
  KwMut,  // `mut` (Mutable borrow/pointer)

  // --- OOP & Closures ---
  KwVirtual,  // `virtual` (Dynamic VTable dispatch)
  KwOverride, // `override` (Subclass method replacement)
  KwInput,    // `input` (Standard input reading)
  KwClosure,  // `closure` (Fat pointer / lambda type)
  KwMove,     // `move` (Transfer ownership into closure)

  // --- Assembly / Hardware ---
  KwOut,         // `out` (Inline asm output constraint)
  KwInout,       // `inout` (Inline asm input/output constraint)
  KwClobber,     // `clobber` (Inline asm register invalidation)
  KwExtern,      // `extern` (Foreign Function Interface / FFI linkage)
  KwUsing,       // `using` (Type aliasing)
  KwAsm,         // `asm` (Inline assembly block)
  KwSection,     // `section` (Linker script section placement)
  KwUsed,        // `used` (Prevent dead-code elimination)
  KwThreadLocal, // `thread_local` (TLS memory storage)

  // --- Mathematical & Logical Operators ---
  Plus,       // `+`
  Minus,      // `-`
  Star,       // `*`
  Slash,      // `/`
  Percent,    // `%` (Modulo)
  Power,      // `**` (Exponentiation)
  PlusPlus,   // `++` (Increment)
  MinusMinus, // `--` (Decrement)

  // --- Assignment Operators ---
  Equal,        // `=`
  PlusEqual,    // `+=`
  MinusEqual,   // `-=`
  StarEqual,    // `*=`
  SlashEqual,   // `/=`
  PercentEqual, // `%=`

  // --- Bitwise & Pointers ---
  Amp,                 // `&` (Bitwise AND / Address-of / Reference)
  Pipe,                // `|` (Bitwise OR)
  Caret,               // `^` (Bitwise XOR)
  Tilde,               // `~` (Bitwise NOT)
  AmpEqual,            // `&=`
  PipeEqual,           // `|=`
  CaretEqual,          // `^=`
  LessLess,            // `<<` (Bitwise Left Shift)
  GreaterGreater,      // `>>` (Bitwise Right Shift / Generic closing)
  LessLessEqual,       // `<<=`
  GreaterGreaterEqual, // `>>=`

  // --- Logical Operators ---
  AmpAmp,      // `&&` (Logical AND)
  PipePipe,    // `||` (Logical OR)
  PipeGreater, // `|>` (Pipeline / Forward function application)
  Bang,        // `!` (Logical NOT)

  // --- Relational Operators ---
  EqualEqual,   // `==`
  NotEqual,     // `!=`
  Less,         // `<`
  LessEqual,    // `<=`
  Greater,      // `>`
  GreaterEqual, // `>=`

  // --- Structural / Syntax Operators ---
  FatArrow,         // `=>` (Lambda/Closure definition)
  Arrow,            // `->` (Lambda/Closure return type specifier)
  Dot,              // `.` (Member access)
  DotDotDot,        // `...` (Spread operator)
  QuestionDot,      // `?.` (Safe navigation / Optional chaining)
  QuestionQuestion, // `??` (Null coalescing)
  Question,         // `?` (Ternary / Nullable type modifier)
  Colon,            // `:` (Ternary else)
  Comma,            // `,`
  Semicolon,        // `;`

  // --- Brackets & Grouping ---
  LParen,   // `(`
  RParen,   // `)`
  LBrace,   // `{`
  RBrace,   // `}`
  LBracket, // `[`
  RBracket  // `]`
};

/// @brief Represents explicit type suffixes attached to numeric literals.
/// @note Used by the Lexer to force the TypeChecker to bypass implicit
///       type inference (e.g., forcing `10` to be treated as `10_u8`).
enum class NumericSuffix : uint8_t {
  None,

  // --- Signed Integers ---
  i8,
  i16,
  i32,
  i64,
  isize, ///< Architecture-dependent signed pointer size

  // --- Unsigned Integers ---
  u8,
  u16,
  u32,
  u64,
  usize, ///< Architecture-dependent unsigned pointer size

  // --- Floating Point ---
  f8,  ///< 8-bit Quarter precision (E5M2) for AI workloads
  f16, ///< 16-bit Half precision
  f32,
  f64,

  // --- Specialized ---
  d ///< Base-10 exact decimal representation (e.g., 1.23d)
};

/// @brief Bitmask flags to attach metadata to Tokens without increasing struct
/// size.
/// @details Flags are stored in a bitmask. Use bitwise AND/OR to check or set
/// flags.
enum TokenFlag : uint16_t {
  TF_None = 0,

  // --- Number Formats ---
  TF_IsHex = 1 << 0,   ///< Literal was parsed in base-16 (0x)
  TF_IsBin = 1 << 1,   ///< Literal was parsed in base-2 (0b)
  TF_IsOctal = 1 << 2, ///< Literal was parsed in base-8 (0o)

  // --- String Semantics ---

  /// @brief Indicates the string literal contains escape sequences (e.g. \n,
  /// \t).
  /// @note If this flag is FALSE, the parser can use zero-copy `StringView`
  ///       directly into the source buffer. If TRUE, the string must be
  ///       allocated.
  TF_IsEscaped = 1 << 3,

  /// @brief Indicates a raw string where escapes are ignored (e.g., C:\dev).
  TF_IsRaw = 1 << 4,

  // --- AST & Macro Hygiene ---

  /// @brief Indicates this token was generated by the compiler or macro
  /// expander,
  ///        NOT written by the user in the source file.
  /// @note Crucial for macro hygiene and ensuring error messages point to the
  ///       correct original macro invocation, not the generated code.
  TF_IsSynthesized = 1 << 5,
};

/**
 * @brief Represents a single atomic unit of source code.
 *
 * Tokens are designed to be lightweight (POD-like). The location holds a
 * pointer into the original source buffer, ensuring zero-copy tokenization. The
 * parser consumes these tokens to build the AST.
 */
class Token {
public:
  TokenKind kind; ///< The categorical type (e.g., IntegerLiteral, KwIf)
  SourceLocation location; ///< Start pointer in the raw source buffer
  uint32_t length;         ///< Length of the token in bytes

  /// @brief Type suffix for numeric literals (e.g., _i8, _f16, _d)
  NumericSuffix suffix = NumericSuffix::None;

  /// @brief Bitmask for token metadata (e.g., TF_IsHex, TF_IsEscaped)
  uint16_t flags = TF_None;

  Token() : kind(TokenKind::Error), length(0) {}
  Token(TokenKind kind, SourceLocation loc, uint32_t len)
      : kind(kind), location(loc), length(len) {}

  /// @brief Returns the raw string representation of the token.
  /// @note This is a zero-copy operation; it creates a view, not a new string.
  llvm::StringRef getSpelling() const {
    return llvm::StringRef(location.getPointer(), length);
  }

  /// @brief Returns true if token matches the specified kind.
  bool is(TokenKind k) const { return kind == k; }

  /// @brief Returns true if token does NOT match the specified kind.
  bool isNot(TokenKind k) const { return kind != k; }

  /// @brief Checks if the token is any of the provided TokenKinds.
  /// @example if (tok.isAny(TokenKind::KwIf, TokenKind::KwWhile)) { ... }
  template <typename... T> bool isAny(TokenKind k, T... args) const {
    return is(k) || isAny(args...);
  }

  bool isAny(TokenKind k) const { return is(k); }
};

/**
 * @brief Performs lexical analysis on a source buffer, converting raw text
 * into a stream of Token objects.
 * * The Lexer maintains an internal state machine to handle context-sensitive
 * features like template string interpolation and nested block comments.
 */
class Lexer {
public:
  /// @brief Initialize lexer with a buffer and diagnostic reporting engine.
  explicit Lexer(llvm::StringRef buffer, DiagnosticEngine &Diags);

  /// @brief Scans the next token from the input stream.
  Token next();

  /// @brief Returns the current source location for diagnostic reporting.
  SourceLocation getLoc() const;

private:
  DiagnosticEngine &Diags;

  const char *bufferStart; ///< Pointer to the start of the entire input buffer
  const char *bufferEnd; ///< Pointer to the end (sentinel) of the input buffer
  const char *curPtr;    ///< Current scanning cursor

  /// @brief Tracks the start of the current template string fragment for
  /// interpolation.
  const char *fragmentStart = nullptr;

  // --- Scanning Methods ---
  Token scanIdentifier();
  Token scanNumber();
  Token scanString();
  Token scanChar();
  Token scanOperator();

  // --- Helper Methods ---
  void skipWhitespace();
  bool skipComment();   ///< Handles line comments and nested block comments
  bool advanceUTF8();   ///< Advances the cursor by one valid UTF-8 code point
  bool consumeEscape(); ///< Parses and validates string/char escape sequences
                        ///< (\n, \x, \uXXXX, \u{...}, \UXXXXXXXX)

  /// @brief Peeks at the Nth character ahead without advancing the cursor.
  char peek(int n = 0) const;

  /// @brief Helper to generate an error token at a specific source position.
  Token errorAt(const char *start) {
    size_t len = std::max<size_t>(1, curPtr - start);
    return Token(TokenKind::Error, SourceLocation::getFromPointer(start), len);
  }

  // --- State Management ---
  enum class LexerState { Normal, TemplateString };

  /// @brief Tracks brace nesting level inside template string interpolation
  /// (e.g., `${ {braceBalance} }`).
  struct InterpolationState {
    int braceBalance = 0;
  };

  LexerState state = LexerState::Normal;
  std::vector<InterpolationState> interpolationStack;

  /// @brief Returns true if the lexer is currently inside a template string
  /// context.
  bool inTemplateString() const { return state == LexerState::TemplateString; }
};

} // namespace moksha
