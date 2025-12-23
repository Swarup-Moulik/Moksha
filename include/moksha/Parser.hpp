#pragma once

#include "moksha/ast.hpp"
#include "moksha/token.hpp"
#include "moksha/Lexer.hpp" 
#include <vector>
#include <memory>
#include <stdexcept>
#include <map>
#include <iostream>
#include <deque>

namespace Moksha {

// --- Precedence Enum ---
enum class Precedence {
    NONE, 
    ASSIGNMENT,       // = += -=
    TERNARY,          // ? :
    NULL_COALESCING,  // ??
    LOGICAL_OR,       // ||  <-- Renamed from OR for clarity
    LOGICAL_AND,      // &&  <-- Renamed from AND
    BITWISE_OR,       // |   <-- NEW
    BITWISE_XOR,      // ^   <-- NEW
    BITWISE_AND,      // &   <-- NEW
    EQUALITY,         // == !=
    COMPARISON,       // < > <= >=
    SHIFT,            // << >> <-- NEW
    TERM,             // + -
    FACTOR,           // * / %
    EXPONENT,         // **
    UNARY,            // ! - ++ -- ~ * & await
    CALL,             // . () [] ?.
    PRIMARY
};

class Parser {
private:
    Lexer& lexer;
    std::deque<Token> tokenBuffer;
    Token currentToken;
    Token previousToken;
    bool hadError = false;

    // --- Expression Parsing ---
    // Controls precedence logic
    std::unique_ptr<ExpressionNode> parseExpression(Precedence precedence = Precedence::NONE);
    
    // Sub-expression parsers for finer control
    std::unique_ptr<ExpressionNode> parsePrimary(); // Literals, identifiers, parens
    std::unique_ptr<ExpressionNode> parseUnaryExpression(); // await, !, -, &, *

    // Handles int(x), string(y), double(z)
    std::unique_ptr<ExpressionNode> parseExplicitCast();
    std::unique_ptr<ExpressionNode> parseTypeCast();
    
    // Lambda / Arrow Function: () => {}
    std::unique_ptr<ExpressionNode> parseLambda(); 

    // --- Statement Parsers ---
    std::unique_ptr<StatementNode> parseStatement();
    std::unique_ptr<StatementNode> parseDeclaration(AccessLevel inheritedAccess);
    
    // Variable Declaration now handles Visibility and Shared Memory
    std::unique_ptr<StatementNode> parseVariableDeclaration(
        std::unique_ptr<TypeNode> type, 
        Token name, 
        bool isConst, 
        AccessLevel visibility = AccessLevel::DEFAULT, 
        bool isShared = false,
        bool isVolatile = false, 
        bool isExtern = false,   
        int alignment = 0,       
        std::string section = "" 
    ); 

    // Function Definition now handles Async, Operators, and Visibility
    std::unique_ptr<StatementNode> parseFunctionDefinition(
        std::unique_ptr<TypeNode> returnType, 
        Token name, 
        AccessLevel visibility = AccessLevel::DEFAULT,
        bool isAsync = false,
        bool isOperator = false,
        bool isExtern = false,   
        bool isInterrupt = false,
        std::string section = ""
    );

    // Class Definition now handles 'ref class'
    std::unique_ptr<StatementNode> parseClassDefinition(bool isRef = false);

    // Struct Definition (POD types)
    std::unique_ptr<StatementNode> parseStructDefinition(bool isPacked = false, bool isUnion = false);

    // Declare the ASM parser 
    std::unique_ptr<ExpressionNode> parseAsmExpression();

    // Handles enum Color { Red, Blue }
    std::unique_ptr<StatementNode> parseEnumDefinition();
    
    // Control Flow
    std::unique_ptr<StatementNode> parseIfStatement();
    std::unique_ptr<StatementNode> parseWhileStatement();
    std::unique_ptr<StatementNode> parseForStatement();
    std::unique_ptr<StatementNode> parseCStyleForStatement();
    std::unique_ptr<StatementNode> parseForInStatement();
    std::unique_ptr<StatementNode> parseDoWhileStatement(); 
    std::unique_ptr<StatementNode> parseSwitchStatement();
    std::unique_ptr<StatementNode> parseReturnStatement();
    std::unique_ptr<StatementNode> parseBreakStatement();
    std::unique_ptr<StatementNode> parseContinueStatement();
    
    // Modularity & I/O
    std::unique_ptr<StatementNode> parseImportStatement();
    std::unique_ptr<StatementNode> parseMacroDefinition(); 
    std::unique_ptr<StatementNode> parsePrintStatement();   

    // Systems & Safety
    std::unique_ptr<StatementNode> parseUnsafeBlock();
    std::unique_ptr<StatementNode> parseDeferStatement();
    std::unique_ptr<StatementNode> parseWithStatement();

    // Error Handling
    std::unique_ptr<StatementNode> parseTryStatement();
    std::unique_ptr<StatementNode> parseThrowStatement();

    std::unique_ptr<StatementNode> parseExpressionStatement();
    std::unique_ptr<BlockStatementNode> parseBlockStatement();

    // Helpers
    std::unique_ptr<ExpressionNode> parseTableLiteral();
    std::unique_ptr<ExpressionNode> parseDeleteExpression();
    std::unique_ptr<ExpressionNode> parseLengthExpression();
    std::unique_ptr<ExpressionNode> parseArrayLiteral();
    
    // Sub-parsers
    std::unique_ptr<TypeNode> parseType();
    std::vector<std::unique_ptr<ParameterNode>> parseParameterList(bool &isVariadic);
    std::unique_ptr<CallNode> parseCall(std::unique_ptr<ExpressionNode> callee);
    std::unique_ptr<StatementNode> parseStaticIfStatement();
    std::unique_ptr<StatementNode> parseTypedef();
    
    // Constructor (Special method inside class)
    std::unique_ptr<StatementNode> parseConstructorDefinition(AccessLevel access);
    std::unique_ptr<StatementNode> parseDestructorDefinition(AccessLevel access);
    
    std::unique_ptr<ExpressionNode> parseTemplateLiteral();

    // --- Helper Methods ---
    void advance();
    Token peek(int distance = 0);
    void consume(TokenType type, const std::string& message);
    bool check(TokenType type);
    bool checkPeek(TokenType type);
    bool match(const std::vector<TokenType>& types);
    bool isTypeToken();
    void logError(const std::string& message);
    void synchronize();
    Precedence getPrecedence(TokenType type);
    bool isPointerDeclaration();

    // Helper to detect public/private/protected
    AccessLevel parseAccessModifier(); 

public:
    // --- Public Interface ---
    Parser(Lexer& lexer);
    std::vector<std::unique_ptr<StatementNode>> parseProgram();
    bool hasError() const;
};

} // namespace Moksha