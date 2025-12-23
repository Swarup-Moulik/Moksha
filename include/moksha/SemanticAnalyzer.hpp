#pragma once

#include "moksha/ast.hpp"
#include "moksha/SymbolTable.hpp"
#include <vector>
#include <string>
#include <iostream>
#include <memory>
#include <map>
#include <set>

namespace Moksha {

class SemanticAnalyzer : public ASTVisitor {  
public:
    struct MemberMeta {
        std::shared_ptr<Type> type;
        AccessLevel access;
    };
    struct VTableEntry {
        std::string methodName;     
        std::string implName;        
        std::shared_ptr<FunctionType> signature;
    };

private:
    SymbolTable symbolTable;
    bool hasError = false;
    static std::map<std::string, std::map<std::string, MemberMeta>> classMembers;
    static std::map<std::string, std::vector<std::string>> classParents;

    // Maps ClassName -> List of Methods in order
    static std::map<std::string, std::vector<VTableEntry>> classVTables;
    // Maps ClassName -> MethodName -> Index in VTable
    static std::map<std::string, std::map<std::string, int>> classVTableIndices;

    // --- State for Robust Checks ---
    int inLoopDepth = 0; 
    std::shared_ptr<Type> currentFnReturnType = std::make_shared<VoidType>(); 
    std::string currentClassName = ""; 
    
    // Global Registry (Static to persist across multiple files if needed)
    static std::map<std::string, std::map<std::string, std::shared_ptr<Type>>> classFieldTypes; 
    static std::map<std::string, std::set<std::string>> classMethodNames;
    static std::map<std::string, std::map<std::string, std::shared_ptr<Type>>> enumMembers;
    std::map<std::string, ClassDefinitionNode*> classMap;

    // Stores resolved FunctionType signatures
    static std::map<std::string, std::shared_ptr<FunctionType>> functionSignatures;
    static std::map<std::string, std::map<size_t, std::shared_ptr<FunctionType>>> constructorSignatures;

    void error(const std::string& message);
    
    // --- Type Helpers ---
    std::shared_ptr<Type> resolveTypeNode(const TypeNode& node);
    bool isTypeCompatible(std::shared_ptr<Type> expected, std::shared_ptr<Type> actual);
    bool isIntegralType(std::shared_ptr<Type> type);
    bool isSubclass(const std::string& child, const std::string& parent);
    std::shared_ptr<Type> getTypeFromToken(const Token& token);
    void checkConditionalType(ExpressionNode* cond);

public:
    SemanticAnalyzer() = default;
    bool insideUnsafeBlock = false;
    bool analyze(const std::vector<std::unique_ptr<StatementNode>>& statements);
    bool evaluateConstBool(ExpressionNode* node);
    static std::map<std::string, std::map<std::string, std::shared_ptr<Type>>> structFieldTypes;

    // Public getter for CodeGen to access VTables
    static const std::vector<VTableEntry>& getVTable(const std::string& className) {
        return classVTables[className];
    }
    static int getVTableIndex(const std::string& className, const std::string& methodName) {
        if (classVTableIndices[className].count(methodName))
            return classVTableIndices[className].at(methodName);
        return -1;
    }

    // --- Visitor Implementation Overrides ---
    void visit(BlockStatementNode& node) override;
    void visit(VariableDeclarationNode& node) override;
    void visit(VariableNode& node) override;
    void visit(BinaryOpNode& node) override;
    void visit(FunctionDefinitionNode& node) override;
    void visit(ClassDefinitionNode& node) override;
    void visit(StructDefinitionNode& node) override;
    void visit(ReturnNode& node) override;
    void visit(IfStatementNode& node) override;
    void visit(WhileNode& node) override;
    void visit(ForNode& node) override;
    
    // Expressions
    void visit(LiteralNode& node) override;
    void visit(ThisExpression& node) override;
    void visit(NewExpression& node) override;
    void visit(InputExpression& node) override;
    void visit(UnaryOpNode& node) override;
    void visit(UpdateNode& node) override;
    void visit(TernaryOpNode& node) override;
    void visit(GroupingNode& node) override;
    void visit(CallNode& node) override;
    void visit(MemberAccessNode& node) override;
    void visit(IndexNode& node) override;
    void visit(TemplateLiteralNode& node) override;
    void visit(ArrayLiteralNode& node) override;

    // Control Flow & Structures
    void visit(CaseNode& node) override;
    void visit(SwitchStatementNode& node) override;
    void visit(DoWhileNode& node) override;
    void visit(ForInNode& node) override;
    
    // Statements
    void visit(ImportNode& node) override;
    void visit(ExpressionStatementNode& node) override;
    void visit(MacroDefinitionNode& node) override;
    void visit(PrintStatementNode& node) override;
    void visit(BreakStatementNode& node) override;
    void visit(ContinueStatementNode& node) override;
    void visit(TableLiteralNode& node) override;
    void visit(DeleteStatementNode& node) override;
    void visit(LengthExpressionNode& node) override;
    void visit(LambdaNode& node) override;
    void visit(SpreadElementNode& node) override;
    void visit(UnsafeBlockNode& node) override;
    void visit(ThrowStatementNode& node) override;
    void visit(TryStatementNode& node) override;
    void visit(DeferStatementNode& node) override;
    void visit(WithStatementNode& node) override;
    void visit(AsmExpressionNode& node) override;  
    void visit(EnumDefinitionNode& node) override;
    void visit(TypeCastNode& node) override;
    void visit(SizeOfNode& node) override;
    void visit(StaticIfNode& node) override;
    void visit(TypedefNode& node) override;

    // Pass-throughs
    void visit(TypeNode& node) override;
    void visit(ParameterNode& node) override;
};

} // namespace Moksha