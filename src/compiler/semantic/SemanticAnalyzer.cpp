#include "moksha/SemanticAnalyzer.hpp"
#include <typeinfo>
#include <algorithm>
#include <map>
#include <cmath>
#include <sstream>
#include <set>
#include <stack>
#include <vector>
#include <string>

namespace Moksha
{

    // --- Static Map Definition (Simulating Global Class/Field Storage) ---
    std::map<std::string, std::map<std::string, std::shared_ptr<Type>>> SemanticAnalyzer::classFieldTypes;
    std::map<std::string, std::set<std::string>> SemanticAnalyzer::classMethodNames;
    std::map<std::string, std::map<size_t, std::shared_ptr<FunctionType>>> SemanticAnalyzer::constructorSignatures;
    std::map<std::string, std::shared_ptr<FunctionType>> SemanticAnalyzer::functionSignatures;
    std::map<std::string, std::map<std::string, std::shared_ptr<Type>>> SemanticAnalyzer::enumMembers;
    std::map<std::string, std::vector<SemanticAnalyzer::VTableEntry>> SemanticAnalyzer::classVTables;
    std::map<std::string, std::map<std::string, int>> SemanticAnalyzer::classVTableIndices;
    std::map<std::string, std::vector<std::string>> SemanticAnalyzer::classParents;
    std::map<std::string, std::map<std::string, std::shared_ptr<Type>>> SemanticAnalyzer::structFieldTypes;

    static std::map<std::string, bool> isRefClass;
    std::map<std::string, std::map<std::string, SemanticAnalyzer::MemberMeta>> SemanticAnalyzer::classMembers;
    static std::map<std::string, std::vector<std::string>> classParents;

    struct LambdaScopeInfo
    {
        LambdaNode *node;
        int scopeDepth; // The depth where this lambda starts
    };

    struct ConstValue
    {
        bool isInt;
        int intVal;
        std::string strVal;
        bool valid = false;
    };

    ConstValue evalConstant(ExpressionNode *node, SymbolTable &symbols)
    {
        // 1. Literal
        if (auto lit = dynamic_cast<LiteralNode *>(node))
        {
            if (lit->token.type == TokenType::INTEGER_LITERAL)
                return {true, std::stoi(lit->token.lexeme), "", true};
            if (lit->token.type == TokenType::STRING_LITERAL)
                return {false, 0, lit->token.lexeme, true};
            if (lit->token.type == TokenType::BOOLEAN_LITERAL)
                return {true, (lit->token.lexeme == "true" ? 1 : 0), "", true};
        }

        // 2. Variable (Look up const)
        if (auto var = dynamic_cast<VariableNode *>(node))
        {
            SymbolInfo *info = symbols.resolve(var->name.lexeme);
            if (info && info->isConst && info->declNode && info->declNode->initializer)
            {
                // Recursively evaluate the initializer of the const variable
                return evalConstant(info->declNode->initializer.get(), symbols);
            }
        }

        return {false, 0, "", false}; // Could not evaluate at compile time
    }

    bool isNumeric(std::shared_ptr<Type> t)
    {
        if (!t)
            return false;
        return std::dynamic_pointer_cast<IntType>(t) ||
               std::dynamic_pointer_cast<DoubleType>(t) ||
               std::dynamic_pointer_cast<FloatType>(t) ||
               std::dynamic_pointer_cast<ShortType>(t) ||
               std::dynamic_pointer_cast<LongType>(t) ||
               std::dynamic_pointer_cast<CharType>(t);
    }

    static std::stack<LambdaScopeInfo> lambdaStack;

    // Converts Type objects to readable strings for debugging error messages
    static std::string typeToString(std::shared_ptr<Type> type)
    {
        if (!type)
            return "unknown";
        if (std::dynamic_pointer_cast<IntType>(type))
            return "int";
        if (std::dynamic_pointer_cast<DoubleType>(type))
            return "double";
        if (std::dynamic_pointer_cast<StringType>(type))
            return "string";
        if (std::dynamic_pointer_cast<CharType>(type))  
            return "char";
        if (std::dynamic_pointer_cast<BooleanType>(type))
            return "boolean";
        if (std::dynamic_pointer_cast<VoidType>(type))
            return "void";
        if (std::dynamic_pointer_cast<NullType>(type))
            return "null";
        if (std::dynamic_pointer_cast<AnyType>(type))
            return "any";

        if (auto cls = std::dynamic_pointer_cast<ClassInstanceType>(type))
        {
            return cls->className;
        }
        if (auto arr = std::dynamic_pointer_cast<ArrayType>(type))
        {
            return typeToString(arr->elementType) + "[]";
        }
        return "complex_type";
    }

    // --- Helper Methods ---
    void SemanticAnalyzer::error(const std::string &message)
    {
        std::cerr << "[Semantic Error] " << message << std::endl;
        hasError = true;
    }

    bool SemanticAnalyzer::analyze(const std::vector<std::unique_ptr<StatementNode>> &statements)
    {
        hasError = false;
        inLoopDepth = 0;
        currentFnReturnType = std::make_shared<VoidType>();
        currentClassName = "";

        functionSignatures.clear();
        constructorSignatures.clear();
        classMethodNames.clear();
        classFieldTypes.clear();

        // 1. malloc(size: int) -> any
        std::vector<std::shared_ptr<Type>> mallocParams = {std::make_shared<IntType>()};
        auto mallocType = std::make_shared<FunctionType>(mallocParams, std::make_shared<AnyType>(), false, 1);
        functionSignatures["malloc"] = mallocType;
        symbolTable.define("malloc", "function", SymbolKind::FUNCTION, nullptr, nullptr, false, false, std::make_shared<AnyType>());

        // 2. free(ptr: any) -> void
        std::vector<std::shared_ptr<Type>> freeParams = {std::make_shared<AnyType>()};
        auto freeType = std::make_shared<FunctionType>(freeParams, std::make_shared<VoidType>(), false, 1);
        functionSignatures["free"] = freeType;
        symbolTable.define("free", "function", SymbolKind::FUNCTION, nullptr, nullptr, false, false, std::make_shared<VoidType>());

        // 3. strcpy(dest: any, src: any) -> any
        std::vector<std::shared_ptr<Type>> strcpyParams = {std::make_shared<AnyType>(), std::make_shared<AnyType>()};
        auto strcpyType = std::make_shared<FunctionType>(strcpyParams, std::make_shared<AnyType>(), false, 2);
        functionSignatures["strcpy"] = strcpyType;
        symbolTable.define("strcpy", "function", SymbolKind::FUNCTION, nullptr, nullptr, false, false, std::make_shared<AnyType>());

        // 4. memcpy(dest: any, src: any, size: int) -> any
        std::vector<std::shared_ptr<Type>> memcpyParams = {std::make_shared<AnyType>(), std::make_shared<AnyType>(), std::make_shared<IntType>()};
        auto memcpyType = std::make_shared<FunctionType>(memcpyParams, std::make_shared<AnyType>(), false, 3);
        functionSignatures["memcpy"] = memcpyType;
        symbolTable.define("memcpy", "function", SymbolKind::FUNCTION, nullptr, nullptr, false, false, std::make_shared<AnyType>());

        // 5. memset(dest: any, val: int, size: int) -> any
        std::vector<std::shared_ptr<Type>> memsetParams = {std::make_shared<AnyType>(), std::make_shared<IntType>(), std::make_shared<IntType>()};
        auto memsetType = std::make_shared<FunctionType>(memsetParams, std::make_shared<AnyType>(), false, 3);
        functionSignatures["memset"] = memsetType;
        symbolTable.define("memset", "function", SymbolKind::FUNCTION, nullptr, nullptr, false, false, std::make_shared<AnyType>());

        // --- END BUILT-INS ---

        for (const auto &stmt : statements)
        {
            if (stmt)
                stmt->accept(*this);
        }
        return !hasError;
    }

    // --- Type Helpers Implementation ---

    std::shared_ptr<Type> SemanticAnalyzer::getTypeFromToken(const Token &token)
    {
        if (token.type == TokenType::KW_INT)
            return std::make_shared<IntType>();
        if (token.type == TokenType::KW_DOUBLE)
            return std::make_shared<DoubleType>();
        if (token.type == TokenType::KW_STRING)
            return std::make_shared<StringType>();
        if (token.type == TokenType::KW_BOOLEAN)
            return std::make_shared<BooleanType>();
        if (token.type == TokenType::KW_VOID)
            return std::make_shared<VoidType>();
        if (token.type == TokenType::KW_NULL)
            return std::make_shared<NullType>();
        if (token.type == TokenType::KW_ANY)
            return std::make_shared<AnyType>();
        if (token.type == TokenType::KW_TABLE)
            return std::make_shared<TableType>();
        if (token.type == TokenType::KW_CHAR)
            return std::make_shared<CharType>();
        if (token.type == TokenType::KW_SHORT)
            return std::make_shared<ShortType>();
        if (token.type == TokenType::KW_LONG)
            return std::make_shared<LongType>();
        if (token.type == TokenType::KW_FLOAT)
            return std::make_shared<FloatType>();

        std::string text = token.lexeme;
        if (text == "int")
            return std::make_shared<IntType>();
        if (text == "double")
            return std::make_shared<DoubleType>();
        if (text == "string")
            return std::make_shared<StringType>();
        if (text == "float")
            return std::make_shared<FloatType>();
        if (text == "boolean")
            return std::make_shared<BooleanType>();
        if (text == "any")
            return std::make_shared<AnyType>();
        if (text == "table")
            return std::make_shared<TableType>();

        SymbolInfo *sym = symbolTable.resolve(text);
        if (sym)
        {
            if (sym->kind == SymbolKind::STRUCT)
            {
                return std::make_shared<StructType>(text);
            }
            if (sym->kind == SymbolKind::ENUM)
            {
                return std::make_shared<IntType>();
            }
        }

        auto alias = symbolTable.resolveType(text);
        if (alias)
        {
            return alias;
        }

        return std::make_shared<ClassInstanceType>(text);
    }

    std::shared_ptr<Type> SemanticAnalyzer::resolveTypeNode(const TypeNode &node)
    {
        std::shared_ptr<Type> type = getTypeFromToken(node.baseTypeToken);
        type->isNullable = node.isNullable;
        for (int i = 0; i < node.pointerDepth; i++)
        {
            type = std::make_shared<PointerType>(type);
        }

        for (const auto &dim : node.dimensions)
        {
            if (dim)
            {
                auto &mutableDim = const_cast<std::unique_ptr<ExpressionNode> &>(dim);
                mutableDim->accept(*this);
                if (!isTypeCompatible(std::make_shared<IntType>(), mutableDim->resolvedType))
                {
                    error("Array size must be an integer.");
                }
            }
        }

        if (node.elementType)
        {
            std::shared_ptr<Type> elementType = resolveTypeNode(*node.elementType);
            auto arrType = std::make_shared<ArrayType>(elementType);
            if (!node.dimensions.empty())
            {
                // The parser stores dimensions in the TypeNode
                if (auto lit = dynamic_cast<LiteralNode *>(node.dimensions[0].get()))
                {
                    if (lit->token.type == TokenType::INTEGER_LITERAL)
                    {
                        arrType->isFixedSize = true;
                        try
                        {
                            arrType->fixedSize = std::stoi(lit->token.lexeme, nullptr, 0);
                        }
                        catch (...)
                        {
                            arrType->fixedSize = 0;
                        }
                    }
                }
            }
            type = arrType;
            type->isNullable = node.isNullable;
        }

        return type;
    }

    // Determines the Least Common Supertype (LCS) for two types (simplified: Int < Double)
    std::shared_ptr<Type> getLeastCommonSupertype(std::shared_ptr<Type> t1, std::shared_ptr<Type> t2)
    {
        if (!t1)
            return t2;
        if (!t2)
            return t1;

        // 1. Identity Check
        if (typeid(*t1) == typeid(*t2))
        {
            if (auto a1 = std::dynamic_pointer_cast<ArrayType>(t1))
            {
                if (auto a2 = std::dynamic_pointer_cast<ArrayType>(t2))
                {
                    auto inner = getLeastCommonSupertype(a1->elementType, a2->elementType);
                    return std::make_shared<ArrayType>(inner);
                }
            }
            return t1;
        }

        // 2. Numeric Promotion
        if (isNumeric(t1) && isNumeric(t2))
        {
            // Double dominates everything
            if (std::dynamic_pointer_cast<DoubleType>(t1) || std::dynamic_pointer_cast<DoubleType>(t2))
                return std::make_shared<DoubleType>();

            // Float dominates integers
            if (std::dynamic_pointer_cast<FloatType>(t1) || std::dynamic_pointer_cast<FloatType>(t2))
                return std::make_shared<FloatType>();

            // Long dominates Int/Short/Char
            if (std::dynamic_pointer_cast<LongType>(t1) || std::dynamic_pointer_cast<LongType>(t2))
                return std::make_shared<LongType>();

            // Default to Int for smaller types
            return std::make_shared<IntType>();
        }

        // 3. String Concatenation
        if (std::dynamic_pointer_cast<StringType>(t1) || std::dynamic_pointer_cast<StringType>(t2))
            return std::make_shared<StringType>();

        return std::make_shared<AnyType>();
    }

    bool SemanticAnalyzer::isTypeCompatible(std::shared_ptr<Type> expected, std::shared_ptr<Type> actual)
    {
        // 1. Any Types (Universal)
        if (typeid(*expected) == typeid(AnyType))
            return true;
        if (typeid(*actual) == typeid(AnyType))
            return true; // Dynamic -> Static allowed (runtime check)

        // 2. Class Compatibility
        if (auto e = std::dynamic_pointer_cast<ClassInstanceType>(expected))
        {
            if (auto a = std::dynamic_pointer_cast<ClassInstanceType>(actual))
            {
                return isSubclass(a->className, e->className);
            }
            if (typeid(*actual) == typeid(NullType))
                return true;
            return false;
        }

        // 3. Array Compatibility (DEEP CHECK)
        if (auto eArr = std::dynamic_pointer_cast<ArrayType>(expected))
        {
            if (auto aArr = std::dynamic_pointer_cast<ArrayType>(actual))
            {
                // If actual is Any[] (e.g. empty literal), allow it
                if (std::dynamic_pointer_cast<AnyType>(aArr->elementType))
                    return true;
                return isTypeCompatible(eArr->elementType, aArr->elementType);
            }
            if (typeid(*actual) == typeid(NullType))
                return true;
            return false;
        }

        // 4. Generic Identity
        if (typeid(*expected) == typeid(*actual))
            return true;

        // 5. Numeric Promotion
        if (isNumeric(expected) && isNumeric(actual))
        {
            // Allow Int/Float -> Double
            if (std::dynamic_pointer_cast<DoubleType>(expected))
                return true;

            // Allow Int -> Float
            if (std::dynamic_pointer_cast<FloatType>(expected))
            {
                return !std::dynamic_pointer_cast<DoubleType>(actual); // Can't demote Double->Float
            }

            // Allow Char/Short -> Int
            if (std::dynamic_pointer_cast<IntType>(expected))
            {
                return std::dynamic_pointer_cast<CharType>(actual) ||
                       std::dynamic_pointer_cast<ShortType>(actual);
            }
        }

        if (std::dynamic_pointer_cast<StringType>(expected) && std::dynamic_pointer_cast<CharType>(actual)) 
        {
            return true;
        }

        // 6. Nullability
        if (expected->isNullable && typeid(*actual) == typeid(NullType))
            return true;
        if (typeid(*actual) == typeid(NullType))
        {
            if (std::dynamic_pointer_cast<StringType>(expected))
                return true;
        }

        if (auto ptr = std::dynamic_pointer_cast<PointerType>(expected))
        {
            if (std::dynamic_pointer_cast<CharType>(ptr->pointeeType))
            {
                if (std::dynamic_pointer_cast<StringType>(actual))
                    return true;
            }
        }

        return false;
    }

    bool SemanticAnalyzer::isIntegralType(std::shared_ptr<Type> type)
    {
        if (!type)
            return false;
        return (std::dynamic_pointer_cast<IntType>(type) ||
                std::dynamic_pointer_cast<CharType>(type) ||
                std::dynamic_pointer_cast<ShortType>(type) ||
                std::dynamic_pointer_cast<LongType>(type));
    }

    bool SemanticAnalyzer::evaluateConstBool(ExpressionNode *node)
    {
        // 1. Handle Binary Comparisons (ARCH == "x86")
        if (auto bin = dynamic_cast<BinaryOpNode *>(node))
        {
            auto left = evalConstant(bin->left.get(), symbolTable);
            auto right = evalConstant(bin->right.get(), symbolTable);

            if (!left.valid || !right.valid)
            {
                error("Static if condition must be a compile-time constant.");
                return false;
            }

            if (bin->op.type == TokenType::EQ)
            {
                if (left.isInt && right.isInt)
                    return left.intVal == right.intVal;
                if (!left.isInt && !right.isInt)
                    return left.strVal == right.strVal;
            }
            if (bin->op.type == TokenType::NEQ)
            {
                if (left.isInt && right.isInt)
                    return left.intVal != right.intVal;
                if (!left.isInt && !right.isInt)
                    return left.strVal != right.strVal;
            }
        }

        // 2. Handle simple booleans (true/false)
        auto val = evalConstant(node, symbolTable);
        if (val.valid && val.isInt)
            return val.intVal != 0;

        return false;
    }

    bool SemanticAnalyzer::isSubclass(const std::string &child, const std::string &parent)
    {
        if (child == parent)
            return true;
        if (classParents.count(child))
        {
            for (const auto &p : classParents[child])
            {
                if (isSubclass(p, parent))
                    return true;
            }
        }
        return false;
    }

    // --- Visitor Implementations ---

    void SemanticAnalyzer::visit(BlockStatementNode &node)
    {
        symbolTable.enterScope();
        for (auto &stmt : node.statements)
        {
            if (stmt)
                stmt->accept(*this);
        }
        symbolTable.exitScope();
    }

    void SemanticAnalyzer::visit(VariableDeclarationNode &node)
    {
        if (symbolTable.isDefinedInCurrentScope(node.name.lexeme))
        {
            error("Variable '" + node.name.lexeme + "' is already defined in this scope.");
            return;
        }

        std::shared_ptr<Type> declaredType = resolveTypeNode(*node.type);
        node.type->resolvedType = declaredType;

        if (node.initializer)
        {
            if (dynamic_cast<LambdaNode *>(node.initializer.get()))
            {
                if (!std::dynamic_pointer_cast<AnyType>(declaredType))
                {
                    error("Type mismatch: Lambdas must be assigned to variables of type 'any'. Variable '" + node.name.lexeme + "' is declared as '" + typeToString(declaredType) + "'.");
                }
            }
            node.initializer->accept(*this);
            if (!isTypeCompatible(declaredType, node.initializer->resolvedType))
            {
                error("Type mismatch at line " + std::to_string(node.name.line) +
                      ". Variable '" + node.name.lexeme + "' expects '" + typeToString(declaredType) +
                      "' but got '" + typeToString(node.initializer->resolvedType) + "'.");
            }
        }

        symbolTable.define(
            node.name.lexeme,
            node.type->baseTypeToken.lexeme,
            SymbolKind::VARIABLE,
            nullptr, nullptr,
            node.isConst,
            node.type->isNullable,
            declaredType,
            &node,
            node.isVolatile,
            node.isExtern,
            node.alignment,
            node.section);
    }

    void SemanticAnalyzer::visit(VariableNode &node)
    {
        SymbolInfo *info = symbolTable.resolve(node.name.lexeme);
        if (!info)
        {
            error("Undefined variable '" + node.name.lexeme + "'.");
            node.resolvedType = std::make_shared<AnyType>();
            return;
        }

        // --- CAPTURE DETECTION & HEAP PROMOTION ---
        if (!lambdaStack.empty())
        {
            LambdaScopeInfo &activeLambda = lambdaStack.top();

            // If the variable is defined in a scope *above* the current lambda
            if (info->scopeDepth < activeLambda.scopeDepth)
            {

                // 1. Promote the Original Declaration to Heap
                if (info->declNode)
                {
                    info->declNode->isHeapAllocated = true; // <--- CRITICAL FIX
                }

                // 2. Register the Capture
                bool alreadyCaptured = false;
                for (auto &cap : activeLambda.node->captures)
                {
                    if (cap.name == node.name.lexeme)
                    {
                        alreadyCaptured = true;
                        break;
                    }
                }

                if (!alreadyCaptured)
                {
                    // Determine if we capture by Ref (Heap Pointer) or Value
                    // If declNode exists, we promoted it, so we capture by Ref.
                    bool isRef = (info->declNode != nullptr);

                    activeLambda.node->captures.push_back({
                        node.name.lexeme,
                        info->resolvedType,
                        false,
                        isRef // <--- CRITICAL FIX: Tell CodeGen this is a pointer capture
                    });
                }
            }
        }

        // Standard Type Resolution (Keep existing)
        if (info->type == "table" || info->type == "table?")
        {
            auto tableType = std::make_shared<TableType>();
            tableType->isNullable = (info->type.back() == '?');
            node.resolvedType = tableType;
        }
        else if (info->kind == SymbolKind::ENUM)
        {
            node.resolvedType = std::make_shared<ClassInstanceType>(info->name);
        }
        else if (info->resolvedType)
        {
            node.resolvedType = info->resolvedType;
        }
        else
        {
            node.resolvedType = std::make_shared<AnyType>();
        }
    }

    void SemanticAnalyzer::visit(BinaryOpNode &node)
    {
        node.left->accept(*this);
        node.right->accept(*this);

        if (node.op.type == TokenType::PLUS)
        {
            bool lStr = (std::dynamic_pointer_cast<StringType>(node.left->resolvedType) != nullptr);
            bool rStr = (std::dynamic_pointer_cast<StringType>(node.right->resolvedType) != nullptr);
            if (lStr || rStr)
            {
                node.resolvedType = std::make_shared<StringType>();
                return;
            }
        }

        TokenType op = node.op.type;
        bool isBitwise = (op == TokenType::BITWISE_OR || op == TokenType::BITWISE_XOR ||
                          op == TokenType::AMPERSAND || op == TokenType::LSHIFT ||
                          op == TokenType::RSHIFT);

        if (isBitwise)
        {
            if (!isIntegralType(node.left->resolvedType) || !isIntegralType(node.right->resolvedType))
            {
                error("Bitwise operators can only be applied to Integers.");
                node.resolvedType = std::make_shared<IntType>(); // Error recovery
                return;
            }
        }

        if (op == TokenType::PLUS || op == TokenType::MINUS)
        {
            bool leftIsPtr = (std::dynamic_pointer_cast<ArrayType>(node.left->resolvedType) != nullptr ||
                              std::dynamic_pointer_cast<PointerType>(node.left->resolvedType) != nullptr);
            bool rightIsInt = isIntegralType(node.right->resolvedType);

            if (leftIsPtr && rightIsInt)
            {
                // 1. Check if it is a String (char* or char[])
                bool isString = false;

                // Check PointerType -> uses 'pointeeType' (not baseType)
                if (auto ptr = std::dynamic_pointer_cast<PointerType>(node.left->resolvedType))
                {
                    if (std::dynamic_pointer_cast<CharType>(ptr->pointeeType))
                    {
                        isString = true;
                    }
                }
                // Check ArrayType -> uses 'elementType'
                else if (auto arr = std::dynamic_pointer_cast<ArrayType>(node.left->resolvedType))
                {
                    if (std::dynamic_pointer_cast<CharType>(arr->elementType))
                    {
                        isString = true;
                    }
                }

                // 2. Allow String Concatenation
                if (isString && op == TokenType::PLUS)
                {
                    node.resolvedType = std::make_shared<StringType>();
                    return;
                }

                node.resolvedType = node.left->resolvedType;
                return;
            }
        }

        if (auto cls = std::dynamic_pointer_cast<ClassInstanceType>(node.left->resolvedType))
        {
            if (op == TokenType::PLUS || op == TokenType::MINUS || op == TokenType::STAR || op == TokenType::SLASH)
            {
                node.resolvedType = node.left->resolvedType;
                return;
            }
        }

        if (node.op.type == TokenType::ASSIGN)
        {
            if (auto varNode = dynamic_cast<VariableNode *>(node.left.get()))
            {
                SymbolInfo *info = symbolTable.resolve(varNode->name.lexeme);
                if (info && info->isConst)
                {
                    error("Cannot assign to const variable '" + varNode->name.lexeme + "'.");
                }
            }
            if (!isTypeCompatible(node.left->resolvedType, node.right->resolvedType))
            {
                // DETAILED ERROR MESSAGE
                error("Type mismatch in assignment at line " + std::to_string(node.op.line) +
                      ". Cannot assign '" + typeToString(node.right->resolvedType) +
                      "' to '" + typeToString(node.left->resolvedType) + "'.");
            }
            node.resolvedType = node.left->resolvedType;
        }
        else
        {
            // Binary operation type inference (LCS check, simplified)
            node.resolvedType = getLeastCommonSupertype(node.left->resolvedType, node.right->resolvedType);
        }
    }

    void SemanticAnalyzer::visit(FunctionDefinitionNode &node)
    {
        if (!symbolTable.define(node.name.lexeme, "function", SymbolKind::FUNCTION, nullptr, nullptr, false, false, nullptr, nullptr,
                                false, node.isExtern, 0, node.section))
        {
            error("Function '" + node.name.lexeme + "' is already defined.");
        }

        std::shared_ptr<Type> oldFnType = currentFnReturnType;
        if (node.returnType)
        {
            currentFnReturnType = resolveTypeNode(*node.returnType);
            node.returnType->resolvedType = currentFnReturnType;
        }
        else
        {
            // Constructors/Destructors implicitly return Void
            currentFnReturnType = std::make_shared<VoidType>();
        }

        std::vector<std::shared_ptr<Type>> paramTypes;
        int minArgs = 0;

        for (auto &param : node.parameters)
        {
            if (param->type)
            {
                auto t = resolveTypeNode(*param->type);
                param->type->resolvedType = t; // CRITICAL: Save param types for CodeGen
                paramTypes.push_back(t);
            }
            else
            {
                paramTypes.push_back(std::make_shared<AnyType>());
            }
            if (param->defaultValue == nullptr)
            {
                minArgs++;
            }
        }

        auto fnType = std::make_shared<FunctionType>(std::move(paramTypes), currentFnReturnType, node.isAsync, minArgs, node.isVariadic);

        // Check if this is a constructor (no return type, specific name like "constructor")
        if (node.name.lexeme == "constructor")
        {
            // Store constructor signature by parameter count
            constructorSignatures[currentClassName][fnType->parameterTypes.size()] = fnType;
        }
        else
        {
            functionSignatures[node.name.lexeme] = fnType;
        }

        symbolTable.enterScope();

        for (auto &param : node.parameters)
        {
            std::shared_ptr<Type> pType = param->type ? param->type->resolvedType : std::make_shared<AnyType>();
            symbolTable.define(
                param->name.lexeme,
                param->type ? param->type->baseTypeToken.lexeme : "any",
                SymbolKind::PARAMETER,
                nullptr, nullptr, false, false,
                pType // <--- The important part
            );
            if (param->defaultValue)
                param->defaultValue->accept(*this);
        }

        if (node.body)
        {
            symbolTable.enterScope();
            node.body->accept(*this);
            symbolTable.exitScope();
        }
        else if (!node.isExtern)
        {
            error("Non-extern function '" + node.name.lexeme + "' must have a body.");
        }
        currentFnReturnType = oldFnType;
    }

    void SemanticAnalyzer::visit(ReturnNode &node)
    {
        if (node.value)
        {
            node.value->accept(*this);

            if (typeid(*currentFnReturnType) != typeid(VoidType))
            {
                if (!isTypeCompatible(currentFnReturnType, node.value->resolvedType))
                {
                    error("Return type mismatch.");
                }
            }
        }
        else
        {
            if (typeid(*currentFnReturnType) != typeid(VoidType))
            {
                error("Function must return a value.");
            }
        }
    }

    // --- Control Flow (Conditional Checks Implemented) ---

    void SemanticAnalyzer::checkConditionalType(ExpressionNode *cond)
    {
        if (!cond)
            return;

        std::shared_ptr<Type> type = cond->resolvedType;

        // Allow Boolean and Integer (Standard)
        if (isNumeric(type) || std::dynamic_pointer_cast<BooleanType>(type))
            return;

        // Allow Doubles (0.0 is false)
        if (typeid(*type) == typeid(DoubleType))
        {
            return;
        }

        // Allow Pointers/Objects (Strings, Arrays, Tables, Classes, Any). These will be checked for NULL or Empty in CodeGen
        if (std::dynamic_pointer_cast<StringType>(type) ||
            std::dynamic_pointer_cast<ArrayType>(type) ||
            std::dynamic_pointer_cast<TableType>(type) ||
            std::dynamic_pointer_cast<ClassInstanceType>(type) ||
            std::dynamic_pointer_cast<AnyType>(type) ||
            std::dynamic_pointer_cast<PointerType>(type)) // Pointers are valid conditions
        {
            return;
        }

        // Only NullType or VoidType are strictly forbidden conditions
        if (typeid(*type) == typeid(NullType) || typeid(*type) == typeid(VoidType))
        {
            error("Conditional expression cannot be null or void.");
            return;
        }
    }

    void SemanticAnalyzer::visit(IfStatementNode &node)
    {
        node.condition->accept(*this);
        checkConditionalType(node.condition.get());
        node.thenBranch->accept(*this);
        if (node.elseBranch)
            node.elseBranch->accept(*this);
    }

    void SemanticAnalyzer::visit(WhileNode &node)
    {
        inLoopDepth++;
        node.condition->accept(*this);
        checkConditionalType(node.condition.get());
        node.body->accept(*this);
        inLoopDepth--;
    }

    void SemanticAnalyzer::visit(DoWhileNode &node)
    {
        inLoopDepth++;
        node.body->accept(*this);
        node.condition->accept(*this);
        checkConditionalType(node.condition.get());
        inLoopDepth--;
    }

    void SemanticAnalyzer::visit(ForNode &node)
    {
        symbolTable.enterScope();
        inLoopDepth++;
        if (node.initializer)
            node.initializer->accept(*this);
        if (node.condition)
        {
            node.condition->accept(*this);
            checkConditionalType(node.condition.get());
        }
        if (node.increment)
            node.increment->accept(*this);

        node.body->accept(*this);
        inLoopDepth--;
        symbolTable.exitScope();
    }

    void SemanticAnalyzer::visit(ForInNode &node)
    {
        symbolTable.enterScope();
        inLoopDepth++;

        // 1. Analyze the iterable expression
        node.iterable->accept(*this);
        std::shared_ptr<Type> type = node.iterable->resolvedType;

        // 2. Validate Type (Must be Array, String, or Table)
        bool isArray = (std::dynamic_pointer_cast<ArrayType>(type) != nullptr);
        bool isString = (std::dynamic_pointer_cast<StringType>(type) != nullptr);
        bool isTable = (std::dynamic_pointer_cast<TableType>(type) != nullptr);

        if (!isArray && !isString && !isTable)
        {
            error("'for...in' can only iterate over Arrays, Strings, or Tables.");
        }

        // 3. Determine Key and Value Types
        std::shared_ptr<Type> keyType = std::make_shared<AnyType>();
        std::shared_ptr<Type> valType = std::make_shared<AnyType>();

        if (isArray)
        {
            keyType = std::make_shared<IntType>();                             // Index
            valType = std::dynamic_pointer_cast<ArrayType>(type)->elementType; // Element
        }
        else if (isString)
        {
            keyType = std::make_shared<IntType>();    // Index
            valType = std::make_shared<StringType>(); // Character (as String)
        }
        else if (isTable)
        {
            keyType = std::make_shared<AnyType>(); // Hash Key (Unknown Type)
            valType = std::make_shared<AnyType>(); // Hash Value (Unknown Type)
        }

        // 4. Define Loop Variables in Scope
        if (node.hasValueVar)
        {
            // Syntax: for (k, v in obj)
            symbolTable.define(node.keyVar.lexeme, "iter_key", SymbolKind::VARIABLE, nullptr, nullptr, false, false, keyType);
            symbolTable.define(node.valVar.lexeme, "iter_val", SymbolKind::VARIABLE, nullptr, nullptr, false, false, valType);
        }
        else
        {
            if (isTable)
            {
                symbolTable.define(node.keyVar.lexeme, "iter_key", SymbolKind::VARIABLE, nullptr, nullptr, false, false, keyType);
            }
            else
            {
                symbolTable.define(node.keyVar.lexeme, "iter_val", SymbolKind::VARIABLE, nullptr, nullptr, false, false, valType);
            }
        }

        node.body->accept(*this);
        inLoopDepth--;
        symbolTable.exitScope();
    }

    void SemanticAnalyzer::visit(BreakStatementNode &node)
    {
        if (inLoopDepth == 0)
        {
            error("'break' statement used outside of a loop.");
        }
    }

    void SemanticAnalyzer::visit(ContinueStatementNode &node)
    {
        if (inLoopDepth == 0)
        {
            error("'continue' statement used outside of a loop.");
        }
    }

    void SemanticAnalyzer::visit(ClassDefinitionNode &node)
    {
        if (!symbolTable.define(node.name.lexeme, "class", SymbolKind::CLASS))
            error("Class '" + node.name.lexeme + "' is already defined.");

        isRefClass[node.name.lexeme] = node.isRef;
        std::string oldClassName = currentClassName;
        currentClassName = node.name.lexeme;
        classMap[node.name.lexeme] = &node;

        std::map<std::string, std::shared_ptr<Type>> fields;
        std::map<std::string, MemberMeta> members;
        std::vector<std::string> parentList;

        // [NEW] VTable Construction
        std::vector<VTableEntry> vtable;
        std::map<std::string, int> vtableIndices;

        // 1. Inherit from Parents
        for (auto &parentVar : node.parentClassNames)
        {
            // FIX: Use arrow operator for unique_ptr
            std::string pName = parentVar->name.lexeme;
            parentList.push_back(pName);

            // Inherit Members
            if (classMembers.count(pName))
            {
                for (auto const &[memName, meta] : classMembers[pName])
                {
                    if (memName != "constructor")
                    {
                        members[memName] = meta;
                    }
                }
            }
            // Inherit Fields
            if (classFieldTypes.count(pName))
            {
                for (auto const &[fName, fType] : classFieldTypes[pName])
                    fields[fName] = fType;
            }
            // Inherit VTable
            if (classVTables.count(pName))
            {
                for (const auto &entry : classVTables[pName])
                {
                    if (vtableIndices.find(entry.methodName) == vtableIndices.end())
                    {
                        vtableIndices[entry.methodName] = vtable.size();
                        vtable.push_back(entry);
                    }
                }
            }
        }
        classParents[node.name.lexeme] = parentList;

        // 2. Process Own Members
        for (auto &member : node.members)
        {
            if (auto varDecl = dynamic_cast<VariableDeclarationNode *>(member.get()))
            {
                std::shared_ptr<Type> t = resolveTypeNode(*varDecl->type);
                if (!varDecl->isShared)
                {
                    fields[varDecl->name.lexeme] = t;
                    MemberMeta meta;
                    meta.type = t;
                    meta.access = varDecl->visibility;
                    members[varDecl->name.lexeme] = meta;
                }
            }
            else if (auto funcDecl = dynamic_cast<FunctionDefinitionNode *>(member.get()))
            {
                std::string methodName = funcDecl->name.lexeme;
                std::string implName = currentClassName + "_" + methodName;

                // Analyze Method Signature
                std::shared_ptr<Type> retType = funcDecl->returnType ? resolveTypeNode(*funcDecl->returnType) : std::make_shared<VoidType>();
                std::vector<std::shared_ptr<Type>> paramTypes;
                for (auto &p : funcDecl->parameters)
                {
                    paramTypes.push_back(p->type ? resolveTypeNode(*p->type) : std::make_shared<AnyType>());
                }

                // FIX: Pass 4 arguments to FunctionType constructor
                auto sig = std::make_shared<FunctionType>(paramTypes, retType, funcDecl->isAsync, 0);

                // --- VTABLE LOGIC ---
                // Only add to VTable if it is NOT a constructor/destructor
                bool isSpecial = (methodName == "constructor" || methodName == "destructor");

                if (!isSpecial)
                {
                    if (vtableIndices.count(methodName))
                    {
                        int idx = vtableIndices[methodName];
                        vtable[idx].implName = implName;
                        vtable[idx].signature = sig;
                    }
                    else
                    {
                        vtableIndices[methodName] = vtable.size();
                        vtable.push_back({methodName, implName, sig});
                    }
                }

                // --- MEMBER REGISTRATION ---
                MemberMeta meta;
                meta.type = sig;
                meta.access = funcDecl->visibility;
                members[methodName] = meta;
            }
        }

        // --- 3. IMPLICIT CONSTRUCTOR INHERITANCE ---
        bool hasUserCtor = false;
        if (members.count("constructor"))
        {
            hasUserCtor = true;
        }

        // If NO constructor, and has Parent -> Inherit Parent's Signature
        if (!hasUserCtor && !node.parentClassNames.empty())
        {
            std::string parentName = node.parentClassNames[0]->name.lexeme;

            if (classMap.count(parentName))
            {
                ClassDefinitionNode *parentAST = classMap[parentName];

                FunctionDefinitionNode *parentCtor = nullptr;
                for (auto &pm : parentAST->members)
                {
                    if (auto func = dynamic_cast<FunctionDefinitionNode *>(pm.get()))
                    {
                        if (func->name.lexeme == "constructor")
                        {
                            parentCtor = func;
                            break;
                        }
                    }
                }

                // Create the signature
                std::vector<std::shared_ptr<Type>> paramTypes;
                if (parentCtor)
                {
                    for (auto &p : parentCtor->parameters)
                    {
                        paramTypes.push_back(p->type ? resolveTypeNode(*p->type) : std::make_shared<AnyType>());
                    }
                }

                // Create Function Type
                auto ctorType = std::make_shared<FunctionType>(paramTypes, std::make_shared<VoidType>(), false, 0);

                // A. Register in Members Map
                MemberMeta meta;
                meta.type = ctorType;
                meta.access = AccessLevel::PUBLIC;
                members["constructor"] = meta;

                // B. CRITICAL FIX: Register in Global Constructor Signatures Map
                // This is what NewExpression uses for validation!
                constructorSignatures[node.name.lexeme][paramTypes.size()] = ctorType;
            }
        }

        // Save Global State
        classFieldTypes[node.name.lexeme] = fields;
        classMembers[node.name.lexeme] = members;
        classVTables[node.name.lexeme] = vtable;
        classVTableIndices[node.name.lexeme] = vtableIndices;

        // Analyze Bodies
        symbolTable.enterScope();
        symbolTable.define("this", node.name.lexeme, SymbolKind::VARIABLE, nullptr, nullptr, true);

        for (auto &member : node.members)
            if (member)
                member->accept(*this);
        symbolTable.exitScope();
        currentClassName = oldClassName;
    }

    void SemanticAnalyzer::visit(StructDefinitionNode &node)
    {
        std::string structName = node.name.lexeme;
        int currentBitOffset = 0;

        // 1. Analyze and Register Fields FIRST
        for (auto &member : node.members)
        {
            if (auto var = dynamic_cast<VariableDeclarationNode *>(member.get()))
            {
                var->accept(*this);
                if (var->bitWidth > 0)
                {
                    // 1. Ensure type is integer
                    if (!std::dynamic_pointer_cast<IntType>(var->type->resolvedType) &&
                        !std::dynamic_pointer_cast<CharType>(var->type->resolvedType))
                    {
                        error("Bitfields must be integer or char types.");
                    }

                    // 2. Update offsets
                    // (Real packing logic happens in CodeGen, but we validate here)
                    if (var->bitWidth > 32)
                    { // Assuming int is 32-bit
                        error("Bitfield width exceeds type size.");
                    }

                    currentBitOffset += var->bitWidth;
                }
                else
                {
                    // Reset bit offset for normal fields
                    currentBitOffset = 0;
                }
                if (var->type && var->type->resolvedType)
                {
                    structFieldTypes[structName][var->name.lexeme] = var->type->resolvedType;
                }
                else
                {
                    structFieldTypes[structName][var->name.lexeme] = std::make_shared<AnyType>();
                }
            }
            else
            {
                error("Structs can only contain fields, not methods.");
            }
        }

        // 2. Define the symbol if it's not already there
        if (!symbolTable.resolve(structName))
        {
            symbolTable.define(structName, "struct", SymbolKind::STRUCT);
        }
    }

    void SemanticAnalyzer::visit(AsmExpressionNode &node)
    {
        // Resolve return type
        if (node.returnType)
        {
            node.returnType->resolvedType = resolveTypeNode(*node.returnType);
            node.resolvedType = node.returnType->resolvedType;
        }
        else
        {
            node.resolvedType = std::make_shared<VoidType>();
        }

        // Visit arguments to ensure variables exist
        for (auto &arg : node.arguments)
        {
            arg->accept(*this);
        }
    }

    // --- Expression Nodes ---

    void SemanticAnalyzer::visit(LiteralNode &node)
    {
        if (node.token.type == TokenType::INTEGER_LITERAL)
            node.resolvedType = std::make_shared<IntType>();
        else if (node.token.type == TokenType::FLOAT_LITERAL)
            node.resolvedType = std::make_shared<DoubleType>();
        else if (node.token.type == TokenType::BOOLEAN_LITERAL)
            node.resolvedType = std::make_shared<BooleanType>();
        else if (node.token.type == TokenType::CHAR_LITERAL)
            node.resolvedType = std::make_shared<CharType>();
        else if (node.token.type == TokenType::STRING_LITERAL || node.token.type == TokenType::STRING_CHUNK)
            node.resolvedType = std::make_shared<StringType>();
        else if (node.token.type == TokenType::KW_NULL)
            node.resolvedType = std::make_shared<NullType>();
        else
            node.resolvedType = std::make_shared<AnyType>();
    }

    void SemanticAnalyzer::visit(ThisExpression &node)
    {
        SymbolInfo *info = symbolTable.resolve("this");
        if (!info)
        {
            error("'this' can only be used inside a class.");
            node.resolvedType = std::make_shared<AnyType>();
        }
        else
        {
            node.resolvedType = std::make_shared<ClassInstanceType>(info->type);
        }
    }

    void SemanticAnalyzer::visit(NewExpression &node)
    {
        SymbolInfo *info = symbolTable.resolve(node.className.lexeme);
        if (!info || info->kind != SymbolKind::CLASS)
        {
            error("Undefined class '" + node.className.lexeme + "'.");
            node.resolvedType = std::make_shared<AnyType>();
            return;
        }

        for (auto &arg : node.arguments)
            arg->accept(*this);

        // --- Constructor Signature Check ---
        size_t argCount = node.arguments.size();
        if (constructorSignatures.count(node.className.lexeme) &&
            constructorSignatures.at(node.className.lexeme).count(argCount))
        {
            auto &signature = constructorSignatures.at(node.className.lexeme).at(argCount);
            for (size_t i = 0; i < argCount; ++i)
            {
                if (!isTypeCompatible(signature->parameterTypes[i], node.arguments[i]->resolvedType))
                {
                    error("Argument " + std::to_string(i) + " type mismatch in constructor call for '" + node.className.lexeme + "'.");
                }
            }
        }
        else if (argCount > 0)
        {
            // Only complain if args were passed but no matching constructor was found
            error("No constructor matching " + std::to_string(argCount) + " arguments found for '" + node.className.lexeme + "'.");
        }

        node.resolvedType = std::make_shared<ClassInstanceType>(node.className.lexeme);
    }

    void SemanticAnalyzer::visit(UpdateNode &node)
    {
        node.argument->accept(*this);
        if (auto varNode = dynamic_cast<VariableNode *>(node.argument.get()))
        {
            SymbolInfo *info = symbolTable.resolve(varNode->name.lexeme);
            if (info && info->isConst)
            {
                error("Cannot modify const variable '" + varNode->name.lexeme + "'.");
            }
        }
        node.resolvedType = node.argument->resolvedType;
    }

    void SemanticAnalyzer::visit(MemberAccessNode &node)
    {
        node.object->accept(*this);
        std::shared_ptr<Type> objType = node.object->resolvedType;

        if (auto classType = std::dynamic_pointer_cast<ClassInstanceType>(objType))
        {
            std::string typeName = classType->className;
            std::string memberName = node.name.lexeme;

            // 1. Check Access Control via classMembers
            if (classMembers.count(typeName) && classMembers.at(typeName).count(memberName))
            {
                MemberMeta meta = classMembers.at(typeName).at(memberName);
                if (classMembers.count(typeName) && classMembers.at(typeName).count(memberName))
                {
                    node.resolvedType = meta.type;
                    return;
                }

                // --- ENFORCE ENCAPSULATION ---
                bool isInternal = (currentClassName == typeName);
                bool isChild = isSubclass(currentClassName, typeName);

                if (meta.access == AccessLevel::PRIVATE)
                {
                    if (!isInternal)
                    {
                        error("Cannot access private member '" + memberName + "' of class '" + typeName + "'.");
                    }
                }
                else if (meta.access == AccessLevel::PROTECTED)
                {
                    if (!isInternal && !isChild)
                    {
                        error("Cannot access protected member '" + memberName + "' of class '" + typeName + "'.");
                    }
                }
                return;
            }

            // 2. Check Enum Members (Public by default)
            if (enumMembers.count(typeName) && enumMembers.at(typeName).count(node.name.lexeme))
            {
                node.resolvedType = enumMembers.at(typeName).at(node.name.lexeme);
                return;
            }

            error("Member '" + node.name.lexeme + "' does not exist on type '" + typeName + "'.");
            node.resolvedType = std::make_shared<AnyType>();
            return;
        }
        else if (auto structType = std::dynamic_pointer_cast<StructType>(node.object->resolvedType))
        {
            std::string structName = structType->structName;
            std::string memberName = node.name.lexeme;

            // Check if the struct has this field
            if (structFieldTypes.count(structName) && structFieldTypes[structName].count(memberName))
            {
                node.resolvedType = structFieldTypes[structName][memberName];
                return;
            }
            else
            {
                error("Struct '" + structName + "' has no field named '" + memberName + "'.");
                node.resolvedType = std::make_shared<AnyType>();
            }
        }
        else if (auto ptrType = std::dynamic_pointer_cast<PointerType>(node.object->resolvedType))
        {
            if (auto pointedStruct = std::dynamic_pointer_cast<StructType>(ptrType->pointeeType))
            {
                std::string structName = pointedStruct->structName;
                std::string memberName = node.name.lexeme;

                if (structFieldTypes.count(structName) && structFieldTypes[structName].count(memberName))
                {
                    node.resolvedType = structFieldTypes[structName][memberName];
                    return;
                }
                else
                {
                    error("Struct '" + structName + "' has no field named '" + memberName + "'.");
                    node.resolvedType = std::make_shared<AnyType>();
                }
            }
            else
            {
                error("Cannot access member '" + node.name.lexeme + "' on a pointer to non-struct type.");
                node.resolvedType = std::make_shared<AnyType>();
            }
        }
    }

    void SemanticAnalyzer::visit(InputExpression &node)
    {
        if (node.token.type == TokenType::KW_READINT)
            node.resolvedType = std::make_shared<IntType>();
        else if (node.token.type == TokenType::KW_READDOUBLE)
            node.resolvedType = std::make_shared<DoubleType>();
        else if (node.token.type == TokenType::KW_READBOOL)
            node.resolvedType = std::make_shared<BooleanType>();
        else
            node.resolvedType = std::make_shared<StringType>();
    }

    void SemanticAnalyzer::visit(UnaryOpNode &node)
    {
        // 1. Handle Address-Of (&)
        if (node.op.type == TokenType::AMPERSAND)
        {
            // [NEW] Check if we are taking the address of a Function name
            if (auto varNode = dynamic_cast<VariableNode *>(node.right.get()))
            {
                SymbolInfo *info = symbolTable.resolve(varNode->name.lexeme);
                if (info && info->kind == SymbolKind::FUNCTION)
                {
                    // Treat raw function pointers as 'ptr' (void*)
                    node.resolvedType = std::make_shared<PointerType>(std::make_shared<VoidType>());
                    return;
                }
            }

            // Standard Variable Address Logic
            node.right->accept(*this);
            node.resolvedType = std::make_shared<PointerType>(node.right->resolvedType);
            return;
        }

        // 2. Handle Dereference (*)
        node.right->accept(*this);
        if (node.op.type == TokenType::STAR)
        {
            if (auto ptrType = std::dynamic_pointer_cast<PointerType>(node.right->resolvedType))
            {
                node.resolvedType = ptrType->pointeeType;
            }
            else if (auto arrType = std::dynamic_pointer_cast<ArrayType>(node.right->resolvedType))
            {
                node.resolvedType = arrType->elementType;
            }
            else
            {
                error("Cannot dereference non-pointer type '" + typeToString(node.right->resolvedType) + "'.");
                node.resolvedType = std::make_shared<AnyType>();
            }
            return;
        }

        // 3. Handle Logical/Bitwise Not
        if (node.op.type == TokenType::NOT)
        {
            node.resolvedType = std::make_shared<BooleanType>();
            return;
        }

        // Default propogation
        node.resolvedType = node.right->resolvedType;
    }

    void SemanticAnalyzer::visit(TernaryOpNode &node)
    {
        node.condition->accept(*this);
        checkConditionalType(node.condition.get());
        node.thenBranch->accept(*this);
        node.elseBranch->accept(*this);

        // LCS check for ternary result
        node.resolvedType = getLeastCommonSupertype(node.thenBranch->resolvedType, node.elseBranch->resolvedType);
    }

    void SemanticAnalyzer::visit(GroupingNode &node)
    {
        node.expression->accept(*this);
        node.resolvedType = node.expression->resolvedType;
    }

    void SemanticAnalyzer::visit(CallNode &node)
    {
        node.callee->accept(*this);
        for (auto &arg : node.arguments)
            arg->accept(*this);

        if (auto memAccess = dynamic_cast<MemberAccessNode *>(node.callee.get()))
        {
            std::shared_ptr<Type> objType = memAccess->object->resolvedType;
            if (auto cls = std::dynamic_pointer_cast<ClassInstanceType>(objType))
            {
                if (classVTableIndices.count(cls->className) &&
                    classVTableIndices[cls->className].count(memAccess->name.lexeme))
                {
                    int idx = classVTableIndices[cls->className][memAccess->name.lexeme];
                    auto &entry = classVTables[cls->className][idx];
                    bool isVariadic = entry.signature->isVariadic; // Assumes you updated FunctionType in ast.hpp
                    size_t actualArgs = node.arguments.size();
                    size_t paramCount = entry.signature->parameterTypes.size();

                    if (actualArgs < entry.signature->minArgCount || (!isVariadic && actualArgs > paramCount))
                    {
                        error("Method '" + memAccess->name.lexeme + "' expects " +
                              (isVariadic ? "at least " : "") + std::to_string(paramCount) + " arguments.");
                    }
                }
            }
        }

        std::string funcName = "";
        if (auto varNode = dynamic_cast<VariableNode *>(node.callee.get()))
        {
            funcName = varNode->name.lexeme;
        }

        // --- CHECK CLASS CAST ---
        SymbolInfo *symbol = symbolTable.resolve(funcName);
        if (symbol && symbol->kind == SymbolKind::CLASS)
        {
            if (node.arguments.size() != 1)
            {
                error("Explicit class casting for '" + funcName + "' requires exactly 1 argument.");
            }
            else
            {
                node.arguments[0]->accept(*this);
                node.resolvedType = std::make_shared<ClassInstanceType>(funcName);
            }
            return;
        }

        if (functionSignatures.count(funcName))
        {
            auto &signature = functionSignatures.at(funcName);
            size_t actualArgs = node.arguments.size();
            size_t paramCount = signature->parameterTypes.size();
            bool isVariadic = signature->isVariadic; // Ensure this flag is passed from FunctionDefinitionNode

            // [FIXED LOGIC]
            if (actualArgs < signature->minArgCount || (!isVariadic && actualArgs > paramCount))
            {
                error("Function '" + funcName + "' expected " +
                      (isVariadic ? "at least " : "exactly ") +
                      std::to_string(paramCount) + " arguments, but got " +
                      std::to_string(actualArgs) + ".");
            }
            else
            {
                // Only type-check arguments that have a defined parameter type
                for (size_t i = 0; i < paramCount; ++i)
                {
                    if (i < actualArgs)
                    {
                        // ... (your existing Lambda and isTypeCompatible checks) ...
                        if (!isTypeCompatible(signature->parameterTypes[i], node.arguments[i]->resolvedType))
                        {
                            error("Argument " + std::to_string(i + 1) + " type mismatch in call to '" + funcName + "'.");
                        }
                    }
                }
            }
            node.resolvedType = signature->returnType;
        }
        else
        {
            node.resolvedType = std::make_shared<AnyType>();
        }
    }

    void SemanticAnalyzer::visit(IndexNode &node)
    {
        node.callee->accept(*this);
        node.index->accept(*this);

        std::shared_ptr<Type> left = node.callee->resolvedType;
        std::shared_ptr<Type> indexType = node.index->resolvedType;

        // 1. Resolve Boolean Flags
        bool isArray = std::dynamic_pointer_cast<ArrayType>(left) != nullptr;
        bool isTable = std::dynamic_pointer_cast<TableType>(left) != nullptr;
        bool isAny = std::dynamic_pointer_cast<AnyType>(left) != nullptr;
        bool isString = std::dynamic_pointer_cast<StringType>(left) != nullptr;
        bool isPointer = std::dynamic_pointer_cast<PointerType>(left) != nullptr;

        // 2. Fallback: If it looks like a class named "table", treat it as a Table
        if (!isTable && !isArray && !isString && !isAny)
        {
            if (auto cls = std::dynamic_pointer_cast<ClassInstanceType>(left))
            {
                if (cls->className == "table")
                {
                    isTable = true;
                }
            }
        }

        // 3. LOGIC: Handle Tables/Any (Allow String Keys) -> RETURN EARLY
        if (isTable || isAny)
        {
            node.resolvedType = std::make_shared<AnyType>();
            return;
        }

        // 4. Handle Arrays, Strings, and Raw Pointers
        if (isArray || isString || isPointer)
        {
            // Ensure the index is an integer (int, char, short, long)
            if (!isIntegralType(indexType) && !std::dynamic_pointer_cast<AnyType>(indexType))
            {
                error("Array or Pointer index must be an integer type.");
            }

            if (isArray)
            {
                node.resolvedType = std::static_pointer_cast<ArrayType>(left)->elementType;
            }
            else if (isPointer)
            {
                // This is what allows 'char* message; message[i]' to work
                node.resolvedType = std::static_pointer_cast<PointerType>(left)->pointeeType;
            }
            else
            {
                // It's a string, indexing returns a string (character) in Moksha
                node.resolvedType = std::make_shared<CharType>();
            }
            return;
        }

        // 5. Invalid Type
        error("Indexing operator '[]' used on non-array/non-table type.");
        node.resolvedType = std::make_shared<AnyType>();
    }

    void SemanticAnalyzer::visit(TemplateLiteralNode &node)
    {
        for (auto &part : node.parts)
            part->accept(*this);
        node.resolvedType = std::make_shared<StringType>();
    }

    void SemanticAnalyzer::visit(ArrayLiteralNode &node)
    {
        std::shared_ptr<Type> widestType = nullptr;

        for (auto &el : node.elements)
        {
            el->accept(*this);
            // --- Determine final element type (LCS) ---
            widestType = getLeastCommonSupertype(widestType, el->resolvedType);
        }

        if (!widestType)
            widestType = std::make_shared<AnyType>();

        node.resolvedType = std::make_shared<ArrayType>(widestType);
    }

    void SemanticAnalyzer::visit(TableLiteralNode &node)
    {
        for (auto &pair : node.entries)
        {
            pair.first->accept(*this);
            pair.second->accept(*this);
        }
        node.resolvedType = std::make_shared<TableType>();
    }

    void SemanticAnalyzer::visit(LengthExpressionNode &node)
    {
        node.target->accept(*this);
        node.resolvedType = std::make_shared<IntType>();
    }

    void SemanticAnalyzer::visit(LambdaNode &node)
    {
        int currentDepth = symbolTable.getCurrentDepth() + 1;
        lambdaStack.push({&node, currentDepth});
        symbolTable.enterScope();
        for (auto &param : node.parameters)
        {
            // Support explicitly typed lambdas
            if (param->type)
            {
                // Typed lambda parameter (int a)
                std::shared_ptr<Type> t = resolveTypeNode(*param->type);
                param->type->resolvedType = t;
                symbolTable.define(param->name.lexeme, param->type->baseTypeToken.lexeme, SymbolKind::PARAMETER, nullptr, nullptr, false, false, t);
            }
            else
            {
                // Untyped lambda parameter (x) -> inferred as Any
                symbolTable.define(param->name.lexeme, "any", SymbolKind::PARAMETER, nullptr, nullptr, false, false, std::make_shared<AnyType>());
            }
        }
        if (node.body)
            node.body->accept(*this);
        symbolTable.exitScope();
        lambdaStack.pop();
        node.resolvedType = std::make_shared<AnyType>(); // Lambdas themselves are "Any" for now, or you could create a specific FunctionType
    }

    void SemanticAnalyzer::visit(SpreadElementNode &node)
    {
        node.expression->accept(*this);

        if (typeid(*node.expression->resolvedType) != typeid(ArrayType))
        {
            error("'...' spread operator can only be used on array types.");
            node.resolvedType = std::make_shared<AnyType>();
            return;
        }
        node.resolvedType = node.expression->resolvedType;
    }

    // --- Visitor Implementation for Enum Definition ---
    void SemanticAnalyzer::visit(EnumDefinitionNode &node)
    {
        // 1. Define the Enum Type Name (e.g., "Color")
        // We treat it as a SymbolKind::ENUM so we can distinguish it later
        if (!symbolTable.define(node.name.lexeme, "enum_type", SymbolKind::ENUM))
        {
            error("Enum '" + node.name.lexeme + "' is already defined.");
        }

        std::map<std::string, std::shared_ptr<Type>> members;

        // 2. Register Members in the Static Map (NOT the symbol table)
        for (const auto &member : node.members)
        {
            if (member.second)
            {
                member.second->accept(*this);
                if (typeid(*member.second->resolvedType) != typeid(IntType))
                {
                    error("Enum member '" + member.first.lexeme + "' must have an integer value.");
                }
            }

            // All enum members are Integers
            members[member.first.lexeme] = std::make_shared<IntType>();
            if (!symbolTable.define(
                    member.first.lexeme,
                    "int",
                    SymbolKind::VARIABLE,
                    nullptr, nullptr,
                    true,                       // isConst = true
                    false,                      // isNullable = false
                    std::make_shared<IntType>() // resolvedType
                    ))
            {
                error("Enum member '" + member.first.lexeme + "' conflicts with an existing variable.");
            }
        }

        // Save to global storage so MemberAccessNode can find it later
        enumMembers[node.name.lexeme] = members;
    }

    // --- Visitor Implementation for Type Cast ---
    void SemanticAnalyzer::visit(TypeCastNode &node)
    {
        // 1. Resolve target type
        // (Assuming resolveTypeNode is your helper method to convert TypeNode -> Shared<Type>)
        std::shared_ptr<Type> target = resolveTypeNode(*node.targetType);

        // CRITICAL: Save the resolved type back to the AST node.
        // Without this, CodeGen sees a null 'resolvedType' and defaults to i8* (void ptr).
        node.targetType->resolvedType = target;

        // 2. Analyze the expression being cast
        node.expression->accept(*this);
        std::shared_ptr<Type> sourceType = node.expression->resolvedType;

        if (!sourceType || !target)
        {
            node.resolvedType = target;
            return;
        }

        // 3. Safety Check: Pointer-to-Integer Casts
        // These are dangerous because they expose memory addresses as numbers.

        bool isSourcePtr = (std::dynamic_pointer_cast<PointerType>(sourceType) != nullptr) ||
                           (std::dynamic_pointer_cast<ArrayType>(sourceType) != nullptr);

        bool isTargetInt = (std::dynamic_pointer_cast<IntType>(target) != nullptr) ||
                           (std::dynamic_pointer_cast<LongType>(target) != nullptr);

        // EXCEPTION: Allow String -> Int (Parsing)
        // Strings are technically pointers (char*) in LLVM, but logically they are Safe.
        bool isString = (std::dynamic_pointer_cast<StringType>(sourceType) != nullptr);

        if (isSourcePtr && isTargetInt)
        {
            // FIX: Allow 'Any' types to be cast to Int (Unboxing) without unsafe block
            bool isAny = (std::dynamic_pointer_cast<AnyType>(sourceType) != nullptr);

        }

        // 4. Finalize the node type
        node.resolvedType = target;
    }

    // --- Remaining Visitors (Structural) ---

    void SemanticAnalyzer::visit(ThrowStatementNode &node) { node.expression->accept(*this); }

    void SemanticAnalyzer::visit(TryStatementNode &node)
    {
        if (node.tryBlock)
            node.tryBlock->accept(*this);
        if (node.catchBlock)
        {
            symbolTable.enterScope();
            symbolTable.define(node.errorVariable.lexeme, "error", SymbolKind::VARIABLE);
            node.catchBlock->accept(*this);
            symbolTable.exitScope();
        }
        if (node.finallyBlock)
            node.finallyBlock->accept(*this);
    }

    void SemanticAnalyzer::visit(ExpressionStatementNode &node)
    {
        if (node.expression)
            node.expression->accept(*this);
    }
    void SemanticAnalyzer::visit(DeleteStatementNode &node) { node.target->accept(*this); }
    void SemanticAnalyzer::visit(UnsafeBlockNode &node)
    {
        bool previousState = insideUnsafeBlock; // Save old state
        insideUnsafeBlock = true;               // Enter unsafe mode

        if (node.body)
            node.body->accept(*this);

        insideUnsafeBlock = previousState; // Restore old state
    }
    void SemanticAnalyzer::visit(DeferStatementNode &node) { node.statement->accept(*this); }
    void SemanticAnalyzer::visit(WithStatementNode &node)
    {
        symbolTable.enterScope();
        if (node.declaration)
            node.declaration->accept(*this);
        if (node.body)
            node.body->accept(*this);
        symbolTable.exitScope();
    }
    void SemanticAnalyzer::visit(ImportNode &node) {}
    void SemanticAnalyzer::visit(MacroDefinitionNode &node)
    {
        // 1. Define Macro Name (Global)
        if (!symbolTable.define(node.name.lexeme, "macro", SymbolKind::FUNCTION))
        {
            error("Macro '" + node.name.lexeme + "' is already defined.");
        }

        // 2. Enter Scope for Body Analysis
        symbolTable.enterScope();

        // 3. Define Parameters as 'Any'
        for (const auto &param : node.parameters)
        {
            symbolTable.define(
                param.lexeme,
                "any",
                SymbolKind::PARAMETER,
                nullptr, nullptr,
                false, false,
                std::make_shared<AnyType>() // Default to Any
            );
        }

        // 4. Analyze Body
        if (node.body)
            node.body->accept(*this);

        symbolTable.exitScope();
    }
    void SemanticAnalyzer::visit(PrintStatementNode &node)
    {
        if (node.expression)
            node.expression->accept(*this);
    }
    void SemanticAnalyzer::visit(CaseNode &node)
    {
        if (node.value)
            node.value->accept(*this);
        if (node.rangeEnd)
            node.rangeEnd->accept(*this);
        for (auto &stmt : node.statements)
            stmt->accept(*this);
    }
    void SemanticAnalyzer::visit(SwitchStatementNode &node)
    {
        node.condition->accept(*this);
        for (auto &c : node.cases)
            c->accept(*this);
        if (node.defaultCase)
            node.defaultCase->accept(*this);
    }
    void SemanticAnalyzer::visit(TypeNode &node)
    {
        // 1. Resolve Base Type
        if (node.baseTypeToken.type == TokenType::KW_INT)
            node.resolvedType = std::make_shared<IntType>();
        else if (node.baseTypeToken.type == TokenType::KW_DOUBLE)
            node.resolvedType = std::make_shared<DoubleType>();
        else if (node.baseTypeToken.type == TokenType::KW_FLOAT)
            node.resolvedType = std::make_shared<FloatType>();
        else if (node.baseTypeToken.type == TokenType::KW_STRING)
            node.resolvedType = std::make_shared<StringType>();
        else if (node.baseTypeToken.type == TokenType::KW_BOOLEAN)
            node.resolvedType = std::make_shared<BooleanType>();
        else if (node.baseTypeToken.type == TokenType::KW_VOID)
            node.resolvedType = std::make_shared<VoidType>();
        else if (node.baseTypeToken.type == TokenType::KW_ANY)
            node.resolvedType = std::make_shared<AnyType>();
        else if (node.baseTypeToken.type == TokenType::KW_TABLE)
            node.resolvedType = std::make_shared<TableType>();
        else if (node.baseTypeToken.type == TokenType::KW_CHAR)
            node.resolvedType = std::make_shared<CharType>();
        else if (node.baseTypeToken.type == TokenType::KW_SHORT)
            node.resolvedType = std::make_shared<ShortType>();
        else if (node.baseTypeToken.type == TokenType::KW_LONG)
            node.resolvedType = std::make_shared<LongType>();
        else if (node.baseTypeToken.type == TokenType::IDENTIFIER)
        {
            // --- UPDATED LOGIC ---
            std::string typeName = node.baseTypeToken.lexeme;

            auto alias = symbolTable.resolveType(typeName);
            if (alias)
            {
                node.resolvedType = alias;
                return;
            }

            SymbolInfo *sym = symbolTable.resolve(typeName);

            if (typeName == "float")
            {
                node.resolvedType = std::make_shared<FloatType>();
                return;
            }

            if (sym)
            {
                if (sym->kind == SymbolKind::CLASS)
                {
                    node.resolvedType = std::make_shared<ClassInstanceType>(typeName);
                }
                else if (sym->kind == SymbolKind::STRUCT)
                {
                    node.resolvedType = std::make_shared<StructType>(typeName);
                }
                else if (sym->kind == SymbolKind::ENUM)
                {
                    // (Existing logic for enums)
                    node.resolvedType = std::make_shared<IntType>();
                }
                else
                {
                    error("Symbol '" + typeName + "' is not a type.");
                    node.resolvedType = std::make_shared<AnyType>();
                }
            }
            else
            {
                error("Unknown type '" + typeName + "'.");
                node.resolvedType = std::make_shared<AnyType>();
            }
        }

        // 2. Handle Pointers and Arrays wrapping
        if (node.resolvedType)
        {
            // Wrap in PointerType
            for (int i = 0; i < node.pointerDepth; i++)
            {
                node.resolvedType = std::make_shared<PointerType>(node.resolvedType);
            }

            for (const auto &dim : node.dimensions)
            {
                auto arr = std::make_shared<ArrayType>(node.resolvedType);
                if (auto lit = dynamic_cast<LiteralNode *>(dim.get()))
                {
                    if (lit->token.type == TokenType::INTEGER_LITERAL)
                    {
                        arr->isFixedSize = true;
                        arr->fixedSize = std::stoi(lit->token.lexeme, nullptr, 0);
                    }
                }
                node.resolvedType = arr;
            }
            node.resolvedType->isNullable = node.isNullable;
        }
    }

    void SemanticAnalyzer::visit(TypedefNode &node)
    {
        // 1. Resolve the source type
        node.sourceType->accept(*this);

        // 2. Store the alias in the Symbol Table
        symbolTable.defineType(node.aliasName.lexeme, node.sourceType->resolvedType);
    }

    void SemanticAnalyzer::visit(SizeOfNode &node)
    {
        if (node.targetType)
        {
            node.targetType->accept(*this);
        }
        else if (node.expression)
        {
            node.expression->accept(*this);
        }
        // The result of 'sizeof' is always an integer (size_t)
        node.resolvedType = std::make_shared<IntType>();
    }

    void SemanticAnalyzer::visit(StaticIfNode &node)
    {
        // 1. Analyze condition (must be resolvable now)
        node.condition->accept(*this);

        // 2. Evaluate
        bool condition = evaluateConstBool(node.condition.get());

        // 3. ONLY visit the active branch
        // This prevents the compiler from analyzing/generating code for the dead branch.
        if (condition)
        {
            node.thenBranch->accept(*this);
        }
        else if (node.elseBranch)
        {
            node.elseBranch->accept(*this);
        }
    }

    void SemanticAnalyzer::visit(ParameterNode &node) {}

} // namespace Moksha