#pragma once

#include "moksha/token.hpp"
#include <string>
#include <map>
#include <vector>

namespace Moksha {

enum class LexerMode {
    NORMAL,
    IN_BACKTICK_STRING,
    IN_INTERPOLATION_EXPRESSION
};

class Lexer {
private:
    std::string sourceCode;
    size_t currentPos;
    int currentLine;
    int currentCol;
    int startCol; 
    
    std::vector<LexerMode> modeStack;
    static const std::map<std::string, TokenType> keywords;

    char nextChar();
    
    void backup(); 
    
    void pushMode(LexerMode mode);
    void popMode();
    LexerMode getMode() const;

    // CHANGED: These now return bool (false = error/unterminated)
    bool skipMultiLineComment();
    bool skipWhitespaceAndComments(); 
    
    Token readIdentifier();       
    Token readNumber();
    Token readCharLiteral();
    Token readStringLiteral();    
    Token readTemplateStringBody(); 
    char readHexEscape();

public:
    Lexer(const std::string& source); 
    Token nextToken(); 
    char peekChar(int offset = 0) const;
};

} // namespace Moksha