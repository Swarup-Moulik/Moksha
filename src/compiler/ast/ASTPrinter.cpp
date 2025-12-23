#include "moksha/ASTPrinter.hpp"

namespace Moksha
{

    // --- Helpers ---

    std::string ASTPrinter::pad()
    {
        return std::string(indentLevel * 2, ' ');
    }

    void ASTPrinter::increment() { indentLevel++; }
    void ASTPrinter::decrement() { indentLevel--; }

    std::string ASTPrinter::printAccessLevel(AccessLevel level)
    {
        switch (level)
        {
        case AccessLevel::PUBLIC:
            return "public ";
        case AccessLevel::PRIVATE:
            return "private ";
        case AccessLevel::PROTECTED:
            return "protected ";
        default:
            return "";
        }
    }

    std::string ASTPrinter::print(const std::vector<std::unique_ptr<StatementNode>> &statements)
    {
        ss.str("");
        ss.clear();
        indentLevel = 0;
        ss << "(program";
        increment();
        for (const auto &stmt : statements)
        {
            ss << "\n"
               << pad();
            if (stmt)
                stmt->accept(*this);
            else
                ss << "(null-statement)";
        }
        decrement();
        ss << "\n)";
        return ss.str();
    }

    // --- Visitor Implementations ---

    void ASTPrinter::visit(TypeNode &node)
    {
        ss << "(type ";
        if (node.isVolatile)
            ss << "volatile ";
        ss << node.baseTypeToken.lexeme;
        for (int i = 0; i < node.pointerDepth; i++)
        {
            ss << "*";
        }
        if (node.isNullable)
            ss << "?";

        for (auto &dim : node.dimensions)
        {
            ss << "[";
            if (dim)
                dim->accept(*this);
            ss << "]";
        }
        if (node.elementType)
        {
            ss << " -> ";
            node.elementType->accept(*this);
        }
        ss << ")";
    }

    void ASTPrinter::visit(ParameterNode &node)
    {
        ss << "(";
        if (node.isRef)
            ss << "ref ";

        // CHANGED: Handle null type (Inferred/Any)
        if (node.type)
        {
            node.type->accept(*this);
            ss << " ";
        }
        else
        {
            ss << "any ";
        }

        ss << node.name.lexeme;
        if (node.defaultValue)
        {
            ss << " = ";
            node.defaultValue->accept(*this);
        }
        ss << ")";
    }

    void ASTPrinter::visit(BlockStatementNode &node)
    {
        if (node.statements.empty())
        {
            ss << "(block <empty>)";
            return;
        }
        ss << "(block";
        increment();
        for (const auto &stmt : node.statements)
        {
            ss << "\n"
               << pad();
            if (stmt)
                stmt->accept(*this);
        }
        decrement();
        ss << ")";
    }

    void ASTPrinter::visit(VariableDeclarationNode &node)
    {
        ss << "(" << printAccessLevel(node.visibility);
        if (node.isConst)
            ss << "const ";
        if (node.isShared)
            ss << "shared ";
        if (node.isVolatile)
            ss << "volatile ";
        if (node.isExtern)
            ss << "extern ";

        ss << "var " << node.name.lexeme;

        // [NEW] Bitfield Width
        if (node.bitWidth > 0)
        {
            ss << " : " << node.bitWidth;
        }

        if (node.type)
        {
            ss << " : ";
            node.type->accept(*this);
        }

        if (node.initializer)
        {
            ss << " = ";
            node.initializer->accept(*this);
        }
        ss << ")";
    }

    void ASTPrinter::visit(FunctionDefinitionNode &node)
    {
        ss << "(" << printAccessLevel(node.visibility);
        if (node.isExtern)
            ss << "extern ";
        if (node.isInterrupt)
            ss << "interrupt "; // [NEW]
        if (node.isAsync)
            ss << "async ";

        ss << "func " << node.name.lexeme << " (";
        for (size_t i = 0; i < node.parameters.size(); ++i)
        {
            if (i > 0)
                ss << ", ";
            node.parameters[i]->accept(*this);
        }

        // [NEW] Variadic Arguments
        if (node.isVariadic)
        {
            if (!node.parameters.empty())
                ss << ", ";
            ss << "...";
        }

        ss << ")";

        if (node.returnType)
        {
            ss << " -> ";
            node.returnType->accept(*this);
        }

        if (node.body)
        {
            increment();
            ss << "\n"
               << pad();
            node.body->accept(*this);
            decrement();
        }
        ss << ")";
    }

    void ASTPrinter::visit(ClassDefinitionNode &node)
    {
        ss << "(";
        if (node.isRef)
            ss << "ref ";
        ss << "class " << node.name.lexeme;

        if (!node.parentClassNames.empty())
        {
            ss << " :";
            for (auto &p : node.parentClassNames)
            {
                if (p)
                    ss << " " << p->name.lexeme;
            }
        }

        increment();
        for (const auto &member : node.members)
        {
            ss << "\n"
               << pad();
            if (member)
                member->accept(*this);
        }
        decrement();
        ss << ")";
    }

    void ASTPrinter::visit(LiteralNode &node)
    {
        if (node.token.type == TokenType::STRING_LITERAL || node.token.type == TokenType::STRING_CHUNK)
        {
            ss << "\"" << node.token.lexeme << "\"";
        }
        else
        {
            ss << node.token.lexeme;
        }
    }

    void ASTPrinter::visit(VariableNode &node)
    {
        ss << node.name.lexeme;
    }

    void ASTPrinter::visit(UnaryOpNode &node)
    {
        ss << "(" << node.op.lexeme << " ";
        if (node.right)
            node.right->accept(*this);
        else
            ss << "null";
        ss << ")";
    }

    void ASTPrinter::visit(UpdateNode &node)
    {
        ss << "(" << node.op.lexeme << (node.isPrefix ? " pre " : " post ");
        if (node.argument)
            node.argument->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(BinaryOpNode &node)
    {
        ss << "(" << node.op.lexeme << " ";
        if (node.left)
            node.left->accept(*this);
        else
            ss << "null";
        ss << " ";
        if (node.right)
            node.right->accept(*this);
        else
            ss << "null";
        ss << ")";
    }

    void ASTPrinter::visit(TernaryOpNode &node)
    {
        ss << "(ternary ";
        if (node.condition)
            node.condition->accept(*this);
        ss << " ? ";
        if (node.thenBranch)
            node.thenBranch->accept(*this);
        ss << " : ";
        if (node.elseBranch)
            node.elseBranch->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(GroupingNode &node)
    {
        ss << "(group ";
        if (node.expression)
            node.expression->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(CallNode &node)
    {
        ss << "(call" << (node.isOptional ? "?" : "") << " ";
        if (node.callee)
            node.callee->accept(*this);
        ss << " args(";
        for (const auto &arg : node.arguments)
        {
            ss << " ";
            if (arg)
                arg->accept(*this);
        }
        ss << "))";
    }

    void ASTPrinter::visit(MemberAccessNode &node)
    {
        ss << "(. " << (node.isOptional ? "?" : "") << " ";
        if (node.object)
            node.object->accept(*this);
        ss << " " << node.name.lexeme << ")";
    }

    void ASTPrinter::visit(IndexNode &node)
    {
        ss << "(index" << (node.isOptional ? "?" : "") << " ";
        if (node.callee)
            node.callee->accept(*this);
        ss << " [";
        if (node.index)
            node.index->accept(*this);
        ss << "])";
    }

    void ASTPrinter::visit(CaseNode &node)
    {
        ss << "(case ";
        if (node.type == CaseType::RANGE)
        {
            if (node.value)
                node.value->accept(*this);
            ss << "..";
            if (node.rangeEnd)
                node.rangeEnd->accept(*this);
        }
        else
        {
            if (node.value)
                node.value->accept(*this);
            else
                ss << "default";
        }

        increment();
        for (const auto &stmt : node.statements)
        {
            ss << "\n"
               << pad();
            if (stmt)
                stmt->accept(*this);
        }
        decrement();
        ss << ")";
    }

    void ASTPrinter::visit(SwitchStatementNode &node)
    {
        ss << "(switch ";
        if (node.condition)
            node.condition->accept(*this);
        increment();
        for (const auto &c : node.cases)
        {
            ss << "\n"
               << pad();
            if (c)
                c->accept(*this);
        }
        if (node.defaultCase)
        {
            ss << "\n"
               << pad();
            node.defaultCase->accept(*this);
        }
        decrement();
        ss << ")";
    }

    void ASTPrinter::visit(IfStatementNode &node)
    {
        ss << "(if ";
        if (node.condition)
            node.condition->accept(*this);

        ss << "\n"
           << pad() << "then ";
        if (node.thenBranch)
            node.thenBranch->accept(*this);

        if (node.elseBranch)
        {
            ss << "\n"
               << pad() << "else ";
            node.elseBranch->accept(*this);
        }
        ss << ")";
    }

    void ASTPrinter::visit(WhileNode &node)
    {
        ss << "(while ";
        if (node.condition)
            node.condition->accept(*this);
        ss << "\n"
           << pad();
        if (node.body)
            node.body->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(DoWhileNode &node)
    {
        ss << "(do-while ";
        if (node.condition)
            node.condition->accept(*this);
        ss << "\n"
           << pad();
        if (node.body)
            node.body->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(ForNode &node)
    {
        ss << "(for ";
        if (node.initializer)
            node.initializer->accept(*this);
        else
            ss << "null";
        ss << "; ";
        if (node.condition)
            node.condition->accept(*this);
        else
            ss << "null";
        ss << "; ";
        if (node.increment)
            node.increment->accept(*this);
        else
            ss << "null";
        ss << "\n"
           << pad();
        if (node.body)
            node.body->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(ForInNode &node)
    {
        ss << "(for-in " << node.keyVar.lexeme;
        if (node.hasValueVar)
            ss << "," << node.valVar.lexeme;
        ss << " in ";
        if (node.iterable)
            node.iterable->accept(*this);
        ss << "\n"
           << pad();
        if (node.body)
            node.body->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(ReturnNode &node)
    {
        ss << "(return";
        if (node.value)
        {
            ss << " ";
            node.value->accept(*this);
        }
        ss << ")";
    }

    void ASTPrinter::visit(ImportNode &node)
    {
        ss << "(import " << node.moduleName.lexeme;
        if (node.isFromImport)
            ss << " from";
        ss << " [";
        for (const auto &token : node.importedSymbols)
            ss << " " << token.lexeme;
        ss << " ])";
    }

    void ASTPrinter::visit(ExpressionStatementNode &node)
    {
        ss << "(expr-stmt ";
        if (node.expression)
            node.expression->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(TemplateLiteralNode &node)
    {
        ss << "(template";
        for (const auto &part : node.parts)
        {
            ss << " ";
            if (part)
                part->accept(*this);
        }
        ss << ")";
    }

    void ASTPrinter::visit(ArrayLiteralNode &node)
    {
        ss << "(array";
        for (const auto &el : node.elements)
        {
            ss << " ";
            if (el)
                el->accept(*this);
            else
                ss << "null";
        }
        ss << ")";
    }

    void ASTPrinter::visit(ThisExpression &node) { ss << "this"; }

    void ASTPrinter::visit(NewExpression &node)
    {
        ss << "(new " << node.className.lexeme << " (";
        for (const auto &arg : node.arguments)
        {
            ss << " ";
            if (arg)
                arg->accept(*this);
        }
        ss << "))";
    }

    void ASTPrinter::visit(InputExpression &node) { ss << "(input " << node.token.lexeme << ")"; }

    void ASTPrinter::visit(MacroDefinitionNode &node)
    {
        ss << "(macro " << node.name.lexeme;
        ss << "\n"
           << pad();
        if (node.body)
            node.body->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(PrintStatementNode &node)
    {
        ss << "(" << node.variant.lexeme << " ";
        if (node.expression)
            node.expression->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(BreakStatementNode &node) { ss << "(break)"; }
    void ASTPrinter::visit(ContinueStatementNode &node) { ss << "(continue)"; }

    void ASTPrinter::visit(TableLiteralNode &node)
    {
        ss << "(table";
        for (const auto &pair : node.entries)
        {
            ss << " {k:";
            if (pair.first)
                pair.first->accept(*this);
            else
                ss << "null";
            ss << " v:";
            if (pair.second)
                pair.second->accept(*this);
            else
                ss << "null";
            ss << "}";
        }
        ss << ")";
    }

    void ASTPrinter::visit(DeleteStatementNode &node)
    {
        ss << "(delete ";
        if (node.target)
            node.target->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(LengthExpressionNode &node)
    {
        ss << "(length ";
        if (node.target)
            node.target->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(LambdaNode &node)
    {
        ss << "(lambda (";
        for (size_t i = 0; i < node.parameters.size(); ++i)
        {
            if (i > 0)
                ss << " ";
            if (node.parameters[i])
                node.parameters[i]->accept(*this);
        }
        ss << ")";

        // We don't indent expression bodies to keep them compact,
        // but block bodies will handle their own indentation.
        if (dynamic_cast<BlockStatementNode *>(node.body.get()))
        {
            ss << "\n"
               << pad();
            node.body->accept(*this);
        }
        else
        {
            ss << " => ";
            if (node.body)
                node.body->accept(*this);
        }
        ss << ")";
    }

    void ASTPrinter::visit(SpreadElementNode &node)
    {
        ss << "(spread ";
        if (node.expression)
            node.expression->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(UnsafeBlockNode &node)
    {
        ss << "(unsafe";
        if (node.body)
        {
            node.body->accept(*this);
        }
        ss << ")";
    }

    void ASTPrinter::visit(ThrowStatementNode &node)
    {
        ss << "(throw ";
        if (node.expression)
            node.expression->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(TryStatementNode &node)
    {
        ss << "(try ";
        if (node.tryBlock)
            node.tryBlock->accept(*this);

        if (node.catchBlock)
        {
            ss << "\n"
               << pad() << "catch (" << node.errorVariable.lexeme << ") ";
            node.catchBlock->accept(*this);
        }

        if (node.finallyBlock)
        {
            ss << "\n"
               << pad() << "finally ";
            node.finallyBlock->accept(*this);
        }
        ss << ")";
    }

    void ASTPrinter::visit(DeferStatementNode &node)
    {
        ss << "(defer ";
        if (node.statement)
            node.statement->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(WithStatementNode &node)
    {
        ss << "(with ";
        if (node.declaration)
            node.declaration->accept(*this);
        ss << "\n"
           << pad();
        if (node.body)
            node.body->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(EnumDefinitionNode &node)
    {
        ss << "(enum " << node.name.lexeme;
        increment();
        for (const auto &member : node.members)
        {
            ss << "\n"
               << pad() << "(" << member.first.lexeme;
            if (member.second)
            {
                ss << " = ";
                member.second->accept(*this);
            }
            ss << ")";
        }
        decrement();
        ss << ")";
    }

    void ASTPrinter::visit(TypeCastNode &node)
    {
        ss << "(cast ";
        if (node.targetType)
            node.targetType->accept(*this);
        else
            ss << "unknown-type";

        ss << " ";
        if (node.expression)
            node.expression->accept(*this);
        else
            ss << "null-expr";

        ss << ")";
    }

    void ASTPrinter::visit(StructDefinitionNode &node)
    {
        if (node.isUnion)
            ss << "(union ";
        else if (node.isPacked)
            ss << "(packed-struct ";
        else
            ss << "(struct ";
        ss << node.name.lexeme;
        increment();
        for (auto &field : node.members)
        {
            ss << "\n"
               << pad();
            if (field)
                field->accept(*this);
        }
        decrement();
        ss << ")";
    }

    void ASTPrinter::visit(AsmExpressionNode &node)
    {
        ss << "(asm \"" << node.assemblyTemplate << "\" : \"" << node.constraints << "\")";
    }

    void ASTPrinter::visit(SizeOfNode &node)
    {
        ss << "(sizeof ";
        if (node.targetType)
            node.targetType->accept(*this);
        else if (node.expression)
            node.expression->accept(*this);
        ss << ")";
    }

    void ASTPrinter::visit(StaticIfNode &node)
    {
        ss << "(static-if ";
        node.condition->accept(*this);
        increment();
        ss << "\n"
           << pad();
        node.thenBranch->accept(*this);
        if (node.elseBranch)
        {
            ss << "\n"
               << pad() << "else ";
            node.elseBranch->accept(*this);
        }
        decrement();
        ss << ")";
    }

    void ASTPrinter::visit(TypedefNode &node)
    {
        ss << "(typedef " << node.aliasName.lexeme << " ";
        if (node.sourceType)
            node.sourceType->accept(*this);
        ss << ")";
    }

} // namespace Moksha