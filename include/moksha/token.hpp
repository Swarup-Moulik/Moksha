#pragma once

#include <string>
#include <iostream>
#include <map>

namespace Moksha {

enum class TokenType {
    // --- Meta ---
    EOFT, ILLEGAL,

    // --- Literals ---
    IDENTIFIER, INTEGER_LITERAL, FLOAT_LITERAL, 
    STRING_LITERAL, STRING_CHUNK, CHAR_LITERAL, 
    BOOLEAN_LITERAL,

    // --- Template Strings ---
    BACKTICK, INTERPOLATION_START,

    // --- Operators: Arithmetic ---
    PLUS, MINUS, STAR, SLASH, 
    STAR_STAR, MODULO,
    PLUS_PLUS, MINUS_MINUS,

    // --- Operators: Assignment ---
    ASSIGN, PLUS_ASSIGN, MINUS_ASSIGN, STAR_ASSIGN, SLASH_ASSIGN,

    // --- Operators: Comparison & Logical ---
    EQ, NEQ, LT, GT, LTE, GTE,
    AND, OR, NOT,

    // --- Operators: Bitwise (NEW) ---
    BITWISE_OR,  // |
    BITWISE_XOR, // ^
    BITWISE_NOT, // ~
    LSHIFT,      // <<
    RSHIFT,      // >>
    // Note: AMPERSAND (&) is already defined under Systems & Memory but works as Bitwise AND

    // --- Operators: Systems & Memory ---
    AMPERSAND,        // & (Address-of / Bitwise AND)
    QUESTION_QUESTION, // ?? (Null Coalescing)

    // --- Operators: Functional & Structural ---
    ARROW,    // => (Lambda)
    ELLIPSIS, // ... (Spread)

    // --- Punctuation ---
    SEMICOLON, COMMA, COLON, DOT,
    LPAREN, RPAREN,    // ()
    LBRACE, RBRACE,    // {}
    LBRACKET, RBRACKET,// []
    QUESTION_MARK,     // ? (Ternary / Nullable Type)
    QUESTION_DOT,      // ?. (Optional Chaining)

    // --- Keywords: Declarations ---
    KW_IMPORT, KW_FROM, KW_CLASS, KW_NEW, KW_CAST,
    KW_MACRO, KW_CONSTRUCTOR, KW_CONST, KW_STRUCT, KW_ASM,
    KW_THIS, KW_NULL, KW_DESTRUCTOR, KW_ENUM, KW_PACKED,
    
    // Access Specifiers 
    KW_PUBLIC, KW_PRIVATE, KW_PROTECTED,
    
    // Operator Overloading 
    KW_OPERATOR,

    // --- Keywords: Systems Programming ---
    KW_REF,    // ref (Pass-by-reference / Ref Class)
    KW_UNSAFE, // unsafe { ... }
    
    // Shared Memory 
    KW_SHARED, // shared Type variable;

    // --- Keywords: Error Handling ---
    KW_TRY, KW_CATCH, KW_FINALLY, KW_THROW,

    // --- Keywords: Resource Management ---
    KW_DEFER, // defer close();
    KW_WITH,  // with (file) { ... }
    
    // Async/Await 
    KW_ASYNC, KW_AWAIT,

    // --- Keywords: Control Flow ---
    KW_IF, KW_ELSE, KW_ELSEIF, KW_RETURN,
    KW_WHILE, KW_FOR, KW_IN, KW_DO,
    KW_SWITCH, KW_CASE, KW_DEFAULT,
    KW_BREAK, KW_CONTINUE,

    // --- Keywords: Data Types ---
    KW_CHAR, KW_SHORT, KW_INT, 
    KW_LONG, KW_DOUBLE, KW_FLOAT,
    KW_BOOLEAN, KW_STRING,
    KW_ANY, KW_TABLE, KW_VOID,  

    // --- Keywords: Built-ins ---
    KW_PRINT, KW_PRINTLN, 
    KW_READ, KW_READINT, KW_READDOUBLE, KW_READBOOL,
    KW_DELETE, KW_LENGTH,

    // --- OS & Low-Level Keywords ---
    KW_VOLATILE,  // volatile
    KW_UNION,     // union
    KW_ALIGN,     // alignas / align
    KW_SECTION,   // section
    KW_EXTERN,    // extern (For calling C code)
    KW_SIZEOF,    // sizeof
    KW_INTERRUPT, // interrupt
    KW_STATIC,    // static
    KW_TYPEDEF,   // typedef
};

struct Token {
    TokenType type;
    std::string lexeme;
    int line;
    int column;

    static const std::map<TokenType, std::string> TokenNames;
    
    std::string toString() const {
        std::string typeName = TokenNames.count(type) ? TokenNames.at(type) : "UNKNOWN";
        return "{Type: " + typeName + 
               ", Lexeme: '" + lexeme + 
               "', Loc: " + std::to_string(line) + ":" + std::to_string(column) + "}";
    }
};

inline const std::map<TokenType, std::string> Token::TokenNames = {
    // --- Meta ---
    {TokenType::EOFT, "EOFT"}, 
    {TokenType::ILLEGAL, "ILLEGAL"},

    // --- Literals ---
    {TokenType::IDENTIFIER, "IDENTIFIER"}, 
    {TokenType::INTEGER_LITERAL, "INTEGER_LITERAL"}, 
    {TokenType::FLOAT_LITERAL, "FLOAT_LITERAL"}, 
    {TokenType::STRING_CHUNK, "STRING_CHUNK"}, 
    {TokenType::STRING_LITERAL, "STRING_LITERAL"},
    {TokenType::CHAR_LITERAL, "CHAR_LITERAL"}, 
    {TokenType::BOOLEAN_LITERAL, "BOOLEAN_LITERAL"},

    // --- Template Strings ---
    {TokenType::BACKTICK, "BACKTICK"}, 
    {TokenType::INTERPOLATION_START, "INTERPOLATION_START"},

    // --- Arithmetic ---
    {TokenType::PLUS, "PLUS"}, {TokenType::MINUS, "MINUS"}, 
    {TokenType::STAR, "STAR"}, {TokenType::SLASH, "SLASH"}, 
    {TokenType::STAR_STAR, "STAR_STAR"}, {TokenType::MODULO, "MODULO"}, 
    {TokenType::PLUS_PLUS, "PLUS_PLUS"}, {TokenType::MINUS_MINUS, "MINUS_MINUS"},

    // --- Assignment ---
    {TokenType::ASSIGN, "ASSIGN"}, 
    {TokenType::PLUS_ASSIGN, "PLUS_ASSIGN"}, {TokenType::MINUS_ASSIGN, "MINUS_ASSIGN"}, 
    {TokenType::STAR_ASSIGN, "STAR_ASSIGN"}, {TokenType::SLASH_ASSIGN, "SLASH_ASSIGN"},

    // --- Logic & Comparison ---
    {TokenType::EQ, "EQ"}, {TokenType::NEQ, "NEQ"}, 
    {TokenType::LT, "LT"}, {TokenType::GT, "GT"},
    {TokenType::LTE, "LTE"}, {TokenType::GTE, "GTE"}, 
    {TokenType::AND, "AND"}, {TokenType::OR, "OR"}, {TokenType::NOT, "NOT"}, 

    // --- Bitwise ---
    {TokenType::BITWISE_OR, "BITWISE_OR"},
    {TokenType::BITWISE_XOR, "BITWISE_XOR"},
    {TokenType::BITWISE_NOT, "BITWISE_NOT"},
    {TokenType::LSHIFT, "LSHIFT"},
    {TokenType::RSHIFT, "RSHIFT"},

    // --- Systems & Functional ---
    {TokenType::AMPERSAND, "AMPERSAND"}, 
    {TokenType::QUESTION_QUESTION, "NULL_COALESCING"},
    {TokenType::ARROW, "ARROW"}, 
    {TokenType::ELLIPSIS, "ELLIPSIS"},

    // --- Punctuation ---
    {TokenType::SEMICOLON, "SEMICOLON"}, {TokenType::COMMA, "COMMA"}, 
    {TokenType::COLON, "COLON"}, {TokenType::DOT, "DOT"}, 
    {TokenType::QUESTION_MARK, "QUESTION_MARK"}, {TokenType::QUESTION_DOT, "QUESTION_DOT"},
    {TokenType::LPAREN, "LPAREN"}, {TokenType::RPAREN, "RPAREN"}, 
    {TokenType::LBRACE, "LBRACE"}, {TokenType::RBRACE, "RBRACE"}, 
    {TokenType::LBRACKET, "LBRACKET"}, {TokenType::RBRACKET, "RBRACKET"},

    // --- Declarations ---
    {TokenType::KW_IMPORT, "IMPORT"}, {TokenType::KW_FROM, "FROM"}, {TokenType::KW_STRUCT, "STRUCT"},
    {TokenType::KW_CLASS, "CLASS"}, {TokenType::KW_MACRO, "MACRO"}, {TokenType::KW_CAST, "CAST"},
    {TokenType::KW_CONSTRUCTOR, "CONSTRUCTOR"}, {TokenType::KW_CONST, "CONST"}, {TokenType::KW_ASM, "ASM"},
    {TokenType::KW_NEW, "NEW"}, {TokenType::KW_THIS, "THIS"}, {TokenType::KW_ENUM, "ENUM"},
    {TokenType::KW_NULL, "NULL"}, {TokenType::KW_DESTRUCTOR, "DESTRUCTOR"}, {TokenType::KW_PACKED, "PACKED"},
    
    // Access Specifiers
    {TokenType::KW_PUBLIC, "PUBLIC"}, {TokenType::KW_PRIVATE, "PRIVATE"}, 
    {TokenType::KW_PROTECTED, "PROTECTED"},
    
    // Operator Keyword
    {TokenType::KW_OPERATOR, "OPERATOR"},

    // --- Systems Programming ---
    {TokenType::KW_REF, "REF"}, 
    {TokenType::KW_UNSAFE, "UNSAFE"},
    
    // Shared Keyword
    {TokenType::KW_SHARED, "SHARED"},

    // --- Error & Resource Handling ---
    {TokenType::KW_TRY, "TRY"}, {TokenType::KW_CATCH, "CATCH"}, 
    {TokenType::KW_FINALLY, "FINALLY"}, {TokenType::KW_THROW, "THROW"},
    {TokenType::KW_DEFER, "DEFER"}, {TokenType::KW_WITH, "WITH"},
    
    // Async/Await
    {TokenType::KW_ASYNC, "ASYNC"}, {TokenType::KW_AWAIT, "AWAIT"},

    // --- Control Flow ---
    {TokenType::KW_IF, "IF"}, {TokenType::KW_ELSE, "ELSE"}, {TokenType::KW_ELSEIF, "ELSEIF"}, 
    {TokenType::KW_RETURN, "RETURN"}, {TokenType::KW_WHILE, "WHILE"}, 
    {TokenType::KW_FOR, "FOR"}, {TokenType::KW_IN, "IN"}, {TokenType::KW_DO, "DO"},
    {TokenType::KW_SWITCH, "SWITCH"}, {TokenType::KW_CASE, "CASE"}, 
    {TokenType::KW_DEFAULT, "DEFAULT"},
    {TokenType::KW_BREAK, "BREAK"}, {TokenType::KW_CONTINUE, "CONTINUE"},

    // --- Types ---
    {TokenType::KW_INT, "INT_TYPE"}, {TokenType::KW_DOUBLE, "DOUBLE_TYPE"},
    {TokenType::KW_STRING, "STRING_TYPE"}, {TokenType::KW_BOOLEAN, "BOOLEAN_TYPE"},
    {TokenType::KW_ANY, "ANY_TYPE"}, {TokenType::KW_TABLE, "TABLE_TYPE"}, {TokenType::KW_VOID, "VOID_TYPE"},
    {TokenType::KW_CHAR, "CHAR_TYPE"},  
    {TokenType::KW_SHORT, "SHORT_TYPE"}, 
    {TokenType::KW_LONG, "LONG_TYPE"},   
    {TokenType::KW_FLOAT, "FLOAT_TYPE"},

    // --- Built-ins ---
    {TokenType::KW_PRINT, "PRINT"}, {TokenType::KW_PRINTLN, "PRINTLN"}, 
    {TokenType::KW_READ, "READ"}, {TokenType::KW_READINT, "READINT"}, 
    {TokenType::KW_READDOUBLE, "READDOUBLE"}, {TokenType::KW_READBOOL, "READBOOL"},
    {TokenType::KW_DELETE, "DELETE"}, {TokenType::KW_LENGTH, "LENGTH"},

    // --- OS & Low-Level ---
    {TokenType::KW_VOLATILE, "VOLATILE"}, 
    {TokenType::KW_UNION, "UNION"},
    {TokenType::KW_ALIGN, "ALIGN"},
    {TokenType::KW_SECTION, "SECTION"},
    {TokenType::KW_EXTERN, "EXTERN"},
    {TokenType::KW_SIZEOF, "SIZEOF"},
    {TokenType::KW_INTERRUPT, "INTERRUPT"},
    {TokenType::KW_STATIC, "STATIC"},
    {TokenType::KW_TYPEDEF, "TYPEDEF"},
};

} // namespace Moksha