#include "moksha/Lexer.hpp"
#include "moksha/token.hpp"
#include <cctype>
#include <stdexcept>
#include <iostream>

namespace Moksha
{

    // --- 1. Static Member Initialization (Optimization) ---
    // We define the map here so it is created only once for the entire program,
    // not every time you create a Lexer instance.
    const std::map<std::string, TokenType> Lexer::keywords = {
        // --- Control Flow ---
        {"import", TokenType::KW_IMPORT},
        {"from", TokenType::KW_FROM},
        {"if", TokenType::KW_IF},
        {"else", TokenType::KW_ELSE},
        {"elseif", TokenType::KW_ELSEIF},
        {"return", TokenType::KW_RETURN},
        {"while", TokenType::KW_WHILE},
        {"for", TokenType::KW_FOR},
        {"in", TokenType::KW_IN},
        {"do", TokenType::KW_DO},
        {"switch", TokenType::KW_SWITCH},
        {"case", TokenType::KW_CASE},
        {"default", TokenType::KW_DEFAULT},
        {"break", TokenType::KW_BREAK},
        {"continue", TokenType::KW_CONTINUE},

        // --- OO & Structure ---
        {"class", TokenType::KW_CLASS},
        {"struct", TokenType::KW_STRUCT},
        {"macro", TokenType::KW_MACRO},
        {"constructor", TokenType::KW_CONSTRUCTOR},
        {"new", TokenType::KW_NEW},
        {"asm", TokenType::KW_ASM},
        {"packed", TokenType::KW_PACKED},
        {"this", TokenType::KW_THIS},
        {"operator", TokenType::KW_OPERATOR},
        {"destructor", TokenType::KW_DESTRUCTOR},
        {"enum", TokenType::KW_ENUM},
        {"cast", TokenType::KW_CAST},

        // Access Specifiers
        {"public", TokenType::KW_PUBLIC},
        {"private", TokenType::KW_PRIVATE},
        {"protected", TokenType::KW_PROTECTED},

        // --- Types ---
        {"int", TokenType::KW_INT},
        {"double", TokenType::KW_DOUBLE},
        {"string", TokenType::KW_STRING},
        {"boolean", TokenType::KW_BOOLEAN},
        {"any", TokenType::KW_ANY},
        {"table", TokenType::KW_TABLE},
        {"const", TokenType::KW_CONST},
        {"null", TokenType::KW_NULL},
        {"void", TokenType::KW_VOID},
        {"true", TokenType::BOOLEAN_LITERAL},
        {"false", TokenType::BOOLEAN_LITERAL},
        {"char", TokenType::KW_CHAR},   
        {"short", TokenType::KW_SHORT}, 
        {"long", TokenType::KW_LONG}, 
        {"float", TokenType::KW_FLOAT},  

        // --- Built-ins ---
        {"print", TokenType::KW_PRINT},
        {"println", TokenType::KW_PRINTLN},
        {"read", TokenType::KW_READ},
        {"readInt", TokenType::KW_READINT},
        {"readDouble", TokenType::KW_READDOUBLE},
        {"readBool", TokenType::KW_READBOOL},
        {"delete", TokenType::KW_DELETE},
        {"length", TokenType::KW_LENGTH},

        // --- Systems Programming ---
        {"ref", TokenType::KW_REF},
        {"shared", TokenType::KW_SHARED},
        {"unsafe", TokenType::KW_UNSAFE},

        // --- Async Programming ---
        {"async", TokenType::KW_ASYNC},
        {"await", TokenType::KW_AWAIT},

        // --- Error Handling ---
        {"try", TokenType::KW_TRY},
        {"catch", TokenType::KW_CATCH},
        {"finally", TokenType::KW_FINALLY},
        {"throw", TokenType::KW_THROW},

        // --- Resource Management ---
        {"defer", TokenType::KW_DEFER},
        {"with", TokenType::KW_WITH},

        // --- OS & Low-Level ---
        {"volatile", TokenType::KW_VOLATILE}, 
        {"union", TokenType::KW_UNION},       
        {"align", TokenType::KW_ALIGN},       
        {"section", TokenType::KW_SECTION},   
        {"extern", TokenType::KW_EXTERN},   
        {"sizeof", TokenType::KW_SIZEOF},
        {"interrupt", TokenType::KW_INTERRUPT},
        {"static", TokenType::KW_STATIC},
        {"typedef", TokenType::KW_TYPEDEF},  
    };

    // --- 2. Constructor ---
    Lexer::Lexer(const std::string &source)
        : sourceCode(source),
          currentPos(0),
          currentLine(1),
          currentCol(1),
          startCol(1)
    {
        // Initialize the stack with the default mode
        modeStack.push_back(LexerMode::NORMAL);
    }

    // --- 3. Mode Management (Stack Implementation) ---

    void Lexer::pushMode(LexerMode mode)
    {
        modeStack.push_back(mode);
    }

    void Lexer::popMode()
    {
        if (!modeStack.empty())
        {
            modeStack.pop_back();
        }
        // Safety: ensure we always have at least NORMAL mode
        if (modeStack.empty())
        {
            modeStack.push_back(LexerMode::NORMAL);
        }
    }

    LexerMode Lexer::getMode() const
    {
        if (modeStack.empty())
            return LexerMode::NORMAL;
        return modeStack.back();
    }

    // --- 4. Basic Character Operations ---

    char Lexer::nextChar()
    {
        if (currentPos >= sourceCode.length())
            return '\0';
        char c = sourceCode[currentPos++];
        if (c == '\n')
        {
            currentLine++;
            currentCol = 1;
        }
        else
        {
            currentCol++;
        }
        return c;
    }

    char Lexer::readHexEscape()
    {
        nextChar(); // consume 'x'
        char h1 = nextChar();
        char h2 = nextChar();

        // Helper lambda locally or logic
        auto hexVal = [](char c)
        {
            if (c >= '0' && c <= '9')
                return c - '0';
            if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            return 0;
        };

        return (char)((hexVal(h1) << 4) | hexVal(h2));
    }

    char Lexer::peekChar(int offset) const
    {
        if (currentPos + offset >= sourceCode.length())
            return '\0';
        return sourceCode[currentPos + offset];
    }

    void Lexer::backup()
    {
        if (currentPos > 0)
        {
            currentPos--;
            if (sourceCode[currentPos] == '\n')
            {
                currentLine--;
                currentCol = 0; // Approximation
            }
            else
            {
                currentCol--;
            }
        }
    }

    // --- 5. Skippers ---

    bool Lexer::skipMultiLineComment()
    {
        while (currentPos < sourceCode.length())
        {
            if (peekChar() == '*' && peekChar(1) == '/')
            {
                nextChar();
                nextChar(); // Consume */
                return true;
            }
            nextChar();
        }
        return false;
    }

    bool Lexer::skipWhitespaceAndComments()
    {
        while (currentPos < sourceCode.length())
        {
            char c = peekChar();
            if (std::isspace(c))
            {
                nextChar();
            }
            else if (c == '/' && peekChar(1) == '/')
            {
                while (currentPos < sourceCode.length() && peekChar() != '\n')
                {
                    nextChar();
                }
            }
            else if (c == '/' && peekChar(1) == '*')
            {
                nextChar();
                nextChar(); // Consume /*
                if (!skipMultiLineComment())
                {
                    return false; // Propagate error
                }
            }
            else
            {
                break;
            }
        }
        return true;
    }

    // --- 6. Token Readers ---

    Token Lexer::readIdentifier()
    {
        startCol = currentCol;
        std::string lexeme = "";

        while (std::isalnum(peekChar()) || peekChar() == '_')
        {
            lexeme += nextChar();
        }

        TokenType type = TokenType::IDENTIFIER;
        // Uses the static map
        if (keywords.count(lexeme))
        {
            type = keywords.at(lexeme);
        }

        return {type, lexeme, currentLine, startCol - 1};
    }

    Token Lexer::readNumber()
    {
        startCol = currentCol;
        std::string lexeme = "";
        TokenType type = TokenType::INTEGER_LITERAL;

        if (peekChar() == '0')
        {
            char next = peekChar(1);

            // 1. Hexadecimal Handling
            if (next == 'x' || next == 'X')
            {
                lexeme += nextChar(); // consume '0'
                lexeme += nextChar(); // consume 'x'

                bool hasDigits = false;
                // isxdigit checks 0-9, a-f, A-F
                while (std::isxdigit(peekChar()))
                {
                    lexeme += nextChar();
                    hasDigits = true;
                }

                if (!hasDigits)
                {
                    return {TokenType::ILLEGAL, "Malformed hexadecimal literal", currentLine, startCol - 1};
                }
                return {TokenType::INTEGER_LITERAL, lexeme, currentLine, startCol - 1};
            }

            // 2. Binary Handling
            if (next == 'b' || next == 'B')
            {
                lexeme += nextChar(); // consume '0'
                lexeme += nextChar(); // consume 'b'

                bool hasDigits = false;
                while (peekChar() == '0' || peekChar() == '1')
                {
                    lexeme += nextChar();
                    hasDigits = true;
                }

                if (!hasDigits)
                {
                    return {TokenType::ILLEGAL, "Malformed binary literal", currentLine, startCol - 1};
                }
                return {TokenType::INTEGER_LITERAL, lexeme, currentLine, startCol - 1};
            }
        }

        // Standard integer and double handling

        while (std::isdigit(peekChar()))
        {
            lexeme += nextChar();
        }

        if (peekChar() == '.' && std::isdigit(peekChar(1)))
        {
            nextChar(); // consume dot
            lexeme += '.';
            type = TokenType::FLOAT_LITERAL;

            while (std::isdigit(peekChar()))
            {
                lexeme += nextChar();
            }
        }

        if (peekChar() == 'e' || peekChar() == 'E')
        {
            char sign = peekChar(1);
            // Handle optional sign: 1.5e-10 or 1.5e+10
            if (sign == '+' || sign == '-')
            {
                // Need to check if the char AFTER the sign is a digit
                if (std::isdigit(peekChar(2)))
                {
                    nextChar(); // consume 'e'
                    nextChar(); // consume sign
                    lexeme += 'e';
                    lexeme += sign;
                    type = TokenType::FLOAT_LITERAL;
                    while (std::isdigit(peekChar()))
                        lexeme += nextChar();
                }
            }
            else if (std::isdigit(sign))
            {
                nextChar(); // consume 'e'
                lexeme += 'e';
                type = TokenType::FLOAT_LITERAL;
                while (std::isdigit(peekChar()))
                    lexeme += nextChar();
            }
        }

        return {type, lexeme, currentLine, startCol - 1};
    }

    Token Lexer::readCharLiteral()
    {
        startCol = currentCol;
        nextChar(); // Consume opening '

        char content;

        if (peekChar() == '\\')
        {
            nextChar();                // Consume backslash
            char escaped = peekChar(); // Do not consume yet

            // 1. Handle Null Terminator (CRITICAL for OS)
            if (escaped == '0')
            {
                content = '\0';
                nextChar();
            }
            // 2. Handle Hex Escapes (e.g., '\xFF')
            else if (escaped == 'x')
            {
                content = readHexEscape();
            }
            // 3. Standard Escapes
            else
            {
                nextChar(); // consume the char
                if (escaped == 'n')
                    content = '\n';
                else if (escaped == 't')
                    content = '\t';
                else if (escaped == 'r')
                    content = '\r';
                else if (escaped == '\\')
                    content = '\\';
                else if (escaped == '\'')
                    content = '\'';
                else if (escaped == '"')
                    content = '"';
                else
                    content = escaped;
            }
        }
        else
        {
            content = peekChar();
            nextChar();
        }

        if (peekChar() != '\'')
        {
            return {TokenType::ILLEGAL, "Unclosed character literal", currentLine, startCol};
        }
        nextChar(); // Consume closing '

        return {TokenType::CHAR_LITERAL, std::string(1, content), currentLine, startCol - 1};
    }

    Token Lexer::readStringLiteral()
    {
        startCol = currentCol - 1;
        std::string lexeme = "";

        char content;

        while (currentPos < sourceCode.length())
        {
            char c = peekChar();

            if (c == '"')
                break;

            if (peekChar() == '\\')
            {
                nextChar();                // Consume backslash
                char escaped = peekChar(); // Do not consume yet

                // 1. Handle Null Terminator (CRITICAL for OS)
                if (escaped == '0')
                {
                    content = '\0';
                    nextChar();
                }
                // 2. Handle Hex Escapes (e.g., '\xFF')
                else if (escaped == 'x')
                {
                    content = readHexEscape();
                }
                // 3. Standard Escapes
                else
                {
                    nextChar(); // consume the char
                    if (escaped == 'n')
                        content = '\n';
                    else if (escaped == 't')
                        content = '\t';
                    else if (escaped == 'r')
                        content = '\r';
                    else if (escaped == '\\')
                        content = '\\';
                    else if (escaped == '\'')
                        content = '\'';
                    else if (escaped == '"')
                        content = '"';
                    else
                        content = escaped;
                }
                lexeme += content;
                continue;
            }

            lexeme += nextChar();
        }

        if (peekChar() != '"')
        {
            return {TokenType::ILLEGAL, "Unterminated string literal", currentLine, startCol};
        }

        nextChar();
        return {TokenType::STRING_LITERAL, lexeme, currentLine, startCol};
    }

    Token Lexer::readTemplateStringBody()
    {
        startCol = currentCol;
        std::string lexeme = "";

        char content;

        // Safety check: ensure we don't read past EOF
        while (currentPos < sourceCode.length())
        {
            char c = peekChar();

            // Stop at backtick OR start of interpolation
            if (c == '`' || (c == '$' && peekChar(1) == '{'))
            {
                break;
            }

            if (c == '\0')
            {
                return {TokenType::ILLEGAL, "Unterminated template string", currentLine, startCol};
            }

            if (peekChar() == '\\')
            {
                nextChar();                // Consume backslash
                char escaped = peekChar(); // Do not consume yet

                // 1. Handle Null Terminator (CRITICAL for OS)
                if (escaped == '0')
                {
                    content = '\0';
                    nextChar();
                }
                // 2. Handle Hex Escapes (e.g., '\xFF')
                else if (escaped == 'x')
                {
                    content = readHexEscape();
                }
                // 3. Standard Escapes
                else
                {
                    nextChar(); // consume the char
                    if (escaped == 'n')
                        content = '\n';
                    else if (escaped == 't')
                        content = '\t';
                    else if (escaped == 'r')
                        content = '\r';
                    else if (escaped == '\\')
                        content = '\\';
                    else if (escaped == '\'')
                        content = '\'';
                    else if (escaped == '"')
                        content = '"';
                    else
                        content = escaped;
                }
                lexeme += content;
                continue;
            }

            lexeme += nextChar();
        }

        return {TokenType::STRING_CHUNK, lexeme, currentLine, startCol};
    }

    // --- 7. Main Token Logic (Updated for Stack Mode) ---

    Token Lexer::nextToken()
    {
        LexerMode currentMode = getMode();

        // 1. Handle Template Strings
        if (currentMode == LexerMode::IN_BACKTICK_STRING)
        {
            char c = peekChar();

            // A. Closing Backtick
            if (c == '`')
            {
                nextChar();
                popMode();
                return {TokenType::BACKTICK, "`", currentLine, currentCol - 1};
            }

            // B. Interpolation Start
            if (c == '$' && peekChar(1) == '{')
            {
                startCol = currentCol;
                nextChar();
                nextChar(); // Consume ${
                pushMode(LexerMode::IN_INTERPOLATION_EXPRESSION);
                return {TokenType::INTERPOLATION_START, "${", currentLine, startCol};
            }

            // C. End of File (Error)
            if (c == '\0')
            {
                return {TokenType::ILLEGAL, "Unterminated template string", currentLine, currentCol};
            }

            return readTemplateStringBody();
        }

        // 2. Handle Interpolation End
        if (currentMode == LexerMode::IN_INTERPOLATION_EXPRESSION)
        {
            if (!skipWhitespaceAndComments())
                return {TokenType::ILLEGAL, "Unterminated multi-line comment", currentLine, currentCol};
            if (peekChar() == '}')
            {
                startCol = currentCol;
                nextChar();
                popMode();
                return {TokenType::RBRACE, "}", currentLine, startCol};
            }
        }

        // 3. Normal Mode + Whitespace Skip
        if (currentMode == LexerMode::NORMAL || currentMode == LexerMode::IN_INTERPOLATION_EXPRESSION)
        {
            if (!skipWhitespaceAndComments())
                return {TokenType::ILLEGAL, "Unterminated multi-line comment", currentLine, currentCol};
        }

        if (currentPos >= sourceCode.length())
        {
            return {TokenType::EOFT, "", currentLine, currentCol};
        }

        startCol = currentCol;
        char c = nextChar();

        // Identifiers
        if (std::isalpha(c) || c == '_')
        {
            backup();
            return readIdentifier();
        }

        // Numbers
        if (std::isdigit(c))
        {
            backup();
            return readNumber();
        }

        switch (c)
        {
        case '`':
            pushMode(LexerMode::IN_BACKTICK_STRING);
            return {TokenType::BACKTICK, "`", currentLine, startCol};
        case '"':
            return readStringLiteral();
        case '\'':
            backup();
            return readCharLiteral();

        // --- Arithmetic ---
        case '+':
            if (peekChar() == '=')
            {
                nextChar();
                return {TokenType::PLUS_ASSIGN, "+=", currentLine, startCol};
            }
            if (peekChar() == '+')
            {
                nextChar();
                return {TokenType::PLUS_PLUS, "++", currentLine, startCol};
            }
            return {TokenType::PLUS, "+", currentLine, startCol};
        case '-':
            if (peekChar() == '=')
            {
                nextChar();
                return {TokenType::MINUS_ASSIGN, "-=", currentLine, startCol};
            }
            if (peekChar() == '-')
            {
                nextChar();
                return {TokenType::MINUS_MINUS, "--", currentLine, startCol};
            }
            return {TokenType::MINUS, "-", currentLine, startCol};
        case '*':
            if (peekChar() == '*')
            {
                nextChar();
                return {TokenType::STAR_STAR, "**", currentLine, startCol};
            }
            if (peekChar() == '=')
            {
                nextChar();
                return {TokenType::STAR_ASSIGN, "*=", currentLine, startCol};
            }
            return {TokenType::STAR, "*", currentLine, startCol};
        case '/':
            if (peekChar() == '=')
            {
                nextChar();
                return {TokenType::SLASH_ASSIGN, "/=", currentLine, startCol};
            }
            return {TokenType::SLASH, "/", currentLine, startCol};
        case '%':
            return {TokenType::MODULO, "%", currentLine, startCol};

        // --- Punctuation ---
        case ';':
            return {TokenType::SEMICOLON, ";", currentLine, startCol};
        case ',':
            return {TokenType::COMMA, ",", currentLine, startCol};
        case ':':
            return {TokenType::COLON, ":", currentLine, startCol};
        case '.':
            if (peekChar() == '.' && peekChar(1) == '.')
            {
                nextChar();
                nextChar();
                return {TokenType::ELLIPSIS, "...", currentLine, startCol};
            }
            if (std::isdigit(peekChar()))
            {
                backup();
                return readNumber();
            }
            return {TokenType::DOT, ".", currentLine, startCol};
        case '(':
            return {TokenType::LPAREN, "(", currentLine, startCol};
        case ')':
            return {TokenType::RPAREN, ")", currentLine, startCol};
        case '{':
            return {TokenType::LBRACE, "{", currentLine, startCol};
        case '}':
            return {TokenType::RBRACE, "}", currentLine, startCol};
        case '[':
            return {TokenType::LBRACKET, "[", currentLine, startCol};
        case ']':
            return {TokenType::RBRACKET, "]", currentLine, startCol};

        // --- Bitwise Shifts & Comparisons ---
        case '<':
            if (peekChar() == '<')
            {
                nextChar();
                return {TokenType::LSHIFT, "<<", currentLine, startCol};
            } // NEW
            if (peekChar() == '=')
            {
                nextChar();
                return {TokenType::LTE, "<=", currentLine, startCol};
            }
            return {TokenType::LT, "<", currentLine, startCol};
        case '>':
            if (peekChar() == '>')
            {
                nextChar();
                return {TokenType::RSHIFT, ">>", currentLine, startCol};
            } // NEW
            if (peekChar() == '=')
            {
                nextChar();
                return {TokenType::GTE, ">=", currentLine, startCol};
            }
            return {TokenType::GT, ">", currentLine, startCol};

        case '=':
            if (peekChar() == '>')
            {
                nextChar();
                return {TokenType::ARROW, "=>", currentLine, startCol};
            }
            if (peekChar() == '=')
            {
                nextChar();
                return {TokenType::EQ, "==", currentLine, startCol};
            }
            return {TokenType::ASSIGN, "=", currentLine, startCol};

        case '!':
            if (peekChar() == '=')
            {
                nextChar();
                return {TokenType::NEQ, "!=", currentLine, startCol};
            }
            return {TokenType::NOT, "!", currentLine, startCol};

        // --- Bitwise AND / Logic AND ---
        case '&':
            if (peekChar() == '&')
            {
                nextChar();
                return {TokenType::AND, "&&", currentLine, startCol};
            }
            return {TokenType::AMPERSAND, "&", currentLine, startCol};

        // --- Bitwise OR / Logic OR ---
        case '|':
            if (peekChar() == '|')
            {
                nextChar();
                return {TokenType::OR, "||", currentLine, startCol};
            }
            return {TokenType::BITWISE_OR, "|", currentLine, startCol}; // FIXED

        // --- Bitwise XOR ---
        case '^':
            return {TokenType::BITWISE_XOR, "^", currentLine, startCol}; // NEW

        // --- Bitwise NOT ---
        case '~':
            return {TokenType::BITWISE_NOT, "~", currentLine, startCol}; // NEW

        case '?':
            if (peekChar() == '?')
            {
                nextChar();
                return {TokenType::QUESTION_QUESTION, "??", currentLine, startCol};
            }
            if (peekChar() == '.')
            {
                nextChar();
                return {TokenType::QUESTION_DOT, "?.", currentLine, startCol};
            }
            return {TokenType::QUESTION_MARK, "?", currentLine, startCol};

        default:
            return {TokenType::ILLEGAL, std::string(1, c), currentLine, startCol};
        }

        return {TokenType::ILLEGAL, "Unexpected character", currentLine, startCol};
    }

} // namespace Moksha