#include "moksha/Parser.hpp"
#include <iostream>

namespace Moksha
{
    // --- Constructor ---
    Parser::Parser(Lexer &lexer) : lexer(lexer), hadError(false)
    {
        advance();
    }

    Token Parser::peek(int distance)
    {
        // Fill buffer until we have enough tokens
        while (tokenBuffer.size() <= distance)
        {
            tokenBuffer.push_back(lexer.nextToken());
        }
        return tokenBuffer[distance];
    }

    // --- Public Entry Point ---
    std::vector<std::unique_ptr<StatementNode>> Parser::parseProgram()
    {
        std::vector<std::unique_ptr<StatementNode>> statements;
        while (!check(TokenType::EOFT))
        {
            Token startToken = currentToken;
            try
            {
                std::unique_ptr<StatementNode> stmt = nullptr;

                if (check(TokenType::KW_REF) && checkPeek(TokenType::KW_CLASS))
                {
                    advance();
                    advance();
                    stmt = parseClassDefinition(true);
                }
                else if (match({TokenType::KW_CLASS}))
                    stmt = parseClassDefinition(false);
                else if (check(TokenType::KW_EXTERN) || check(TokenType::KW_VOLATILE) ||
                         check(TokenType::KW_ALIGN) || check(TokenType::KW_SECTION))
                {
                    stmt = parseDeclaration(AccessLevel::DEFAULT);
                }
                else if (match({TokenType::KW_UNION}))
                {
                    stmt = parseStructDefinition(false, true); // isPacked=false, isUnion=true
                }
                else if (match({TokenType::KW_STRUCT}))
                {
                    stmt = parseStructDefinition(false, false);
                }
                else if (match({TokenType::KW_IMPORT}) || match({TokenType::KW_FROM}))
                    stmt = parseImportStatement();
                else if (match({TokenType::SEMICOLON}))
                    continue;
                else
                    stmt = parseStatement();

                if (stmt)
                    statements.push_back(std::move(stmt));

                // Infinite Loop Failsafe
                if (currentToken.line == startToken.line && currentToken.column == startToken.column)
                {
                    logError("Infinite loop detected at token '" + currentToken.lexeme + "'. Forcing advance.");
                    advance();
                }
            }
            catch (const std::runtime_error &e)
            {
                synchronize();
            }
        }
        return statements;
    }

    bool Parser::hasError() const { return hadError; }

    // --- Helper Methods ---
    void Parser::advance()
    {
        previousToken = currentToken;
        
        // Get next token from buffer or lexer
        if (tokenBuffer.empty())
        {
            currentToken = lexer.nextToken();
        }
        else
        {
            currentToken = tokenBuffer.front();
            tokenBuffer.pop_front();
        }

        // Handle Illegal tokens immediately if they appear
        while (currentToken.type == TokenType::ILLEGAL)
        {
            logError("Invalid character: " + currentToken.lexeme);
            if (tokenBuffer.empty())
                currentToken = lexer.nextToken();
            else {
                currentToken = tokenBuffer.front();
                tokenBuffer.pop_front();
            }
        }
    }

    AccessLevel Parser::parseAccessModifier()
    {
        if (match({TokenType::KW_PUBLIC}))
            return AccessLevel::PUBLIC;
        if (match({TokenType::KW_PRIVATE}))
            return AccessLevel::PRIVATE;
        if (match({TokenType::KW_PROTECTED}))
            return AccessLevel::PROTECTED;
        return AccessLevel::DEFAULT;
    }

    void Parser::consume(TokenType type, const std::string &message)
    {
        if (currentToken.type == type)
        {
            advance();
            return;
        }
        logError(message + ". Got " + currentToken.toString());
        throw std::runtime_error("Parser Error");
    }

    bool Parser::check(TokenType type) { return currentToken.type == type; }
    bool Parser::checkPeek(TokenType type) { return peek(0).type == type; }
    bool Parser::match(const std::vector<TokenType> &types)
    {
        for (TokenType type : types)
        {
            if (check(type))
            {
                advance();
                return true;
            }
        }
        return false;
    }

    bool Parser::isPointerDeclaration()
    {
        // We are at: [ID] [STAR] ...
        int offset = 0; // peek(0) is the first token after currentToken
        
        // Skip all stars: ID * * * var
        while (peek(offset).type == TokenType::STAR) {
            offset++;
        }
        
        Token nextToken = peek(offset);
        
        // 1. Basic Check: Must be followed by Identifier or Modifiers
        if (nextToken.type != TokenType::IDENTIFIER && 
            nextToken.type != TokenType::KW_CONST && 
            nextToken.type != TokenType::KW_VOLATILE)
        {
            return false;
        }
        
        Token tokenAfterVar = peek(offset + 1);
        
        if (tokenAfterVar.type == TokenType::RBRACE || // Ends block: { x * x }
            tokenAfterVar.type == TokenType::RPAREN || // Ends group: ( x * x )
            tokenAfterVar.type == TokenType::COMMA)    // In list: [ x * x, ... ]
        {
            return false; // Treat as Multiplication
        }

        // Default to Declaration for cases like "Type* x;" or "Type* x ="
        return true;
    }

    bool Parser::isTypeToken()
    {
        return check(TokenType::KW_INT) || check(TokenType::KW_DOUBLE) || check(TokenType::KW_STRING) || check(TokenType::KW_LONG) ||
               check(TokenType::KW_BOOLEAN) || check(TokenType::KW_EXTERN) || check(TokenType::KW_VOLATILE) ||
               check(TokenType::KW_ANY) || check(TokenType::KW_CHAR) || check(TokenType::KW_SHORT) ||
               check(TokenType::KW_FLOAT) || check(TokenType::KW_TABLE) || check(TokenType::KW_VOID);
    }

    void Parser::logError(const std::string &message)
    {
        hadError = true;
        std::cerr << "[Line " << currentToken.line << ", Col " << currentToken.column
                  << "] Syntax Error: " << message << std::endl;
    }

    void Parser::synchronize()
    {
        advance();
        while (currentToken.type != TokenType::EOFT)
        {
            if (previousToken.type == TokenType::SEMICOLON)
                return;
            switch (currentToken.type)
            {
            case TokenType::KW_CLASS:
            case TokenType::KW_IMPORT:
            case TokenType::KW_FROM:
            case TokenType::KW_IF:
            case TokenType::KW_WHILE:
            case TokenType::KW_FOR:
            case TokenType::KW_DO:
            case TokenType::KW_SWITCH:
            case TokenType::KW_RETURN:
            case TokenType::KW_PRINT:
            case TokenType::KW_PRINTLN:
            case TokenType::KW_MACRO:
            case TokenType::KW_CONST:
            case TokenType::LBRACE:
            case TokenType::KW_PUBLIC:
            case TokenType::KW_PRIVATE:
            case TokenType::KW_PROTECTED:
                return;
            default:
                break;
            }
            advance();
        }
    }

    // --- Expression Parsing ---

    Precedence Parser::getPrecedence(TokenType type)
    {
        switch (type)
        {
        case TokenType::ASSIGN:
        case TokenType::PLUS_ASSIGN:
        case TokenType::MINUS_ASSIGN:
        case TokenType::STAR_ASSIGN:
        case TokenType::SLASH_ASSIGN:
            return Precedence::ASSIGNMENT;
        case TokenType::QUESTION_MARK:
            return Precedence::TERNARY;
        case TokenType::QUESTION_QUESTION:
            return Precedence::NULL_COALESCING;
        case TokenType::OR:
            return Precedence::LOGICAL_OR;
        case TokenType::AND:
            return Precedence::LOGICAL_AND;
        case TokenType::BITWISE_OR:
            return Precedence::BITWISE_OR;
        case TokenType::BITWISE_XOR:
            return Precedence::BITWISE_XOR;
        case TokenType::AMPERSAND: // Acts as Bitwise AND in binary context
            return Precedence::BITWISE_AND;
        case TokenType::LSHIFT:
        case TokenType::RSHIFT:
            return Precedence::SHIFT;
        case TokenType::EQ:
        case TokenType::NEQ:
            return Precedence::EQUALITY;
        case TokenType::LT:
        case TokenType::GT:
        case TokenType::LTE:
        case TokenType::GTE:
        case TokenType::KW_IN:
            return Precedence::COMPARISON;
        case TokenType::PLUS:
        case TokenType::MINUS:
            return Precedence::TERM;
        case TokenType::STAR:
        case TokenType::SLASH:
        case TokenType::MODULO:
            return Precedence::FACTOR;
        case TokenType::STAR_STAR:
            return Precedence::EXPONENT;
        case TokenType::LPAREN:
        case TokenType::LBRACKET:
        case TokenType::DOT:
        case TokenType::QUESTION_DOT:
        case TokenType::PLUS_PLUS:
        case TokenType::MINUS_MINUS:
        case TokenType::NOT:
            return Precedence::CALL;
        default:
            return Precedence::NONE;
        }
    }

    std::unique_ptr<ExpressionNode> Parser::parseExplicitCast() // <-- ADDED
    {
        // Consumed 'cast' keyword in parsePrimary

        // 1. Parse the target type <Type>
        consume(TokenType::LT, "Expected '<' after 'cast'.");
        std::unique_ptr<TypeNode> targetType = parseType();
        consume(TokenType::GT, "Expected '>' after target type in 'cast'.");

        // 2. Parse the expression (expr)
        consume(TokenType::LPAREN, "Expected '(' after target type for explicit cast.");
        std::unique_ptr<ExpressionNode> expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after cast expression.");

        return std::make_unique<TypeCastNode>(std::move(targetType), std::move(expr));
    }

    std::unique_ptr<ExpressionNode> Parser::parseTypeCast()
    {
        // 1. Parse the target type (e.g., int, User*)
        std::unique_ptr<TypeNode> targetType = parseType();
        if (!targetType)
        {
            logError("Expected a valid type for the cast.");
            throw std::runtime_error("Parser Error");
        }

        // 2. Parse the expression to be cast
        consume(TokenType::LPAREN, "Expected '(' after target type for explicit cast.");
        std::unique_ptr<ExpressionNode> expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')' after cast expression.");

        return std::make_unique<TypeCastNode>(std::move(targetType), std::move(expr));
    }

    std::unique_ptr<ExpressionNode> Parser::parseExpression(Precedence precedence)
    {
        std::unique_ptr<ExpressionNode> leftExpression = parseUnaryExpression();
        if (!leftExpression)
            leftExpression = parsePrimary();
        if (!leftExpression)
            return nullptr;

        while (getPrecedence(currentToken.type) > Precedence::NONE &&
               precedence <= getPrecedence(currentToken.type))
        {
            Token opToken = currentToken;
            Precedence opPrecedence = getPrecedence(opToken.type);
            advance();

            switch (opToken.type)
            {
            case TokenType::ASSIGN:
            case TokenType::PLUS_ASSIGN:
            case TokenType::MINUS_ASSIGN:
            case TokenType::STAR_ASSIGN:
            case TokenType::SLASH_ASSIGN:
                leftExpression = std::make_unique<BinaryOpNode>(std::move(leftExpression), opToken, parseExpression(Precedence::ASSIGNMENT));
                break;
            case TokenType::QUESTION_MARK:
            {
                if (check(TokenType::LBRACKET))
                {
                    advance(); // Consume '['
                    auto indexExpr = parseExpression();
                    consume(TokenType::RBRACKET, "Expected ']'.");
                    // Create IndexNode with isOptional = true
                    leftExpression = std::make_unique<IndexNode>(std::move(leftExpression), std::move(indexExpr), true);
                    break;
                }
                auto thenBranch = parseExpression();
                consume(TokenType::COLON, "Expected ':' in ternary operator.");
                auto elseBranch = parseExpression(Precedence::TERNARY);
                leftExpression = std::make_unique<TernaryOpNode>(std::move(leftExpression), std::move(thenBranch), std::move(elseBranch));
                break;
            }
            case TokenType::QUESTION_QUESTION:
                leftExpression = std::make_unique<BinaryOpNode>(std::move(leftExpression), opToken, parseExpression(static_cast<Precedence>((int)opPrecedence + 1)));
                break;
            case TokenType::PLUS:
            case TokenType::MINUS:
            case TokenType::STAR:
            case TokenType::SLASH:
            case TokenType::MODULO:
            case TokenType::EQ:
            case TokenType::NEQ:
            case TokenType::LT:
            case TokenType::GT:
            case TokenType::LTE:
            case TokenType::GTE:
            case TokenType::AND:
            case TokenType::OR:
            case TokenType::KW_IN:
            case TokenType::BITWISE_OR:
            case TokenType::BITWISE_XOR:
            case TokenType::AMPERSAND: // Bitwise AND
            case TokenType::LSHIFT:
            case TokenType::RSHIFT:
                leftExpression = std::make_unique<BinaryOpNode>(std::move(leftExpression), opToken, parseExpression(static_cast<Precedence>((int)opPrecedence + 1)));
                break;
            case TokenType::STAR_STAR:
                leftExpression = std::make_unique<BinaryOpNode>(std::move(leftExpression), opToken, parseExpression(opPrecedence));
                break;
            case TokenType::PLUS_PLUS:
            case TokenType::MINUS_MINUS:
                leftExpression = std::make_unique<UpdateNode>(opToken, std::move(leftExpression), false);
                break;
            case TokenType::NOT:
                leftExpression = std::make_unique<UpdateNode>(opToken, std::move(leftExpression), false);
                break;
            case TokenType::DOT:
            {
                Token memberName = currentToken;
                if (check(TokenType::IDENTIFIER) || check(TokenType::KW_LENGTH))
                { // Allow length
                    advance();
                    leftExpression = std::make_unique<MemberAccessNode>(std::move(leftExpression), memberName);
                }
                else
                {
                    logError("Expected member name.");
                }
                break;
            }
            case TokenType::QUESTION_DOT:
            {
                if (check(TokenType::LBRACKET))
                {
                    advance();
                    auto idx = parseExpression();
                    consume(TokenType::RBRACKET, "Expected ']'.");
                    leftExpression = std::make_unique<IndexNode>(std::move(leftExpression), std::move(idx), true);
                }
                else if (check(TokenType::LPAREN))
                {
                    leftExpression = parseCall(std::move(leftExpression));
                    if (auto call = dynamic_cast<CallNode *>(leftExpression.get()))
                    {
                        call->isOptional = true;
                    }
                    break;
                }
                else
                {
                    Token memberName = currentToken;
                    if (check(TokenType::IDENTIFIER) || check(TokenType::KW_LENGTH))
                    { // Allow length
                        advance();
                        leftExpression = std::make_unique<MemberAccessNode>(std::move(leftExpression), memberName, true);
                    }
                    else
                    {
                        logError("Expected property name.");
                    }
                }
                break;
            }
            case TokenType::LBRACKET:
            {
                auto indexExpr = parseExpression();
                consume(TokenType::RBRACKET, "Expected ']'.");
                leftExpression = std::make_unique<IndexNode>(std::move(leftExpression), std::move(indexExpr));
                break;
            }
            case TokenType::LPAREN:
                leftExpression = parseCall(std::move(leftExpression));
                break;
            default:
                return leftExpression;
            }
        }
        return leftExpression;
    }

    std::unique_ptr<ExpressionNode> Parser::parseUnaryExpression()
    {
        if (match({TokenType::KW_AWAIT}))
            return std::make_unique<UnaryOpNode>(previousToken, parseExpression(Precedence::UNARY));
        if (match({TokenType::MINUS, TokenType::AMPERSAND, TokenType::STAR, TokenType::BITWISE_NOT}))
        {
            Token operatorToken = previousToken;
            std::unique_ptr<ExpressionNode> right = parseExpression(Precedence::UNARY);
            return std::make_unique<UnaryOpNode>(operatorToken, std::move(right));
        }
        if (match({TokenType::NOT}) || match({TokenType::PLUS_PLUS}) || match({TokenType::MINUS_MINUS}))
        {
            Token op = previousToken;
            if (op.type == TokenType::PLUS_PLUS || op.type == TokenType::MINUS_MINUS)
                return std::make_unique<UpdateNode>(op, parseExpression(Precedence::UNARY), true);
            return std::make_unique<UnaryOpNode>(op, parseExpression(Precedence::UNARY));
        }
        if (match({TokenType::KW_SIZEOF}))
        {
            consume(TokenType::LPAREN, "Expected '(' after sizeof.");

            // Check if it's a Type or Expression
            if (isTypeToken() || (check(TokenType::IDENTIFIER) && (checkPeek(TokenType::STAR) || checkPeek(TokenType::RPAREN))))
            {
                auto type = parseType();
                consume(TokenType::RPAREN, "Expected ')'.");
                return std::make_unique<SizeOfNode>(std::move(type));
            }

            auto expr = parseExpression();
            consume(TokenType::RPAREN, "Expected ')'.");
            return std::make_unique<SizeOfNode>(std::move(expr));
        }
        return nullptr;
    }

    std::unique_ptr<ExpressionNode> Parser::parsePrimary()
    {
        if (match({TokenType::KW_CAST}))
        {
            return parseExplicitCast();
        }
        if (isTypeToken() && checkPeek(TokenType::LPAREN))
        {
            return parseTypeCast();
        }
        if (match({TokenType::KW_ASM}))
        {
            return parseAsmExpression();
        }
        switch (currentToken.type)
        {
        case TokenType::INTEGER_LITERAL:
        case TokenType::FLOAT_LITERAL:
        case TokenType::BOOLEAN_LITERAL:
        case TokenType::CHAR_LITERAL:
        case TokenType::STRING_LITERAL:
        case TokenType::STRING_CHUNK:
        case TokenType::KW_NULL:
            advance();
            return std::make_unique<LiteralNode>(previousToken);
        case TokenType::IDENTIFIER:
            advance();
            return std::make_unique<VariableNode>(previousToken);
        case TokenType::KW_THIS:
            advance();
            return std::make_unique<ThisExpression>(previousToken);
        case TokenType::KW_READ:
        case TokenType::KW_READINT:
        case TokenType::KW_READDOUBLE:
        case TokenType::KW_READBOOL:
        {
            Token keyword = currentToken;
            advance();
            consume(TokenType::LPAREN, "Expected '('.");
            consume(TokenType::RPAREN, "Expected ')'.");
            return std::make_unique<InputExpression>(keyword);
        }
        case TokenType::KW_NEW:
        {
            advance();
            Token className = currentToken;
            consume(TokenType::IDENTIFIER, "Class name.");
            consume(TokenType::LPAREN, "Expected '('.");
            std::vector<std::unique_ptr<ExpressionNode>> args;
            if (!check(TokenType::RPAREN))
                do
                {
                    args.push_back(parseExpression());
                } while (match({TokenType::COMMA}));
            consume(TokenType::RPAREN, "Expected ')'.");
            return std::make_unique<NewExpression>(className, std::move(args));
        }
        case TokenType::LPAREN:
        {
            // A. Check for "Empty" Lambda: () => ...
            if (checkPeek(TokenType::RPAREN))
            {
                advance(); // consume (
                consume(TokenType::RPAREN, "Expected ')'.");
                consume(TokenType::ARROW, "Expected '=>'.");

                std::unique_ptr<StatementNode> body;
                if (check(TokenType::LBRACE))
                {
                    consume(TokenType::LBRACE, "Expected '{'.");
                    body = parseBlockStatement();
                }
                else
                {
                    auto expr = parseExpression();
                    body = std::make_unique<ReturnNode>(Token{TokenType::KW_RETURN, "return", 0, 0}, std::move(expr));
                }
                return std::make_unique<LambdaNode>(std::vector<std::unique_ptr<ParameterNode>>(), std::move(body));
            }

            advance(); // Consume LPAREN (Start Parsing Inside Parens)

            // B. Check for TYPED Lambda
            if ((isTypeToken() || check(TokenType::IDENTIFIER)) && checkPeek(TokenType::IDENTIFIER))
            {
                // IT IS A TYPED LAMBDA! (int x) or (User u)
                std::vector<std::unique_ptr<ParameterNode>> params;

                // Parse the parameters loop manually since we already ate '('
                while (!check(TokenType::RPAREN) && !check(TokenType::EOFT))
                {
                    // 1. Parse Type
                    std::unique_ptr<TypeNode> paramType = parseType(); // e.g. int

                    // 2. Parse Name
                    Token paramName = currentToken;
                    consume(TokenType::IDENTIFIER, "Expected parameter name.");

                    params.push_back(std::make_unique<ParameterNode>(std::move(paramType), std::move(paramName)));

                    if (check(TokenType::COMMA))
                    {
                        advance();
                    }
                    else if (!check(TokenType::RPAREN))
                    {
                        logError("Expected ',' or ')' after parameter.");
                    }
                }
                consume(TokenType::RPAREN, "Expected ')'.");
                consume(TokenType::ARROW, "Expected '=>'.");

                std::unique_ptr<StatementNode> body;
                if (check(TokenType::LBRACE))
                {
                    consume(TokenType::LBRACE, "Expected '{'.");
                    body = parseBlockStatement();
                }
                else
                {
                    auto expr = parseExpression();
                    body = std::make_unique<ReturnNode>(Token{TokenType::KW_RETURN, "return", 0, 0}, std::move(expr));
                }
                return std::make_unique<LambdaNode>(std::move(params), std::move(body));
            }

            // C. Untyped Lambda (a, b) => ... OR Expression Grouping (a + b)
            std::vector<std::unique_ptr<ExpressionNode>> exprList;
            if (!check(TokenType::RPAREN))
            {
                do
                {
                    exprList.push_back(parseExpression());
                } while (match({TokenType::COMMA}));
            }
            consume(TokenType::RPAREN, "Expected ')'.");

            if (match({TokenType::ARROW}))
            {
                // It WAS a lambda! Convert expressions to parameters.
                std::vector<std::unique_ptr<ParameterNode>> params;
                for (auto &expr : exprList)
                {
                    if (auto v = dynamic_cast<VariableNode *>(expr.get()))
                    {
                        // Untyped parameter (Type = null)
                        params.push_back(std::make_unique<ParameterNode>(nullptr, v->name));
                    }
                    else
                    {
                        logError("Invalid parameter in lambda syntax. Expected identifiers.");
                    }
                }

                std::unique_ptr<StatementNode> body;
                if (check(TokenType::LBRACE))
                {
                    consume(TokenType::LBRACE, "Expected '{'.");
                    body = parseBlockStatement();
                }
                else
                {
                    auto exprBody = parseExpression();
                    if (exprBody)
                    {
                        body = std::make_unique<ReturnNode>(Token{TokenType::KW_RETURN, "return", 0, 0}, std::move(exprBody));
                    }
                    else
                    {
                        logError("Expected expression or block body for lambda.");
                        return nullptr;
                    }
                }
                return std::make_unique<LambdaNode>(std::move(params), std::move(body));
            }

            // It was just a grouping
            if (exprList.empty())
            {
                logError("Unexpected empty grouping '()'.");
                return nullptr;
            }
            if (exprList.size() > 1)
            {
                logError("Unexpected comma in parenthesized expression. (Tuples not supported yet).");
                return nullptr;
            }
            return std::make_unique<GroupingNode>(std::move(exprList[0]));
        }
        case TokenType::BACKTICK:
            return parseTemplateLiteral();
        case TokenType::LBRACE:
            return parseTableLiteral();
        case TokenType::LBRACKET:
            return parseArrayLiteral();

        case TokenType::KW_DELETE:
            advance();
            consume(TokenType::LPAREN, "Expected '('.");
            return parseDeleteExpression();
        case TokenType::KW_LENGTH:
            advance();
            consume(TokenType::LPAREN, "Expected '('.");
            return parseLengthExpression();
        default:
            logError("Expected expression, got " + currentToken.toString());
            throw std::runtime_error("Parser Error");
        }
    }

    std::unique_ptr<ExpressionNode> Parser::parseLambda()
    {
        std::vector<std::unique_ptr<ParameterNode>> params;
        std::unique_ptr<StatementNode> body;
        if (check(TokenType::LBRACE))
        {
            consume(TokenType::LBRACE, "Expected '{'.");
            body = parseBlockStatement();
        }
        else
        {
            auto expr = parseExpression();
            body = std::make_unique<ReturnNode>(Token{TokenType::KW_RETURN, "return", 0, 0}, std::move(expr));
        }
        return std::make_unique<LambdaNode>(std::move(params), std::move(body));
    }

    std::unique_ptr<CallNode> Parser::parseCall(std::unique_ptr<ExpressionNode> callee)
    {
        std::vector<std::unique_ptr<ExpressionNode>> arguments;
        if (!check(TokenType::RPAREN))
        {
            do
            {
                arguments.push_back(parseExpression());
            } while (match({TokenType::COMMA}));
        }
        consume(TokenType::RPAREN, "Expected ')'.");
        return std::make_unique<CallNode>(std::move(callee), std::move(arguments));
    }

    // --- Statement Parsers ---

    std::unique_ptr<StatementNode> Parser::parseStatement()
    {
        if (match({TokenType::KW_UNSAFE}))
            return parseUnsafeBlock();
        if (match({TokenType::KW_DEFER}))
            return parseDeferStatement();
        if (match({TokenType::KW_WITH}))
            return parseWithStatement();
        if (match({TokenType::KW_TRY}))
            return parseTryStatement();
        if (match({TokenType::KW_THROW}))
            return parseThrowStatement();
        if (match({TokenType::KW_PRINT}) || match({TokenType::KW_PRINTLN}))
            return parsePrintStatement();
        if (match({TokenType::KW_MACRO}))
            return parseMacroDefinition();
        if (match({TokenType::KW_IF}))
            return parseIfStatement();
        if (match({TokenType::KW_STATIC}))
        {
            if (match({TokenType::KW_IF}))
                return parseStaticIfStatement();
            // Handle static variables if you want
        }
        if (match({TokenType::KW_TYPEDEF}))
            return parseTypedef();
        if (check(TokenType::KW_CONST) || isTypeToken() ||
            (check(TokenType::IDENTIFIER) && (checkPeek(TokenType::IDENTIFIER))))
        {
            return parseDeclaration(AccessLevel::DEFAULT);
        }
        if (match({TokenType::KW_WHILE}))
            return parseWhileStatement();
        if (match({TokenType::KW_FOR}))
            return parseForStatement();
        if (match({TokenType::KW_DO}))
            return parseDoWhileStatement();
        if (match({TokenType::KW_SWITCH}))
            return parseSwitchStatement();
        if (match({TokenType::KW_RETURN}))
            return parseReturnStatement();
        if (match({TokenType::KW_BREAK}))
            return parseBreakStatement();
        if (match({TokenType::KW_CONTINUE}))
            return parseContinueStatement();
        if (check(TokenType::KW_ENUM))
            return parseEnumDefinition();
        if (match({TokenType::KW_PACKED}))
        {
            consume(TokenType::KW_STRUCT, "Expected 'struct' after 'packed'.");
            return parseStructDefinition(true); // isPacked = true
        }
        if (match({TokenType::KW_STRUCT}))
            return parseStructDefinition(false, false);
        if (check(TokenType::KW_PUBLIC) || check(TokenType::KW_PRIVATE) || check(TokenType::KW_PROTECTED) || check(TokenType::KW_SECTION) ||
            check(TokenType::KW_REF) || check(TokenType::KW_SHARED) || check(TokenType::KW_EXTERN) || check(TokenType::KW_VOLATILE) ||
            check(TokenType::KW_ASYNC) || check(TokenType::KW_CONST) || isTypeToken() || check(TokenType::KW_ALIGN) || check(TokenType::KW_INTERRUPT))
        {
            return parseDeclaration(AccessLevel::DEFAULT);
        }
        if (match({TokenType::KW_UNION}))
        {
            return parseStructDefinition(false, true); // handles local unions
        }
        if (check(TokenType::IDENTIFIER))
        {
            if (checkPeek(TokenType::IDENTIFIER))
                return parseDeclaration(AccessLevel::DEFAULT);
            if (checkPeek(TokenType::KW_OPERATOR))
                return parseDeclaration(AccessLevel::DEFAULT); // Operator overload
            if (checkPeek(TokenType::QUESTION_MARK))
                return parseDeclaration(AccessLevel::DEFAULT); // Nullable type
            if (checkPeek(TokenType::STAR))
            {
                if (isPointerDeclaration()) {
                    return parseDeclaration(AccessLevel::DEFAULT);
                }
            }
        }

        if (match({TokenType::LBRACE}))
            return parseBlockStatement();
        return parseExpressionStatement();
    }

    std::unique_ptr<StatementNode> Parser::parseEnumDefinition()
    {
        consume(TokenType::KW_ENUM, "Expected 'enum'.");
        Token name = currentToken;
        consume(TokenType::IDENTIFIER, "Expected enum name.");
        consume(TokenType::LBRACE, "Expected '{'.");

        std::vector<std::pair<Token, std::unique_ptr<ExpressionNode>>> members;

        if (!check(TokenType::RBRACE))
        {
            do
            {
                Token memberName = currentToken;
                consume(TokenType::IDENTIFIER, "Expected enum member name.");

                std::unique_ptr<ExpressionNode> value = nullptr;
                if (match({TokenType::ASSIGN}))
                {
                    value = parseExpression();
                }

                members.push_back({memberName, std::move(value)});

            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RBRACE, "Expected '}'.");
        return std::make_unique<EnumDefinitionNode>(std::move(name), std::move(members));
    }

    std::unique_ptr<StatementNode> Parser::parseDeclaration(AccessLevel inheritedAccess)
    {
        AccessLevel visibility = (inheritedAccess == AccessLevel::DEFAULT) ? parseAccessModifier() : inheritedAccess;
        bool isExtern = match({TokenType::KW_EXTERN});
        bool isVolatile = match({TokenType::KW_VOLATILE});
        bool isShared = match({TokenType::KW_SHARED});
        bool isRef = match({TokenType::KW_REF});
        if (isRef)
            isShared = true;
        bool isConst = match({TokenType::KW_CONST});
        bool isAsync = match({TokenType::KW_ASYNC});
        bool isInterrupt = match({TokenType::KW_INTERRUPT});

        int alignment = 0;
        std::string section = "";
        while (check(TokenType::KW_ALIGN) || check(TokenType::KW_SECTION))
        {
            if (match({TokenType::KW_ALIGN}))
            {
                consume(TokenType::LPAREN, "Expected '(' after 'align'.");
                Token lit = currentToken;
                consume(TokenType::INTEGER_LITERAL, "Alignment must be a constant integer.");
                alignment = std::stoi(lit.lexeme);
                consume(TokenType::RPAREN, "Expected ')'.");
            }
            if (match({TokenType::KW_SECTION}))
            {
                consume(TokenType::LPAREN, "Expected '(' after 'section'.");
                Token lit = currentToken;
                consume(TokenType::STRING_LITERAL, "Section name must be a string literal.");
                section = lit.lexeme;
                consume(TokenType::RPAREN, "Expected ')'.");
            }
        }

        if (match({TokenType::KW_PACKED}))
        {
            consume(TokenType::KW_STRUCT, "Expected 'struct' after 'packed'.");
            return parseStructDefinition(true, false); // Pass isPacked=true
        }

        if (check(TokenType::KW_STRUCT))
        {
            advance();
            return parseStructDefinition(false, false);
        }

        if (match({TokenType::KW_UNION}))
        {
            advance();
            return parseStructDefinition(false, true); // isPacked=false, isUnion=true
        }

        if (match({TokenType::KW_OPERATOR}))
        {
            Token op = currentToken; // The operator token (e.g., '+')
            advance();
            auto returnType = std::make_unique<TypeNode>(Token{TokenType::KW_ANY, "any", 0, 0});
            return parseFunctionDefinition(std::move(returnType), op, visibility, isAsync, true);
        }

        std::unique_ptr<TypeNode> type = parseType();
        if (!type)
        {
            if (match({TokenType::KW_OPERATOR}))
            {
                Token op = currentToken;
                advance();
                auto returnType = std::make_unique<TypeNode>(Token{TokenType::KW_ANY, "any", 0, 0});
                return parseFunctionDefinition(std::move(returnType), op, visibility, isAsync, true);
            }
            advance();
            return nullptr;
        }

        if (match({TokenType::KW_OPERATOR}))
        {
            Token op = currentToken;
            advance();
            return parseFunctionDefinition(std::move(type), op, visibility, isAsync, true);
        }

        Token name = currentToken;
        consume(TokenType::IDENTIFIER, "Expected identifier.");

        if (check(TokenType::LPAREN))
        {            
            return parseFunctionDefinition(std::move(type), name, visibility, isAsync, false, isExtern, isInterrupt, section);
        }

        if (isAsync)
            logError("Variables cannot be 'async'.");

        // Safe Array Declaration Logic
        while (check(TokenType::LBRACKET))
        {
            advance(); // Consume [
            std::vector<std::unique_ptr<ExpressionNode>> dims;
            if (check(TokenType::RBRACKET))
            {
                dims.push_back(nullptr); // Empty size []
            }
            else
            {
                dims.push_back(parseExpression());
            }
            consume(TokenType::RBRACKET, "Expected ']'.");

            if (type)
            {
                Token base = type->baseTypeToken;
                auto safeType = std::move(type);
                type = std::make_unique<TypeNode>(base, std::move(safeType), std::move(dims));
            }
            else
            {
                logError("Critical: Type became null.");
                return nullptr;
            }
        }

        return parseVariableDeclaration(std::move(type), name, isConst, visibility, isShared, isVolatile, isExtern, alignment, section);
    }

    std::unique_ptr<StatementNode> Parser::parseFunctionDefinition(std::unique_ptr<TypeNode> returnType, Token name, AccessLevel visibility,
                                                                    bool isAsync, bool isOperator, bool isExtern, bool isInterrupt, std::string section)
    {
        bool isVariadic = false;
        auto params = parseParameterList(isVariadic);

        std::unique_ptr<BlockStatementNode> body = nullptr;

        // Extern functions or prototypes end in a semicolon
        if (isExtern || match({TokenType::SEMICOLON}))
        {
            if (!isExtern)
                match({TokenType::SEMICOLON}); // consume semicolon if it wasn't extern
            body = nullptr;                    // Prototype has no body
        }
        else
        {
            consume(TokenType::LBRACE, "Expected '{' for function body.");
            body = parseBlockStatement();
        }

        return std::make_unique<FunctionDefinitionNode>(std::move(returnType), std::move(name), std::move(params), std::move(body),
                                                        visibility, isAsync, isOperator, isExtern, isInterrupt, isVariadic, section);
    }

    std::unique_ptr<StatementNode> Parser::parseVariableDeclaration(std::unique_ptr<TypeNode> type, Token name, bool isConst,
                                                                    AccessLevel visibility, bool isShared, bool isVolatile, bool isExtern, int alignment, std::string section)
    {
        std::unique_ptr<ExpressionNode> initializer = nullptr;
        if (match({TokenType::ASSIGN}))
        {
            initializer = parseExpression();
        }

        // Extern variables usually don't have initializers in the declaration
        if (isExtern && initializer)
        {
            logError("Extern variables cannot have initializers.");
        }

        match({TokenType::SEMICOLON});
        return std::make_unique<VariableDeclarationNode>(std::move(type), std::move(name), std::move(initializer), isConst, visibility,
                                                         isShared, isVolatile, isExtern, alignment, section);
    }

    std::unique_ptr<TypeNode> Parser::parseType()
    {
        if (!isTypeToken() && !check(TokenType::IDENTIFIER))
            return nullptr;
        Token rootToken = currentToken;
        advance();

        int depth = 0;
        while (match({TokenType::STAR}))
        {
            depth++;
        }

        auto type = std::make_unique<TypeNode>(rootToken, depth, false, false); // Base type

        while (check(TokenType::LBRACKET))
        {
            advance();
            std::vector<std::unique_ptr<ExpressionNode>> dims;
            if (check(TokenType::RBRACKET))
            {
                dims.push_back(nullptr); // Unsized array
            }
            else
            {
                dims.push_back(parseExpression()); // Sized array [10]
            }
            consume(TokenType::RBRACKET, "Expected ']'");

            auto innerType = std::move(type);
            type = std::make_unique<TypeNode>(rootToken, std::move(innerType), std::move(dims));
        }

        if (match({TokenType::QUESTION_MARK}))
        {
            type->isNullable = true;
        }

        return type;
    }

    std::unique_ptr<ExpressionNode> Parser::parseArrayLiteral()
    {
        std::vector<std::unique_ptr<ExpressionNode>> elements;
        advance();

        if (!check(TokenType::RBRACKET))
        {
            do
            {
                Token start = currentToken;
                if (match({TokenType::ELLIPSIS}))
                {
                    auto expr = parseExpression();
                    elements.push_back(std::make_unique<SpreadElementNode>(std::move(expr)));
                }
                else
                {
                    auto expr = parseExpression();
                    if (!expr)
                        break;
                    elements.push_back(std::move(expr));
                }
                if (currentToken.line == start.line && currentToken.column == start.column)
                {
                    logError("Stuck in array literal. Aborting.");
                    break;
                }
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RBRACKET, "Expected ']'.");
        return std::make_unique<ArrayLiteralNode>(std::move(elements));
    }

    // --- Switch Statement with Range Support ---
    std::unique_ptr<StatementNode> Parser::parseSwitchStatement()
    {
        consume(TokenType::LPAREN, "Expected '('.");
        auto condition = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'.");
        consume(TokenType::LBRACE, "Expected '{'.");

        std::vector<std::unique_ptr<CaseNode>> cases;
        std::unique_ptr<CaseNode> defaultCase = nullptr;

        while (!check(TokenType::RBRACE) && !check(TokenType::EOFT))
        {
            if (match({TokenType::KW_CASE}))
            {
                auto val1 = parseExpression();
                consume(TokenType::COLON, "Expected ':' after case value.");

                // Check for Range Syntax (value : value)
                bool isRange = false;
                if (check(TokenType::INTEGER_LITERAL) || check(TokenType::FLOAT_LITERAL) ||
                    check(TokenType::CHAR_LITERAL) || check(TokenType::STRING_LITERAL))
                {
                    if (checkPeek(TokenType::COLON))
                    {
                        isRange = true;
                    }
                }

                if (isRange)
                {
                    auto val2 = parseExpression();
                    consume(TokenType::COLON, "Expected ':' after range end.");

                    std::vector<std::unique_ptr<StatementNode>> stmts;
                    while (!check(TokenType::KW_CASE) && !check(TokenType::KW_DEFAULT) && !check(TokenType::RBRACE) && !check(TokenType::EOFT))
                    {
                        stmts.push_back(parseStatement());
                    }
                    cases.push_back(std::make_unique<CaseNode>(std::move(val1), std::move(val2), std::move(stmts)));
                }
                else
                {
                    // Standard Case
                    std::vector<std::unique_ptr<StatementNode>> stmts;
                    while (!check(TokenType::KW_CASE) && !check(TokenType::KW_DEFAULT) && !check(TokenType::RBRACE) && !check(TokenType::EOFT))
                    {
                        stmts.push_back(parseStatement());
                    }
                    cases.push_back(std::make_unique<CaseNode>(std::move(val1), std::move(stmts)));
                }
            }
            else if (match({TokenType::KW_DEFAULT}))
            {
                consume(TokenType::COLON, "Expected ':' after 'default'.");
                std::vector<std::unique_ptr<StatementNode>> statements;
                while (!check(TokenType::KW_CASE) && !check(TokenType::KW_DEFAULT) && !check(TokenType::RBRACE) && !check(TokenType::EOFT))
                {
                    statements.push_back(parseStatement());
                }
                defaultCase = std::make_unique<CaseNode>(nullptr, std::move(statements));
            }
            else
            {
                logError("Expected 'case' or 'default'.");
                advance();
            }
        }
        consume(TokenType::RBRACE, "Expected '}'.");
        return std::make_unique<SwitchStatementNode>(std::move(condition), std::move(cases), std::move(defaultCase));
    }

    std::unique_ptr<StatementNode> Parser::parseReturnStatement()
    {
        Token keyword = previousToken;
        std::unique_ptr<ExpressionNode> value = nullptr;
        if (!check(TokenType::SEMICOLON) && !check(TokenType::RBRACE))
            value = parseExpression();
        match({TokenType::SEMICOLON});
        return std::make_unique<ReturnNode>(keyword, std::move(value));
    }

    std::unique_ptr<StatementNode> Parser::parseImportStatement()
    {
        bool isFromFirst = previousToken.type == TokenType::KW_FROM;
        Token moduleName = {TokenType::ILLEGAL, "", 0, 0};
        std::vector<Token> symbols;

        if (isFromFirst)
        {
            consume(TokenType::STRING_LITERAL, "Expected module path.");
            moduleName = previousToken;
            consume(TokenType::KW_IMPORT, "Expected 'import'.");
            consume(TokenType::LBRACE, "Expected '{'.");
            if (!check(TokenType::RBRACE))
                do
                {
                    symbols.push_back(currentToken);
                    consume(TokenType::IDENTIFIER, "Expected symbol.");
                } while (match({TokenType::COMMA}));
            consume(TokenType::RBRACE, "Expected '}'.");
        }
        else
        {
            // Simple import: import "math"
            // Or named import: import { x } from "mod"
            if (check(TokenType::STRING_LITERAL))
            {
                advance();
                moduleName = previousToken;
                // No symbols, just import the whole module
            }
            else if (match({TokenType::LBRACE}))
            {
                // import { ... } from ...
                if (!check(TokenType::RBRACE))
                    do
                    {
                        symbols.push_back(currentToken);
                        consume(TokenType::IDENTIFIER, "Expected symbol.");
                    } while (match({TokenType::COMMA}));
                consume(TokenType::RBRACE, "Expected '}'.");
                consume(TokenType::KW_FROM, "Expected 'from'.");
                consume(TokenType::STRING_LITERAL, "Expected module path.");
                moduleName = previousToken;
            }
        }
        match({TokenType::SEMICOLON});
        return std::make_unique<ImportNode>(moduleName, std::move(symbols), isFromFirst);
    }

    std::unique_ptr<BlockStatementNode> Parser::parseBlockStatement()
    {
        auto block = std::make_unique<BlockStatementNode>();
        while (!check(TokenType::RBRACE) && !check(TokenType::EOFT))
        {
            Token startToken = currentToken;
            block->statements.push_back(parseStatement());
            if (currentToken.line == startToken.line && currentToken.column == startToken.column)
            {
                logError("Infinite loop in block. Forcing advance.");
                advance();
            }
        }
        consume(TokenType::RBRACE, "Expected '}'.");
        return block;
    }

    std::unique_ptr<StatementNode> Parser::parseExpressionStatement()
    {
        auto expr = parseExpression();
        if (!expr)
            return nullptr;
        match({TokenType::SEMICOLON});
        return std::make_unique<ExpressionStatementNode>(std::move(expr));
    }

    std::unique_ptr<ExpressionNode> Parser::parseTableLiteral()
    {
        std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<ExpressionNode>>> entries;
        advance(); // Consume {
        if (!check(TokenType::RBRACE))
            do
            {
                auto key = parseExpression();
                consume(TokenType::COLON, "Expected ':'.");
                auto val = parseExpression();
                entries.push_back({std::move(key), std::move(val)});
            } while (match({TokenType::COMMA}));
        consume(TokenType::RBRACE, "Expected '}'.");
        return std::make_unique<TableLiteralNode>(std::move(entries));
    }

    std::unique_ptr<ExpressionNode> Parser::parseTemplateLiteral()
    {
        auto node = std::make_unique<TemplateLiteralNode>();
        advance();
        while (!check(TokenType::BACKTICK) && !check(TokenType::EOFT))
        {
            if (check(TokenType::STRING_CHUNK))
            {
                node->parts.push_back(std::make_unique<LiteralNode>(currentToken));
                advance();
            }
            else if (match({TokenType::INTERPOLATION_START}))
            {
                node->parts.push_back(parseExpression());
                consume(TokenType::RBRACE, "Expected '}'.");
            }
            else
            {
                advance();
            }
        }
        consume(TokenType::BACKTICK, "Expected '`'.");
        return node;
    }

    std::vector<std::unique_ptr<ParameterNode>> Parser::parseParameterList(bool &isVariadic)
    {
        std::vector<std::unique_ptr<ParameterNode>> parameters;
        consume(TokenType::LPAREN, "Expected '('.");

        while (!check(TokenType::RPAREN) && !check(TokenType::EOFT))
        {

            bool isRef = match({TokenType::KW_REF});
            std::unique_ptr<TypeNode> paramType = nullptr;
            Token paramName = {TokenType::ILLEGAL, "", 0, 0};

            // [NEW] Handle Variadic
            if (match({TokenType::ELLIPSIS}))
            {
                isVariadic = true;
                // '...' must be the last parameter
                break;
            }

            // Logic to handle Type Name vs Name (untyped)
            if (isTypeToken())
            {
                // Definitely Typed: int a
                paramType = parseType();
                paramName = currentToken;
                consume(TokenType::IDENTIFIER, "Expected parameter name after type.");
            }
            else if (check(TokenType::IDENTIFIER))
            {
                // Ambiguous: 'User u' (Typed) OR 'u' (Untyped)
                if (checkPeek(TokenType::IDENTIFIER))
                {
                    // Lookahead says: ID ID -> Type Name
                    Token typeToken = currentToken;
                    advance(); // Consume type name
                    paramType = std::make_unique<TypeNode>(typeToken);
                    paramName = currentToken;
                    consume(TokenType::IDENTIFIER, "Expected parameter name.");
                }
                else
                {
                    // Lookahead says: ID , or ID ) -> Untyped Name
                    paramName = currentToken;
                    advance(); // Consume name
                }
            }
            else
            {
                logError("Expected parameter declaration.");
                // Panic recovery
                while (!check(TokenType::COMMA) && !check(TokenType::RPAREN) && !check(TokenType::EOFT))
                    advance();
            }

            std::unique_ptr<ExpressionNode> defaultValue = nullptr;
            if (match({TokenType::ASSIGN}))
            {
                defaultValue = parseExpression();
            }

            if (paramName.type != TokenType::ILLEGAL)
            {
                parameters.push_back(std::make_unique<ParameterNode>(std::move(paramType), std::move(paramName), std::move(defaultValue), isRef));
            }

            if (check(TokenType::COMMA))
            {
                advance(); // Consume ','
            }
            else if (!check(TokenType::RPAREN))
            {
                logError("Expected ',' or ')' after parameter.");
            }
        }

        consume(TokenType::RPAREN, "Expected ')'.");
        return parameters;
    }

    std::unique_ptr<StatementNode> Parser::parseConstructorDefinition(AccessLevel access)
    {
        Token name = previousToken;
        bool isVariadic = false;
        auto params = parseParameterList(isVariadic);
        consume(TokenType::LBRACE, "Expected '{'.");
        auto body = parseBlockStatement();
        return std::make_unique<FunctionDefinitionNode>(nullptr, std::move(name), std::move(params), std::move(body), access);
    }

    std::unique_ptr<StatementNode> Parser::parseDestructorDefinition(AccessLevel access)
    {
        Token name = previousToken;
        consume(TokenType::LPAREN, "Expected '('.");
        if (!check(TokenType::RPAREN))
        {
            logError("Destructors cannot have parameters.");
            while (!check(TokenType::RPAREN) && !check(TokenType::EOFT))
                advance();
        }
        consume(TokenType::RPAREN, "Expected ')'.");
        consume(TokenType::LBRACE, "Expected '{'.");
        auto body = parseBlockStatement();
        return std::make_unique<FunctionDefinitionNode>(nullptr, name, std::vector<std::unique_ptr<ParameterNode>>(), std::move(body), access);
    }

    std::unique_ptr<StatementNode> Parser::parseClassDefinition(bool isRef)
    {
        Token className = currentToken;
        consume(TokenType::IDENTIFIER, "Expected class name.");

        std::vector<std::unique_ptr<VariableNode>> parentNames;

        // [FIX] Removed duplicate inheritance check. Kept only one.
        if (match({TokenType::LPAREN}))
        {
            if (!check(TokenType::RPAREN))
                do
                {
                    parentNames.push_back(std::make_unique<VariableNode>(currentToken));
                    consume(TokenType::IDENTIFIER, "Expected parent class name.");
                } while (match({TokenType::COMMA}));
            consume(TokenType::RPAREN, "Expected ')'.");
        }

        consume(TokenType::LBRACE, "Expected '{'.");
        std::vector<std::unique_ptr<StatementNode>> members;

        while (!check(TokenType::RBRACE) && !check(TokenType::EOFT))
        {
            // [FIX] Parse Access Modifier FIRST
            AccessLevel access = parseAccessModifier(); // Consumes public/private/protected or returns DEFAULT

            if (match({TokenType::KW_CONSTRUCTOR}))
            {
                // Pass 'access' to constructor parser
                members.push_back(parseConstructorDefinition(access));
            }
            else if (match({TokenType::KW_DESTRUCTOR}))
            {
                // Pass 'access' to destructor parser
                members.push_back(parseDestructorDefinition(access));
            }
            else if (check(TokenType::KW_ENUM))
            {
                members.push_back(parseEnumDefinition());
            }
            else
            {
                members.push_back(parseDeclaration(access));
            }
        }

        consume(TokenType::RBRACE, "Expected '}'.");
        return std::make_unique<ClassDefinitionNode>(std::move(className), std::move(parentNames), std::move(members), isRef);
    }

    std::unique_ptr<StatementNode> Parser::parseStructDefinition(bool isPacked, bool isUnion)
    {
        Token name = currentToken;
        consume(TokenType::IDENTIFIER, "Expected struct name.");
        consume(TokenType::LBRACE, "Expected '{'.");

        std::vector<std::unique_ptr<StatementNode>> members;

        while (!check(TokenType::RBRACE) && !check(TokenType::EOFT))
        {
            auto declStmt = parseDeclaration(AccessLevel::PUBLIC);

            if (!declStmt) {
                // If we hit a stray semicolon or invalid token, advance to avoid infinite loop
                if (match({TokenType::SEMICOLON})) continue;
                advance(); 
                continue;
            }

            // [NEW] Bitfield Logic
            if (match({TokenType::COLON}))
            {
                Token sizeToken = currentToken;
                consume(TokenType::INTEGER_LITERAL, "Expected bitfield width.");
                int width = std::stoi(sizeToken.lexeme);

                // Cast to VariableDeclarationNode and set bitWidth
                if (auto varDecl = dynamic_cast<VariableDeclarationNode *>(declStmt.get()))
                {
                    varDecl->bitWidth = width;
                }
                match({TokenType::SEMICOLON});
            }

            members.push_back(std::move(declStmt));
        }

        consume(TokenType::RBRACE, "Expected '}'.");

        // Pass isPacked to constructor
        return std::make_unique<StructDefinitionNode>(std::move(name), std::move(members), isPacked, isUnion);
    }

    std::unique_ptr<ExpressionNode> Parser::parseAsmExpression()
    {
        // 'asm' token already consumed by parsePrimary

        std::unique_ptr<TypeNode> retType = nullptr;

        // Optional Return Type: <int>
        if (check(TokenType::LT))
        {
            advance(); // <
            retType = parseType();
            consume(TokenType::GT, "Expected '>' after asm return type.");
        }

        consume(TokenType::LPAREN, "Expected '(' after asm.");

        // 1. Assembly String
        if (!check(TokenType::STRING_LITERAL))
        {
            logError("Expected assembly template string.");
        }
        std::string asmStr = currentToken.lexeme;
        advance();

        consume(TokenType::COMMA, "Expected comma after assembly string.");

        // 2. Constraints String
        if (!check(TokenType::STRING_LITERAL))
        {
            logError("Expected constraints string.");
        }
        std::string constraints = currentToken.lexeme;
        advance();

        // 3. Arguments (Optional)
        std::vector<std::unique_ptr<ExpressionNode>> args;
        if (match({TokenType::COMMA}))
        {
            do
            {
                args.push_back(parseExpression());
            } while (match({TokenType::COMMA}));
        }

        consume(TokenType::RPAREN, "Expected ')'.");

        return std::make_unique<AsmExpressionNode>(std::move(retType), asmStr, constraints, std::move(args));
    }

    std::unique_ptr<StatementNode> Parser::parseUnsafeBlock()
    {
        consume(TokenType::LBRACE, "Expected '{'.");
        auto body = parseBlockStatement();
        return std::make_unique<UnsafeBlockNode>(std::move(body));
    }
    std::unique_ptr<StatementNode> Parser::parseDeferStatement()
    {
        auto stmt = parseStatement();
        return std::make_unique<DeferStatementNode>(std::move(stmt));
    }
    std::unique_ptr<StatementNode> Parser::parseWithStatement()
    {
        consume(TokenType::LPAREN, "Expected '('.");
        auto decl = parseDeclaration(AccessLevel::DEFAULT);
        consume(TokenType::RPAREN, "Expected ')'.");
        auto body = parseStatement();
        return std::make_unique<WithStatementNode>(std::move(decl), std::move(body));
    }
    std::unique_ptr<StatementNode> Parser::parseThrowStatement()
    {
        auto expr = parseExpression();
        match({TokenType::SEMICOLON});
        return std::make_unique<ThrowStatementNode>(std::move(expr));
    }
    std::unique_ptr<StatementNode> Parser::parseTryStatement()
    {
        consume(TokenType::LBRACE, "Expected '{'.");
        auto tryBlock = parseBlockStatement();
        Token errorVar = {TokenType::ILLEGAL, "", 0, 0};
        std::unique_ptr<BlockStatementNode> catchBlock = nullptr;
        if (match({TokenType::KW_CATCH}))
        {
            consume(TokenType::LPAREN, "Expected '('.");
            errorVar = currentToken;
            consume(TokenType::IDENTIFIER, "Expected error variable.");
            consume(TokenType::RPAREN, "Expected ')'.");
            consume(TokenType::LBRACE, "Expected '{'.");
            catchBlock = parseBlockStatement();
        }
        std::unique_ptr<BlockStatementNode> finallyBlock = nullptr;
        if (match({TokenType::KW_FINALLY}))
        {
            consume(TokenType::LBRACE, "Expected '{'.");
            finallyBlock = parseBlockStatement();
        }
        return std::make_unique<TryStatementNode>(std::move(tryBlock), std::move(catchBlock), errorVar, std::move(finallyBlock));
    }
    std::unique_ptr<StatementNode> Parser::parseBreakStatement()
    {
        Token keyword = previousToken;
        match({TokenType::SEMICOLON});
        return std::make_unique<BreakStatementNode>(keyword);
    }
    std::unique_ptr<StatementNode> Parser::parseContinueStatement()
    {
        Token keyword = previousToken;
        match({TokenType::SEMICOLON});
        return std::make_unique<ContinueStatementNode>(keyword);
    }
    std::unique_ptr<ExpressionNode> Parser::parseDeleteExpression()
    {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'.");
        return std::make_unique<DeleteStatementNode>(std::move(expr));
    }
    std::unique_ptr<ExpressionNode> Parser::parseLengthExpression()
    {
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'.");
        return std::make_unique<LengthExpressionNode>(std::move(expr));
    }
    std::unique_ptr<StatementNode> Parser::parseMacroDefinition()
    {
        Token macroName = currentToken;
        consume(TokenType::IDENTIFIER, "Expected macro name.");
        std::vector<Token> params;
        if (check(TokenType::LPAREN))
        {
            consume(TokenType::LPAREN, "Expected '('.");
            if (!check(TokenType::RPAREN))
                do
                {
                    params.push_back(currentToken);
                    consume(TokenType::IDENTIFIER, "Expected parameter.");
                } while (match({TokenType::COMMA}));
            consume(TokenType::RPAREN, "Expected ')'.");
        }
        consume(TokenType::LBRACE, "Expected '{'.");
        auto body = parseBlockStatement();
        return std::make_unique<MacroDefinitionNode>(macroName, params, std::move(body));
    }
    std::unique_ptr<StatementNode> Parser::parsePrintStatement()
    {
        Token keyword = previousToken;
        consume(TokenType::LPAREN, "Expected '('.");
        auto expr = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'.");
        match({TokenType::SEMICOLON});
        return std::make_unique<PrintStatementNode>(keyword, std::move(expr));
    }

    std::unique_ptr<StatementNode> Parser::parseIfStatement()
    {
        consume(TokenType::LPAREN, "Expected '('.");
        auto condition = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'.");

        auto thenBranch = parseStatement();
        std::unique_ptr<StatementNode> elseBranch = nullptr;

        if (match({TokenType::KW_ELSE}))
        {
            elseBranch = parseStatement();
        }
        else if (match({TokenType::KW_ELSEIF}))
        {
            elseBranch = parseIfStatement();
        }

        return std::make_unique<IfStatementNode>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

    std::unique_ptr<StatementNode> Parser::parseWhileStatement()
    {
        consume(TokenType::LPAREN, "Expected '('.");
        auto condition = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'.");

        auto body = parseStatement();
        return std::make_unique<WhileNode>(std::move(condition), std::move(body));
    }

    std::unique_ptr<StatementNode> Parser::parseForStatement()
    {
        consume(TokenType::LPAREN, "Expected '('.");

        // Check if it's a for-in loop
        if (check(TokenType::IDENTIFIER))
        {
            if (checkPeek(TokenType::KW_IN) || checkPeek(TokenType::COMMA))
            {
                return parseForInStatement();
            }
        }

        return parseCStyleForStatement();
    }

    std::unique_ptr<StatementNode> Parser::parseForInStatement()
    {
        Token keyVar = currentToken;
        consume(TokenType::IDENTIFIER, "Expected key variable.");

        Token valVar = {TokenType::ILLEGAL, "", 0, 0};
        bool hasVal = false;

        if (match({TokenType::COMMA}))
        {
            valVar = currentToken;
            consume(TokenType::IDENTIFIER, "Expected value variable.");
            hasVal = true;
        }

        consume(TokenType::KW_IN, "Expected 'in'.");
        auto iterable = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'.");

        auto body = parseStatement();
        return std::make_unique<ForInNode>(keyVar, valVar, std::move(iterable), std::move(body), hasVal);
    }

    std::unique_ptr<StatementNode> Parser::parseCStyleForStatement()
    {
        std::unique_ptr<StatementNode> initializer;
        if (match({TokenType::SEMICOLON}))
        {
            initializer = nullptr;
        }
        else if (check(TokenType::KW_CONST) || isTypeToken() || (check(TokenType::IDENTIFIER) && (checkPeek(TokenType::IDENTIFIER) || checkPeek(TokenType::STAR))))
        {
            initializer = parseDeclaration(AccessLevel::DEFAULT);
        }
        else
        {
            initializer = parseExpressionStatement();
        }

        std::unique_ptr<ExpressionNode> condition = nullptr;
        if (!check(TokenType::SEMICOLON))
        {
            condition = parseExpression();
        }
        consume(TokenType::SEMICOLON, "Expected ';'.");

        std::unique_ptr<ExpressionNode> increment = nullptr;
        if (!check(TokenType::RPAREN))
        {
            increment = parseExpression();
        }
        consume(TokenType::RPAREN, "Expected ')'.");

        auto body = parseStatement();
        return std::make_unique<ForNode>(std::move(initializer), std::move(condition), std::move(increment), std::move(body));
    }

    std::unique_ptr<StatementNode> Parser::parseDoWhileStatement()
    {
        auto body = parseStatement();
        consume(TokenType::KW_WHILE, "Expected 'while'.");
        consume(TokenType::LPAREN, "Expected '('.");
        auto condition = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'.");
        match({TokenType::SEMICOLON});

        return std::make_unique<DoWhileNode>(std::move(body), std::move(condition));
    }

    std::unique_ptr<StatementNode> Parser::parseTypedef()
    {
        // 'typedef' keyword already matched
        auto type = parseType();
        if (!type)
        {
            logError("Expected type after 'typedef'.");
            return nullptr;
        }

        Token aliasName = currentToken;
        consume(TokenType::IDENTIFIER, "Expected alias name for typedef.");
        match({TokenType::SEMICOLON});

        return std::make_unique<TypedefNode>(std::move(type), aliasName);
    }

    std::unique_ptr<StatementNode> Parser::parseStaticIfStatement()
    {
        // 'static' and 'if' keywords were already matched in parseStatement
        consume(TokenType::LPAREN, "Expected '(' after 'static if'.");
        auto condition = parseExpression();
        consume(TokenType::RPAREN, "Expected ')'.");

        auto thenBranch = parseStatement();
        std::unique_ptr<StatementNode> elseBranch = nullptr;

        if (match({TokenType::KW_ELSE}))
        {
            elseBranch = parseStatement();
        }

        return std::make_unique<StaticIfNode>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
    }

} // namespace Moksha