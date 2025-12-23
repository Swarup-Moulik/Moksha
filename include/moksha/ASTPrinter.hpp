#pragma once

#include "moksha/ast.hpp"
#include <string>
#include <sstream>
#include <vector>
#include <memory>

namespace Moksha {

class ASTPrinter : public ASTVisitor {
public:
    std::string print(const std::vector<std::unique_ptr<StatementNode>>& statements);

    // --- Visitor Implementation Overrides ---
    void visit(TypeNode& node) override;
    void visit(ParameterNode& node) override;
    void visit(BlockStatementNode& node) override;
    void visit(VariableDeclarationNode& node) override;
    void visit(FunctionDefinitionNode& node) override;
    void visit(ClassDefinitionNode& node) override;
    void visit(LiteralNode& node) override;
    void visit(VariableNode& node) override;
    void visit(UnaryOpNode& node) override;
    void visit(UpdateNode& node) override;
    void visit(BinaryOpNode& node) override;
    void visit(TernaryOpNode& node) override;
    void visit(GroupingNode& node) override;
    void visit(CallNode& node) override;
    void visit(MemberAccessNode& node) override;
    void visit(IndexNode& node) override;
    void visit(CaseNode& node) override;
    void visit(SwitchStatementNode& node) override;
    void visit(IfStatementNode& node) override;
    void visit(WhileNode& node) override;
    void visit(DoWhileNode& node) override;
    void visit(ForNode& node) override;
    void visit(ForInNode& node) override; 
    void visit(ReturnNode& node) override;
    void visit(ImportNode& node) override;
    void visit(ExpressionStatementNode& node) override;
    void visit(TemplateLiteralNode& node) override;
    void visit(ArrayLiteralNode& node) override;
    void visit(ThisExpression& node) override;
    void visit(NewExpression& node) override;
    void visit(InputExpression& node) override;
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
    void visit(EnumDefinitionNode& node) override;
    void visit(TypeCastNode& node) override;
    void visit(StructDefinitionNode& node) override;
    void visit(AsmExpressionNode& node) override;
    void visit(SizeOfNode& node) override;
    void visit(StaticIfNode& node) override;
    void visit(TypedefNode& node) override;

private:
    std::stringstream ss;
    int indentLevel = 0;

    // Helper to get current indentation string
    std::string pad();
    void increment();
    void decrement();
    
    // Helper to print "public ", "private ", etc.
    std::string printAccessLevel(AccessLevel level);
};

} // namespace Moksha