#pragma once

#include "moksha/token.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace Moksha
{
    struct Type
    {
        bool isNullable = false;
        virtual ~Type() = default;
    };

    struct PointerType : public Type
    {
        std::shared_ptr<Type> pointeeType;
        PointerType(std::shared_ptr<Type> pointee) : pointeeType(std::move(pointee)) {}
    };

    // Primitive Types (For easy comparison)
    struct IntType : public Type
    {
    };
    struct DoubleType : public Type
    {
    };
    struct StringType : public Type
    {
    };
    struct BooleanType : public Type
    {
    };
    struct VoidType : public Type
    {
    };
    struct AnyType : public Type
    {
    };
    struct NullType : public Type
    {
    };
    struct CharType : public Type
    {
    };
    struct ShortType : public Type
    {
    };
    struct LongType : public Type
    {
    };
    struct FloatType : public Type
    {
    };

    // Composite/Complex Types
    struct ArrayType : public Type
    {
        std::shared_ptr<Type> elementType;
        bool isFixedSize = false; // Add this
        int fixedSize = 0;        // Add this
        ArrayType(std::shared_ptr<Type> element) : elementType(std::move(element)) {}
    };

    struct TableType : public Type
    {
    };

    struct ClassInstanceType : public Type
    {
        std::string className;
        ClassInstanceType(std::string name) : className(std::move(name)) {}
        ClassInstanceType() = default;
    };

    struct StructType : public Type
    {
        std::string structName;
        StructType(std::string name) : structName(std::move(name)) {}
    };

    struct FunctionType : public Type
    {
        std::vector<std::shared_ptr<Type>> parameterTypes;
        std::shared_ptr<Type> returnType;
        bool isAsync = false;
        bool isVariadic = false;
        int minArgCount = 0;
        FunctionType(std::vector<std::shared_ptr<Type>> params,
                     std::shared_ptr<Type> retType,
                     bool async,
                     bool variadic = false,
                     int minArgs = 0)
            : parameterTypes(std::move(params)),
              returnType(std::move(retType)),
              isAsync(async), isVariadic(variadic),
              minArgCount(minArgs) {}
    };

    class ASTVisitor;

    // Access Specifiers
    enum class AccessLevel
    {
        PUBLIC,
        PRIVATE,
        PROTECTED,
        DEFAULT // For top-level scripts or local vars
    };

    // --- Base Nodes ---
    class ASTNode
    {
    public:
        virtual ~ASTNode() = default;
        virtual void accept(ASTVisitor &visitor) = 0;
    };
    class StatementNode : public ASTNode
    {
    };
    class ExpressionNode : public ASTNode
    {
    public:
        std::shared_ptr<Type> resolvedType; // The result of semantic analysis
    };

    struct CaptureInfo
    {
        std::string name;
        std::shared_ptr<Type> type;
        bool isCaptured = false; // Helper for CodeGen to avoid duplicate processing
        bool isByRef = false;
    };

    // --- Type Node (Fixed) ---
    class TypeNode : public ASTNode
    {
    public:
        Token baseTypeToken;
        std::unique_ptr<TypeNode> elementType;
        std::vector<std::unique_ptr<ExpressionNode>> dimensions;
        int pointerDepth;
        bool isNullable;
        bool isVolatile;

        std::shared_ptr<Type> resolvedType;

        // Constructor 1: Basic Types (int, int*, User?)
        TypeNode(Token base, int depth = 0, bool isPtr = false, bool isNull = false, bool isVol = false)
            : baseTypeToken(std::move(base)), pointerDepth(depth), isNullable(isNull), isVolatile(isVol) {}

        // Constructor 2: Array Types (int[])
        TypeNode(Token base, std::unique_ptr<TypeNode> elemType, std::vector<std::unique_ptr<ExpressionNode>> dims)
            : baseTypeToken(std::move(base)),
              elementType(std::move(elemType)),
              dimensions(std::move(dims)),
              pointerDepth(0),
              isNullable(false) {}

        void accept(ASTVisitor &visitor) override;
    };

    // --- Expression Nodes ---
    class LiteralNode : public ExpressionNode
    {
    public:
        Token token;
        LiteralNode(Token token) : token(std::move(token)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class VariableNode : public ExpressionNode
    {
    public:
        Token name;
        VariableNode(Token name) : name(std::move(name)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ThisExpression : public ExpressionNode
    {
    public:
        Token keyword;
        ThisExpression(Token keyword) : keyword(std::move(keyword)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class NewExpression : public ExpressionNode
    {
    public:
        Token className;
        std::vector<std::unique_ptr<ExpressionNode>> arguments;
        NewExpression(Token className, std::vector<std::unique_ptr<ExpressionNode>> args)
            : className(std::move(className)), arguments(std::move(args)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class InputExpression : public ExpressionNode
    {
    public:
        Token token;
        InputExpression(Token token) : token(std::move(token)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class UnaryOpNode : public ExpressionNode
    {
    public:
        Token op;
        std::unique_ptr<ExpressionNode> right;
        UnaryOpNode(Token op, std::unique_ptr<ExpressionNode> right) : op(std::move(op)), right(std::move(right)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class UpdateNode : public ExpressionNode
    {
    public:
        Token op;
        std::unique_ptr<ExpressionNode> argument;
        bool isPrefix;
        UpdateNode(Token op, std::unique_ptr<ExpressionNode> arg, bool isPrefix)
            : op(op), argument(std::move(arg)), isPrefix(isPrefix) {}
        void accept(ASTVisitor &visitor) override;
    };
    class BinaryOpNode : public ExpressionNode
    {
    public:
        std::unique_ptr<ExpressionNode> left;
        Token op;
        std::unique_ptr<ExpressionNode> right;
        BinaryOpNode(std::unique_ptr<ExpressionNode> left, Token op, std::unique_ptr<ExpressionNode> right)
            : left(std::move(left)), op(std::move(op)), right(std::move(right)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class TernaryOpNode : public ExpressionNode
    {
    public:
        std::unique_ptr<ExpressionNode> condition;
        std::unique_ptr<ExpressionNode> thenBranch;
        std::unique_ptr<ExpressionNode> elseBranch;
        TernaryOpNode(std::unique_ptr<ExpressionNode> c, std::unique_ptr<ExpressionNode> t, std::unique_ptr<ExpressionNode> e)
            : condition(std::move(c)), thenBranch(std::move(t)), elseBranch(std::move(e)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class GroupingNode : public ExpressionNode
    {
    public:
        std::unique_ptr<ExpressionNode> expression;
        GroupingNode(std::unique_ptr<ExpressionNode> expr) : expression(std::move(expr)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class CallNode : public ExpressionNode
    {
    public:
        std::unique_ptr<ExpressionNode> callee;
        std::vector<std::unique_ptr<ExpressionNode>> arguments;
        bool isOptional;
        CallNode(std::unique_ptr<ExpressionNode> callee, std::vector<std::unique_ptr<ExpressionNode>> args, bool isOptional = false)
            : callee(std::move(callee)), arguments(std::move(args)), isOptional(isOptional) {}
        void accept(ASTVisitor &visitor) override;
    };
    class MemberAccessNode : public ExpressionNode
    {
    public:
        std::unique_ptr<ExpressionNode> object;
        Token name;
        bool isOptional;
        MemberAccessNode(std::unique_ptr<ExpressionNode> obj, Token name, bool isOptional = false)
            : object(std::move(obj)), name(std::move(name)), isOptional(isOptional) {}
        void accept(ASTVisitor &visitor) override;
    };
    class IndexNode : public ExpressionNode
    {
    public:
        std::unique_ptr<ExpressionNode> callee;
        std::unique_ptr<ExpressionNode> index;
        bool isOptional;
        IndexNode(std::unique_ptr<ExpressionNode> callee, std::unique_ptr<ExpressionNode> index, bool isOptional = false)
            : callee(std::move(callee)), index(std::move(index)), isOptional(isOptional) {}
        void accept(ASTVisitor &visitor) override;
    };
    class TemplateLiteralNode : public ExpressionNode
    {
    public:
        std::vector<std::unique_ptr<ExpressionNode>> parts;
        TemplateLiteralNode() = default;
        void accept(ASTVisitor &visitor) override;
    };
    class ArrayLiteralNode : public ExpressionNode
    {
    public:
        std::vector<std::unique_ptr<ExpressionNode>> elements;
        ArrayLiteralNode(std::vector<std::unique_ptr<ExpressionNode>> elems)
            : elements(std::move(elems)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class TableLiteralNode : public ExpressionNode
    {
    public:
        std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<ExpressionNode>>> entries;
        TableLiteralNode(std::vector<std::pair<std::unique_ptr<ExpressionNode>, std::unique_ptr<ExpressionNode>>> ent)
            : entries(std::move(ent)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class DeleteStatementNode : public ExpressionNode
    {
    public:
        std::unique_ptr<ExpressionNode> target;
        DeleteStatementNode(std::unique_ptr<ExpressionNode> tgt) : target(std::move(tgt)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class LengthExpressionNode : public ExpressionNode
    {
    public:
        std::unique_ptr<ExpressionNode> target;
        LengthExpressionNode(std::unique_ptr<ExpressionNode> tgt) : target(std::move(tgt)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class SpreadElementNode : public ExpressionNode
    {
    public:
        std::unique_ptr<ExpressionNode> expression;
        SpreadElementNode(std::unique_ptr<ExpressionNode> expr) : expression(std::move(expr)) {}
        void accept(ASTVisitor &visitor) override;
    };
    // [NEW] SizeOfNode
    class SizeOfNode : public ExpressionNode
    {
    public:
        std::unique_ptr<TypeNode> targetType;       // sizeof(int)
        std::unique_ptr<ExpressionNode> expression; // sizeof(var)

        SizeOfNode(std::unique_ptr<TypeNode> type) : targetType(std::move(type)) {}
        SizeOfNode(std::unique_ptr<ExpressionNode> expr) : expression(std::move(expr)) {}

        void accept(ASTVisitor &visitor) override;
    };
    // [NEW] StaticIfNode
    class StaticIfNode : public StatementNode
    {
    public:
        std::unique_ptr<ExpressionNode> condition;
        std::unique_ptr<StatementNode> thenBranch;
        std::unique_ptr<StatementNode> elseBranch;

        StaticIfNode(std::unique_ptr<ExpressionNode> cond,
                     std::unique_ptr<StatementNode> thenB,
                     std::unique_ptr<StatementNode> elseB)
            : condition(std::move(cond)), thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}

        void accept(ASTVisitor &visitor) override;
    };
    // [NEW] TypedefNode (Optional, mostly for semantic analysis)
    class TypedefNode : public StatementNode
    {
    public:
        std::unique_ptr<TypeNode> sourceType;
        Token aliasName;
        TypedefNode(std::unique_ptr<TypeNode> src, Token alias)
            : sourceType(std::move(src)), aliasName(std::move(alias)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class BlockStatementNode : public StatementNode
    {
    public:
        std::vector<std::unique_ptr<StatementNode>> statements;
        void accept(ASTVisitor &visitor) override;
    };
    class UnsafeBlockNode : public StatementNode
    {
    public:
        std::unique_ptr<BlockStatementNode> body;
        UnsafeBlockNode(std::unique_ptr<BlockStatementNode> b) : body(std::move(b)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ParameterNode : public ASTNode
    {
    public:
        std::unique_ptr<TypeNode> type;
        Token name;
        std::unique_ptr<ExpressionNode> defaultValue;
        bool isRef;
        ParameterNode(std::unique_ptr<TypeNode> type, Token name, std::unique_ptr<ExpressionNode> defaultVal = nullptr, bool isRef = false)
            : type(std::move(type)), name(std::move(name)), defaultValue(std::move(defaultVal)), isRef(isRef) {}
        void accept(ASTVisitor &visitor) override;
    };
    class LambdaNode : public ExpressionNode
    {
    public:
        std::vector<std::unique_ptr<ParameterNode>> parameters;
        std::unique_ptr<StatementNode> body;
        std::vector<CaptureInfo> captures; // List of variables this lambda captures from outer scope
        LambdaNode(std::vector<std::unique_ptr<ParameterNode>> params, std::unique_ptr<StatementNode> body)
            : parameters(std::move(params)), body(std::move(body)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class VariableDeclarationNode : public StatementNode
    {
    public:
        std::unique_ptr<TypeNode> type;
        Token name;
        std::unique_ptr<ExpressionNode> initializer;
        bool isConst;
        AccessLevel visibility;
        bool isShared;
        bool isVolatile;
        bool isExtern;
        int alignment;
        int bitWidth;
        std::string section;
        mutable bool isHeapAllocated = false;
        VariableDeclarationNode(std::unique_ptr<TypeNode> type, Token name, std::unique_ptr<ExpressionNode> init,
                                bool isConst = false, AccessLevel vis = AccessLevel::DEFAULT, bool isShared = false,
                                bool isVol = false, bool isExt = false, int align = 0, std::string sect = "", int bW = 0)
            : type(std::move(type)), name(std::move(name)), initializer(std::move(init)),
              isConst(isConst), visibility(vis), isShared(isShared), bitWidth(bW),
              isVolatile(isVol), isExtern(isExt), alignment(align), section(std::move(sect)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class FunctionDefinitionNode : public StatementNode
    {
    public:
        std::unique_ptr<TypeNode> returnType;
        Token name;
        std::vector<std::unique_ptr<ParameterNode>> parameters;
        std::unique_ptr<BlockStatementNode> body;
        AccessLevel visibility;
        bool isAsync;
        bool isOperator;
        bool isExtern;
        bool isInterrupt;
        bool isVariadic;
        std::string section;
        FunctionDefinitionNode(std::unique_ptr<TypeNode> retType, Token name,
                               std::vector<std::unique_ptr<ParameterNode>> params,
                               std::unique_ptr<BlockStatementNode> body,
                               AccessLevel vis = AccessLevel::DEFAULT,
                               bool isAsync = false,
                               bool isOperator = false,
                               bool isExt = false,
                               bool isIntr = false, bool isVar = false,
                               std::string sect = "")
            : returnType(std::move(retType)), name(std::move(name)),
              parameters(std::move(params)), body(std::move(body)),
              visibility(vis), isAsync(isAsync), isOperator(isOperator),
              isExtern(isExt), section(std::move(sect)), isInterrupt(isIntr), isVariadic(isVar) {}
        void accept(ASTVisitor &visitor) override;
    };
    class MacroDefinitionNode : public StatementNode
    {
    public:
        Token name;
        std::vector<Token> parameters;
        std::unique_ptr<BlockStatementNode> body;
        MacroDefinitionNode(Token name, std::vector<Token> params, std::unique_ptr<BlockStatementNode> body)
            : name(std::move(name)), parameters(std::move(params)), body(std::move(body)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class PrintStatementNode : public StatementNode
    {
    public:
        Token variant;
        std::unique_ptr<ExpressionNode> expression;
        PrintStatementNode(Token variant, std::unique_ptr<ExpressionNode> expr)
            : variant(std::move(variant)), expression(std::move(expr)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ClassDefinitionNode : public StatementNode
    {
    public:
        Token name;
        std::vector<std::unique_ptr<VariableNode>> parentClassNames;
        std::vector<std::unique_ptr<StatementNode>> members;
        bool isRef;
        ClassDefinitionNode(Token name, std::vector<std::unique_ptr<VariableNode>> parents,
                            std::vector<std::unique_ptr<StatementNode>> members,
                            bool isRef = false)
            : name(std::move(name)), parentClassNames(std::move(parents)),
              members(std::move(members)), isRef(isRef) {}
        void accept(ASTVisitor &visitor) override;
    };
    class StructDefinitionNode : public StatementNode
    {
    public:
        Token name;
        std::vector<std::unique_ptr<StatementNode>> members;
        bool isPacked;
        bool isUnion;
        StructDefinitionNode(Token name, std::vector<std::unique_ptr<StatementNode>> members,
                             bool isPacked = false, bool isUnion = false)
            : name(std::move(name)), members(std::move(members)),
              isPacked(isPacked), isUnion(isUnion) {}

        void accept(ASTVisitor &visitor) override;
    };
    class AsmExpressionNode : public ExpressionNode
    {
    public:
        std::unique_ptr<TypeNode> returnType; // Can be nullptr (void)
        std::string assemblyTemplate;
        std::string constraints;
        std::vector<std::unique_ptr<ExpressionNode>> arguments;
        bool hasSideEffects;

        AsmExpressionNode(std::unique_ptr<TypeNode> retType,
                          std::string asmStr,
                          std::string constr,
                          std::vector<std::unique_ptr<ExpressionNode>> args,
                          bool sideEffects = true)
            : returnType(std::move(retType)),
              assemblyTemplate(std::move(asmStr)),
              constraints(std::move(constr)),
              arguments(std::move(args)),
              hasSideEffects(sideEffects) {}

        void accept(ASTVisitor &visitor) override;
    };
    enum class CaseType
    {
        VALUE,
        RANGE
    };
    class CaseNode : public StatementNode
    {
    public:
        CaseType type;
        std::unique_ptr<ExpressionNode> value;
        std::unique_ptr<ExpressionNode> rangeEnd;
        std::vector<std::unique_ptr<StatementNode>> statements;
        CaseNode(std::unique_ptr<ExpressionNode> val, std::vector<std::unique_ptr<StatementNode>> stmts)
            : type(CaseType::VALUE), value(std::move(val)), statements(std::move(stmts)) {}
        CaseNode(std::unique_ptr<ExpressionNode> start, std::unique_ptr<ExpressionNode> end, std::vector<std::unique_ptr<StatementNode>> stmts)
            : type(CaseType::RANGE), value(std::move(start)), rangeEnd(std::move(end)), statements(std::move(stmts)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class SwitchStatementNode : public StatementNode
    {
    public:
        std::unique_ptr<ExpressionNode> condition;
        std::vector<std::unique_ptr<CaseNode>> cases;
        std::unique_ptr<CaseNode> defaultCase;
        SwitchStatementNode(std::unique_ptr<ExpressionNode> cond, std::vector<std::unique_ptr<CaseNode>> cases, std::unique_ptr<CaseNode> defCase)
            : condition(std::move(cond)), cases(std::move(cases)), defaultCase(std::move(defCase)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class IfStatementNode : public StatementNode
    {
    public:
        std::unique_ptr<ExpressionNode> condition;
        std::unique_ptr<StatementNode> thenBranch;
        std::unique_ptr<StatementNode> elseBranch;
        IfStatementNode(std::unique_ptr<ExpressionNode> cond,
                        std::unique_ptr<StatementNode> then,
                        std::unique_ptr<StatementNode> elseB = nullptr)
            : condition(std::move(cond)), thenBranch(std::move(then)), elseBranch(std::move(elseB)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class WhileNode : public StatementNode
    {
    public:
        std::unique_ptr<ExpressionNode> condition;
        std::unique_ptr<StatementNode> body;
        WhileNode(std::unique_ptr<ExpressionNode> cond, std::unique_ptr<StatementNode> body)
            : condition(std::move(cond)), body(std::move(body)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class DoWhileNode : public StatementNode
    {
    public:
        std::unique_ptr<StatementNode> body;
        std::unique_ptr<ExpressionNode> condition;
        DoWhileNode(std::unique_ptr<StatementNode> body, std::unique_ptr<ExpressionNode> cond)
            : body(std::move(body)), condition(std::move(cond)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ForNode : public StatementNode
    {
    public:
        std::unique_ptr<StatementNode> initializer;
        std::unique_ptr<ExpressionNode> condition;
        std::unique_ptr<ExpressionNode> increment;
        std::unique_ptr<StatementNode> body;
        ForNode(std::unique_ptr<StatementNode> init, std::unique_ptr<ExpressionNode> cond,
                std::unique_ptr<ExpressionNode> incr, std::unique_ptr<StatementNode> body)
            : initializer(std::move(init)), condition(std::move(cond)),
              increment(std::move(incr)), body(std::move(body)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ForInNode : public StatementNode
    {
    public:
        Token keyVar;
        Token valVar;
        std::unique_ptr<ExpressionNode> iterable;
        std::unique_ptr<StatementNode> body;
        bool hasValueVar;
        ForInNode(Token key, Token val, std::unique_ptr<ExpressionNode> iter, std::unique_ptr<StatementNode> body, bool hasVal)
            : keyVar(std::move(key)), valVar(std::move(val)), iterable(std::move(iter)), body(std::move(body)), hasValueVar(hasVal) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ReturnNode : public StatementNode
    {
    public:
        Token keyword;
        std::unique_ptr<ExpressionNode> value;
        ReturnNode(Token keyword, std::unique_ptr<ExpressionNode> val = nullptr)
            : keyword(std::move(keyword)), value(std::move(val)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ImportNode : public StatementNode
    {
    public:
        Token moduleName;
        std::vector<Token> importedSymbols;
        bool isFromImport;
        ImportNode(Token modName, std::vector<Token> symbols, bool isFrom)
            : moduleName(std::move(modName)), importedSymbols(std::move(symbols)), isFromImport(isFrom) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ExpressionStatementNode : public StatementNode
    {
    public:
        std::unique_ptr<ExpressionNode> expression;
        ExpressionStatementNode(std::unique_ptr<ExpressionNode> expr)
            : expression(std::move(expr)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class BreakStatementNode : public StatementNode
    {
    public:
        Token keyword;
        BreakStatementNode(Token keyword) : keyword(std::move(keyword)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ContinueStatementNode : public StatementNode
    {
    public:
        Token keyword;
        ContinueStatementNode(Token keyword) : keyword(std::move(keyword)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class ThrowStatementNode : public StatementNode
    {
    public:
        std::unique_ptr<ExpressionNode> expression;
        ThrowStatementNode(std::unique_ptr<ExpressionNode> expr) : expression(std::move(expr)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class TryStatementNode : public StatementNode
    {
    public:
        std::unique_ptr<BlockStatementNode> tryBlock;
        std::unique_ptr<BlockStatementNode> catchBlock;
        Token errorVariable;
        std::unique_ptr<BlockStatementNode> finallyBlock;
        TryStatementNode(std::unique_ptr<BlockStatementNode> t,
                         std::unique_ptr<BlockStatementNode> c,
                         Token errVar,
                         std::unique_ptr<BlockStatementNode> f)
            : tryBlock(std::move(t)), catchBlock(std::move(c)), errorVariable(std::move(errVar)), finallyBlock(std::move(f)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class DeferStatementNode : public StatementNode
    {
    public:
        std::unique_ptr<StatementNode> statement;
        DeferStatementNode(std::unique_ptr<StatementNode> stmt) : statement(std::move(stmt)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class WithStatementNode : public StatementNode
    {
    public:
        std::unique_ptr<StatementNode> declaration;
        std::unique_ptr<StatementNode> body;
        WithStatementNode(std::unique_ptr<StatementNode> decl, std::unique_ptr<StatementNode> b)
            : declaration(std::move(decl)), body(std::move(b)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class EnumDefinitionNode : public StatementNode
    {
    public:
        Token name;
        std::vector<std::pair<Token, std::unique_ptr<ExpressionNode>>> members; // Map of Enum Member Name -> Optional Initializer Expression eg :- { "Red", nullptr }, { "Green", 10 }
        EnumDefinitionNode(Token name, std::vector<std::pair<Token, std::unique_ptr<ExpressionNode>>> members)
            : name(std::move(name)), members(std::move(members)) {}
        void accept(ASTVisitor &visitor) override;
    };
    class TypeCastNode : public ExpressionNode
    {
    public:
        std::unique_ptr<TypeNode> targetType;
        std::unique_ptr<ExpressionNode> expression;

        TypeCastNode(std::unique_ptr<TypeNode> type, std::unique_ptr<ExpressionNode> expr)
            : targetType(std::move(type)), expression(std::move(expr)) {}

        void accept(ASTVisitor &visitor) override;
    };

    class ASTVisitor
    {
    public:
        virtual ~ASTVisitor() = default;
        virtual void visit(TypeNode &node) = 0;
        virtual void visit(ParameterNode &node) = 0;
        virtual void visit(BlockStatementNode &node) = 0;
        virtual void visit(VariableDeclarationNode &node) = 0;
        virtual void visit(FunctionDefinitionNode &node) = 0;
        virtual void visit(ClassDefinitionNode &node) = 0;
        virtual void visit(StructDefinitionNode &node) = 0;
        virtual void visit(LiteralNode &node) = 0;
        virtual void visit(VariableNode &node) = 0;
        virtual void visit(ThisExpression &node) = 0;
        virtual void visit(NewExpression &node) = 0;
        virtual void visit(InputExpression &node) = 0;
        virtual void visit(UnaryOpNode &node) = 0;
        virtual void visit(UpdateNode &node) = 0;
        virtual void visit(BinaryOpNode &node) = 0;
        virtual void visit(TernaryOpNode &node) = 0;
        virtual void visit(GroupingNode &node) = 0;
        virtual void visit(CallNode &node) = 0;
        virtual void visit(MemberAccessNode &node) = 0;
        virtual void visit(IndexNode &node) = 0;
        virtual void visit(TemplateLiteralNode &node) = 0;
        virtual void visit(ArrayLiteralNode &node) = 0;
        virtual void visit(CaseNode &node) = 0;
        virtual void visit(SwitchStatementNode &node) = 0;
        virtual void visit(IfStatementNode &node) = 0;
        virtual void visit(WhileNode &node) = 0;
        virtual void visit(DoWhileNode &node) = 0;
        virtual void visit(ForNode &node) = 0;
        virtual void visit(ForInNode &node) = 0;
        virtual void visit(ReturnNode &node) = 0;
        virtual void visit(ImportNode &node) = 0;
        virtual void visit(ExpressionStatementNode &node) = 0;
        virtual void visit(MacroDefinitionNode &node) = 0;
        virtual void visit(PrintStatementNode &node) = 0;
        virtual void visit(BreakStatementNode &node) = 0;
        virtual void visit(ContinueStatementNode &node) = 0;
        virtual void visit(TableLiteralNode &node) = 0;
        virtual void visit(DeleteStatementNode &node) = 0;
        virtual void visit(LengthExpressionNode &node) = 0;
        virtual void visit(LambdaNode &node) = 0;
        virtual void visit(SpreadElementNode &node) = 0;
        virtual void visit(UnsafeBlockNode &node) = 0;
        virtual void visit(ThrowStatementNode &node) = 0;
        virtual void visit(TryStatementNode &node) = 0;
        virtual void visit(DeferStatementNode &node) = 0;
        virtual void visit(WithStatementNode &node) = 0;
        virtual void visit(EnumDefinitionNode &node) = 0;
        virtual void visit(TypeCastNode &node) = 0;
        virtual void visit(AsmExpressionNode &node) = 0;
        virtual void visit(SizeOfNode &node) = 0;
        virtual void visit(StaticIfNode &node) = 0;
        virtual void visit(TypedefNode &node) = 0;
    };

    inline void TypeNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ParameterNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void BlockStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void VariableDeclarationNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void FunctionDefinitionNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ClassDefinitionNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void StructDefinitionNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void LiteralNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void VariableNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ThisExpression::accept(ASTVisitor &v) { v.visit(*this); }
    inline void NewExpression::accept(ASTVisitor &v) { v.visit(*this); }
    inline void InputExpression::accept(ASTVisitor &v) { v.visit(*this); }
    inline void UnaryOpNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void UpdateNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void BinaryOpNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void TernaryOpNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void GroupingNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void CallNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void MemberAccessNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void IndexNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void TemplateLiteralNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ArrayLiteralNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void CaseNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void SwitchStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void IfStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void WhileNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void DoWhileNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ForNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ForInNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ReturnNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ImportNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ExpressionStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void MacroDefinitionNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void PrintStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void BreakStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ContinueStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void TableLiteralNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void DeleteStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void LengthExpressionNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void LambdaNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void SpreadElementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void UnsafeBlockNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void ThrowStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void TryStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void DeferStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void WithStatementNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void EnumDefinitionNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void TypeCastNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void AsmExpressionNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void SizeOfNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void StaticIfNode::accept(ASTVisitor &v) { v.visit(*this); }
    inline void TypedefNode::accept(ASTVisitor &v) { v.visit(*this); }

} // namespace Moksha