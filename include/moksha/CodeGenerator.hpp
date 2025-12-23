#pragma once

#include "moksha/ast.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Verifier.h>
#include <map>
#include <string>
#include <memory>
#include <vector>
#include <set>
#include <stack>

namespace Moksha
{
    // --- NEW HELPER DECLARATION ---
    // Takes the resolved type object and returns the corresponding LLVM type.
    llvm::Type *getLLVMType(std::shared_ptr<Type> resolvedType, llvm::LLVMContext &context, llvm::Module *module);

    class CodeGenerator : public ASTVisitor
    {
    private:
        bool wantAddress = false;
        bool insideUnsafe = false;
        // Core LLVM classes
        llvm::LLVMContext context;
        llvm::IRBuilder<> builder;
        std::unique_ptr<llvm::Module> module;

        // Maps variable names to their memory location (stack address)
        std::map<std::string, llvm::Value *> namedValues;

        std::set<std::string> heapAllocatedVars;
        std::set<std::string> sharedVars;
        std::map<std::string, bool> isRefClassMap;
        std::set<std::string> volatileVars;

        // Map to track Enum Constants (Name -> Integer Value)
        std::map<std::string, int> enumConstants;

        // Holds the result of the last expression visited
        llvm::Value *lastValue = nullptr;

        std::string currentClassName = "";                                   // Tracks if we are inside a class (for methods)
        std::map<std::string, std::map<std::string, int>> classFieldIndices; // Maps ClassName -> FieldName -> Index
        std::map<std::string, std::vector<llvm::Type *>> classLayouts;       // Maps ClassName -> List of Field Types (Ordered)
        llvm::Value *currentThis = nullptr;

        struct LoopInfo
        {
            llvm::BasicBlock *checkBB;
            llvm::BasicBlock *afterBB;
        };
        std::vector<LoopInfo> loopStack;
        std::vector<const StatementNode *> deferStack;
        std::map<std::string, std::string> variableTypes; // Maps varName -> className
        llvm::GlobalVariable *globalExceptionFlag = nullptr;
        llvm::GlobalVariable *globalExceptionVal = nullptr;
        std::stack<llvm::BasicBlock *> exceptionStack;
        std::string getStructName(std::shared_ptr<Moksha::Type> type);

        struct BitfieldMeta
        {
            int bitOffset;
            int bitWidth;
        };
        static std::map<std::string, std::map<std::string, BitfieldMeta>> structBitfields;

        llvm::Value *generateArrayAllocation(const std::vector<ExpressionNode *> &dims, size_t dimIndex, llvm::Value *leafDefault);
        llvm::Value *getAddress(ASTNode *node);

        llvm::Value *cloneObject(llvm::Value *src, const std::string &className);
        llvm::FunctionType *getLLVMFunctionProto(std::shared_ptr<FunctionType> mokshaType);
        void generateDefaultConstructor(const std::string &className, const std::string &parentName, const std::vector<std::unique_ptr<StatementNode>> &members);

        // Helper to register global exception state
        void setupGlobalExceptionState();

        bool isVolatile(const std::string &name);

    public:
        CodeGenerator();

        // Returns the generated module (to print or compile)
        llvm::Module *getModule() { return module.get(); }
        bool isFreestanding = false;

        // Main entry point
        void generate(const std::vector<std::unique_ptr<StatementNode>> &statements);

        // --- Visitor Implementation ---
        // Expressions
        void visit(LiteralNode &node) override;
        void visit(VariableNode &node) override;
        void visit(BinaryOpNode &node) override;
        void visit(GroupingNode &node) override;

        // Statements
        void visit(VariableDeclarationNode &node) override;
        void visit(ReturnNode &node) override;
        void visit(BlockStatementNode &node) override;
        void visit(ExpressionStatementNode &node) override;
        void visit(PrintStatementNode &node) override;

        // Complex Features
        void visit(FunctionDefinitionNode &node) override;
        void visit(ClassDefinitionNode &node) override;
        void visit(StructDefinitionNode &node) override;
        void visit(IfStatementNode &node) override;
        void visit(WhileNode &node) override;
        void visit(ForNode &node) override;
        void visit(DoWhileNode &node) override;
        void visit(SwitchStatementNode &node) override;
        void visit(CaseNode &node) override;
        void visit(ForInNode &node) override;
        void visit(ThisExpression &node) override;
        void visit(NewExpression &node) override;
        void visit(InputExpression &node) override;
        void visit(UnaryOpNode &node) override;
        void visit(UpdateNode &node) override;
        void visit(TernaryOpNode &node) override;
        void visit(CallNode &node) override;
        void visit(MemberAccessNode &node) override;
        void visit(IndexNode &node) override;
        void visit(TemplateLiteralNode &node) override;
        void visit(ArrayLiteralNode &node) override;
        void visit(ImportNode &node) override;
        void visit(MacroDefinitionNode &node) override;
        void visit(BreakStatementNode &node) override;
        void visit(ContinueStatementNode &node) override;
        void visit(TableLiteralNode &node) override;
        void visit(DeleteStatementNode &node) override;
        void visit(LengthExpressionNode &node) override;
        void visit(LambdaNode &node) override;
        void visit(SpreadElementNode &node) override;
        void visit(UnsafeBlockNode &node) override;
        void visit(ThrowStatementNode &node) override;
        void visit(TryStatementNode &node) override;
        void visit(DeferStatementNode &node) override;
        void visit(WithStatementNode &node) override;
        void visit(TypeNode &node) override;
        void visit(ParameterNode &node) override;
        void visit(EnumDefinitionNode &node) override;
        void visit(TypeCastNode &node) override;
        void visit(AsmExpressionNode &node) override;
        void visit(SizeOfNode &node) override;
        void visit(StaticIfNode &node) override;
        void visit(TypedefNode &node) override;
    };

} // namespace Moksha