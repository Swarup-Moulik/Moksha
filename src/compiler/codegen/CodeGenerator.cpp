#include "moksha/CodeGenerator.hpp"
#include "moksha/SemanticAnalyzer.hpp"
#include <iostream>
#include <vector>
#include <map>
#include <stack>
#include <typeinfo>
#include <algorithm>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Triple.h>
#include <llvm/IR/InlineAsm.h>

namespace Moksha
{

    // --- Global Compiler State (Static Helpers) ---
    // Note: classFieldIndices and classLayouts are MEMBERS of CodeGenerator, so they are not here.
    static std::map<std::string, FunctionDefinitionNode *> globalFunctionDefs;
    static std::map<std::string, std::vector<VariableDeclarationNode *>> classFieldInitializers;
    static std::map<std::string, std::map<std::string, int>> scopedEnumConstants;

    // Structs are currently tracked globally per translation unit (simplifies getLLVMType)
    static std::map<std::string, llvm::StructType *> structLLVMTypes;
    static std::map<std::string, std::map<std::string, int>> structFieldIndices;
    static std::map<std::string, bool> structIsUnion;

    // Definition of the static member from header
    std::map<std::string, std::map<std::string, CodeGenerator::BitfieldMeta>> CodeGenerator::structBitfields;

    // --- C-Runtime Declarations ---
    static void setupMokshaRuntime(llvm::Module *module, llvm::LLVMContext &context)
    {
        llvm::IRBuilder<> builder(context);

        // Standard C Library
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, true), llvm::Function::ExternalLinkage, "printf", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt64Ty()}, false), llvm::Function::ExternalLinkage, "malloc", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt64Ty(), builder.getInt64Ty()}, false), llvm::Function::ExternalLinkage, "calloc", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy(), builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_print_val", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_free", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "strcpy", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getInt64Ty()}, false), llvm::Function::ExternalLinkage, "memcpy", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt32Ty(), builder.getInt64Ty()}, false), llvm::Function::ExternalLinkage, "memset", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "strlen", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "puts", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy(), builder.getPtrTy()}, true), llvm::Function::ExternalLinkage, "sprintf", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "strcmp", module);

        // Boxing / Unboxing
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_box_int", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getDoubleTy()}, false), llvm::Function::ExternalLinkage, "moksha_box_double", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_box_bool", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_box_string", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt8Ty()}, false), llvm::Function::ExternalLinkage, "moksha_box_char", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt16Ty()}, false), llvm::Function::ExternalLinkage, "moksha_box_short", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt64Ty()}, false), llvm::Function::ExternalLinkage, "moksha_box_long", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getFloatTy()}, false), llvm::Function::ExternalLinkage, "moksha_box_float", module);

        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_unbox_int", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getDoubleTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_unbox_double", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_unbox_string", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_any_to_str", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_int_to_str", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_int_to_ascii", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getDoubleTy()}, false), llvm::Function::ExternalLinkage, "moksha_double_to_str", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_ptr_to_str", module);

        // Arrays & Tables
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_alloc_array", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_alloc_table", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getInt32Ty(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_alloc_array_fill", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_table_set", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_table_get", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_table_delete", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_table_keys", module);

        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_array_get_unsafe", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_array_get", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy(), builder.getInt32Ty(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_array_set", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_array_push_val", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_array_spread", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy(), builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_array_delete", module);

        // Strings
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_string_concat", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_get_length", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt32Ty()}, false), llvm::Function::ExternalLinkage, "moksha_string_get_char", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_strlen", module);

        // Inputs
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {}, false), llvm::Function::ExternalLinkage, "moksha_read_int", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getDoubleTy(), {}, false), llvm::Function::ExternalLinkage, "moksha_read_double", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_unbox_bool", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getInt32Ty(), {}, false), llvm::Function::ExternalLinkage, "moksha_read_bool", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {}, false), llvm::Function::ExternalLinkage, "moksha_read_string", module);

        // Runtime Exception & Closures
        llvm::Function::Create(llvm::FunctionType::get(builder.getVoidTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_throw_exception", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt32Ty(), builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_create_closure", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_get_closure_env", module);
        llvm::Function::Create(llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false), llvm::Function::ExternalLinkage, "moksha_get_closure_func", module);
    }

    // --- getLLVMType Helper ---
    llvm::Type *getLLVMType(std::shared_ptr<Type> resolvedType, llvm::LLVMContext &context, llvm::Module *module)
    {
        if (!resolvedType)
            return llvm::PointerType::get(context, 0);

        if (auto arrType = std::dynamic_pointer_cast<ArrayType>(resolvedType))
        {
            if (arrType->isFixedSize)
            {
                llvm::Type *elemTy = getLLVMType(arrType->elementType, context, module);
                return llvm::ArrayType::get(elemTy, arrType->fixedSize);
            }
        }
        if (resolvedType->isNullable)
            return llvm::PointerType::get(context, 0);

        if (std::dynamic_pointer_cast<IntType>(resolvedType))
            return llvm::Type::getInt32Ty(context);
        if (std::dynamic_pointer_cast<DoubleType>(resolvedType))
            return llvm::Type::getDoubleTy(context);
        if (std::dynamic_pointer_cast<BooleanType>(resolvedType))
            return llvm::Type::getInt1Ty(context);
        if (std::dynamic_pointer_cast<VoidType>(resolvedType))
            return llvm::Type::getVoidTy(context);
        if (std::dynamic_pointer_cast<CharType>(resolvedType))
            return llvm::Type::getInt8Ty(context);
        if (std::dynamic_pointer_cast<ShortType>(resolvedType))
            return llvm::Type::getInt16Ty(context);
        if (std::dynamic_pointer_cast<LongType>(resolvedType))
            return llvm::Type::getInt64Ty(context);
        if (std::dynamic_pointer_cast<FloatType>(resolvedType))
            return llvm::Type::getFloatTy(context);

        if (auto structType = std::dynamic_pointer_cast<StructType>(resolvedType))
        {
            if (structLLVMTypes.count(structType->structName))
            {
                return structLLVMTypes[structType->structName];
            }
            return llvm::PointerType::getUnqual(context);
        }
        return llvm::PointerType::get(context, 0); // Default to ptr
    }

    bool CodeGenerator::isVolatile(const std::string &name)
    {
        return volatileVars.count(name) > 0;
    }

    std::string CodeGenerator::getStructName(std::shared_ptr<Moksha::Type> type)
    {
        if (auto st = std::dynamic_pointer_cast<StructType>(type))
        {
            return st->structName;
        }
        if (auto ptr = std::dynamic_pointer_cast<PointerType>(type))
        {
            if (auto st = std::dynamic_pointer_cast<StructType>(ptr->pointeeType))
            {
                return st->structName;
            }
        }
        return "";
    }

    llvm::FunctionType *CodeGenerator::getLLVMFunctionProto(std::shared_ptr<FunctionType> mokshaType)
    {
        llvm::Type *retType = getLLVMType(mokshaType->returnType, context, module.get());
        std::vector<llvm::Type *> args;
        // Add implicit 'this' (void*)
        args.push_back(builder.getPtrTy());
        for (auto &p : mokshaType->parameterTypes)
        {
            args.push_back(getLLVMType(p, context, module.get()));
        }
        return llvm::FunctionType::get(retType, args, mokshaType->isVariadic);
    }

    static llvm::AllocaInst *CreateEntryBlockAlloca(llvm::Function *TheFunction, const std::string &VarName, llvm::Type *type)
    {
        llvm::IRBuilder<> TmpB(&TheFunction->getEntryBlock(), TheFunction->getEntryBlock().begin());
        return TmpB.CreateAlloca(type, nullptr, VarName);
    }

    llvm::Value *CodeGenerator::generateArrayAllocation(const std::vector<ExpressionNode *> &dims, size_t dimIndex, llvm::Value *leafDefault)
    {
        dims[dimIndex]->accept(*this);
        llvm::Value *size = lastValue;

        if (size->getType()->isDoubleTy())
            size = builder.CreateFPToSI(size, builder.getInt32Ty());
        if (!size->getType()->isIntegerTy(32))
            size = builder.CreateZExtOrTrunc(size, builder.getInt32Ty());

        if (dimIndex == dims.size() - 1)
        {
            return builder.CreateCall(module->getFunction("moksha_alloc_array_fill"), {size, leafDefault}, "arr_leaf");
        }

        llvm::Value *outerArr = builder.CreateCall(module->getFunction("moksha_alloc_array"), {size}, "outer_arr");

        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "alloc_cond", func);
        llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "alloc_body", func);
        llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "alloc_after", func);

        // We need a Phi node for outerArr because it changes inside the loop
        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);

        llvm::PHINode *currentArrPtr = builder.CreatePHI(builder.getPtrTy(), 2, "arr_ptr_phi");
        currentArrPtr->addIncoming(outerArr, builder.GetInsertBlock()->getSinglePredecessor());

        llvm::PHINode *idx = builder.CreatePHI(builder.getInt32Ty(), 2, "idx_phi");
        idx->addIncoming(builder.getInt32(0), builder.GetInsertBlock()->getSinglePredecessor());

        llvm::Value *cmp = builder.CreateICmpSLT(idx, size);
        builder.CreateCondBr(cmp, bodyBB, afterBB);

        // Loop Body
        builder.SetInsertPoint(bodyBB);
        llvm::Value *innerArr = generateArrayAllocation(dims, dimIndex + 1, leafDefault);

        // Update the pointer
        llvm::Value *updatedArrPtr = builder.CreateCall(module->getFunction("moksha_array_push_val"), {currentArrPtr, innerArr});

        llvm::Value *nextIdx = builder.CreateAdd(idx, builder.getInt32(1));

        // Update Phi inputs
        currentArrPtr->addIncoming(updatedArrPtr, bodyBB);
        idx->addIncoming(nextIdx, bodyBB);

        builder.CreateBr(condBB);

        builder.SetInsertPoint(afterBB);
        return currentArrPtr;
    }

    llvm::Value *CodeGenerator::getAddress(ASTNode *node)
    {
        if (auto varNode = dynamic_cast<VariableNode *>(node))
        {
            // 1. Look up variable in the correct member 'namedValues'
            if (namedValues.count(varNode->name.lexeme))
            {
                return namedValues[varNode->name.lexeme]; // Returns the address (AllocaInst*)
            }

            std::cerr << "Error: Unknown variable " << varNode->name.lexeme << std::endl;
            return nullptr;
        }
        return nullptr;
    }

    // Helper: Universal Cast & Unbox
    llvm::Value *emitCast(llvm::IRBuilder<> &builder, llvm::Module *module, llvm::Value *val,
                          std::shared_ptr<Type> srcType, std::shared_ptr<Type> destType)
    {
        if (!val)
            return val;
        llvm::Type *valTy = val->getType();

        if (std::dynamic_pointer_cast<StringType>(destType))
        {
            // Already a string pointer?
            if (valTy->isPointerTy() && std::dynamic_pointer_cast<StringType>(srcType))
            {
                if (valTy != builder.getPtrTy())
                    return builder.CreateBitCast(val, builder.getPtrTy());
                return val;
            }

            llvm::Value *rawStr = nullptr;

            // Int -> String
            if (valTy->isIntegerTy(32))
            {
                rawStr = builder.CreateCall(module->getFunction("moksha_int_to_ascii"), {val}, "i2c");
            }
            // Double -> String
            else if (valTy->isDoubleTy())
            {
                rawStr = builder.CreateCall(module->getFunction("moksha_double_to_str"), {val}, "d2s");
            }
            // Bool -> String
            else if (valTy->isIntegerTy(1))
            {
                llvm::Value *i32Val = builder.CreateZExt(val, builder.getInt32Ty());
                llvm::Value *boxed = builder.CreateCall(module->getFunction("moksha_box_bool"), {i32Val});
                rawStr = builder.CreateCall(module->getFunction("moksha_any_to_str"), {boxed});
            }
            // Any -> String
            else if (valTy->isPointerTy())
            {
                if (valTy != builder.getPtrTy())
                    val = builder.CreateBitCast(val, builder.getPtrTy());
                rawStr = builder.CreateCall(module->getFunction("moksha_any_to_str"), {val});
                // moksha_any_to_str returns char*, we need to box it into a String Object
                return builder.CreateCall(module->getFunction("moksha_box_string"), {rawStr});
            }

            if (rawStr)
            {
                return builder.CreateCall(module->getFunction("moksha_box_string"), {rawStr});
            }
        }

        bool destIsNullable = destType && destType->isNullable;
        bool destIsAny = std::dynamic_pointer_cast<AnyType>(destType) != nullptr;

        // 1. Boxing: Source is Primitive/String, Destination is Any OR Nullable
        if (destIsAny || destIsNullable)
        {
            // Box Primitives
            if (valTy->isIntegerTy(32))
                return builder.CreateCall(module->getFunction("moksha_box_int"), {val}, "box_i");
            if (valTy->isIntegerTy(8))
                return builder.CreateCall(module->getFunction("moksha_box_char"), {val}, "box_c");
            if (valTy->isIntegerTy(16))
                return builder.CreateCall(module->getFunction("moksha_box_short"), {val}, "box_s");
            if (valTy->isIntegerTy(64))
                return builder.CreateCall(module->getFunction("moksha_box_long"), {val}, "box_l");
            if (valTy->isFloatTy()) // 32-bit float
                return builder.CreateCall(module->getFunction("moksha_box_float"), {val}, "box_f");
            if (valTy->isDoubleTy())
                return builder.CreateCall(module->getFunction("moksha_box_double"), {val}, "box_d");
            if (valTy->isIntegerTy(1))
            {
                llvm::Value *i32Val = builder.CreateZExt(val, builder.getInt32Ty());
                return builder.CreateCall(module->getFunction("moksha_box_bool"), {i32Val}, "box_b");
            }
        }

        // 2. Unboxing: Source is Any OR Nullable, Destination is Primitive
        bool srcIsAny = std::dynamic_pointer_cast<AnyType>(srcType) != nullptr;
        bool srcIsNullable = srcType && srcType->isNullable;
        bool srcIsString = std::dynamic_pointer_cast<StringType>(srcType) != nullptr;

        if ((srcIsAny || srcIsNullable || srcIsString) && !destIsNullable && !destIsAny)
        {
            if (!val->getType()->isPointerTy())
            {
                if (val->getType()->isIntegerTy(32))
                    val = builder.CreateCall(module->getFunction("moksha_box_int"), {val});
                else if (val->getType()->isDoubleTy())
                    val = builder.CreateCall(module->getFunction("moksha_box_double"), {val});
                else if (val->getType()->isIntegerTy(1))
                {
                    llvm::Value *b = builder.CreateZExt(val, builder.getInt32Ty());
                    val = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                }
            }

            llvm::Value *castedVal = val;
            if (val->getType() != builder.getPtrTy())
            {
                castedVal = builder.CreateBitCast(val, builder.getPtrTy(), "safe_cast");
            }

            if (std::dynamic_pointer_cast<IntType>(destType))
                return builder.CreateCall(module->getFunction("moksha_unbox_int"), {castedVal}, "unbox_i");
            if (std::dynamic_pointer_cast<DoubleType>(destType))
                return builder.CreateCall(module->getFunction("moksha_unbox_double"), {castedVal}, "unbox_d");
            if (std::dynamic_pointer_cast<BooleanType>(destType))
            {
                llvm::Value *i32Val = builder.CreateCall(module->getFunction("moksha_unbox_bool"), {castedVal}, "unbox_b");
                return builder.CreateTrunc(i32Val, builder.getInt1Ty());
            }
        }

        if (valTy->isIntegerTy())
        {
            if (std::dynamic_pointer_cast<ShortType>(destType))
                return builder.CreateIntCast(val, builder.getInt16Ty(), true, "to_i16");
            if (std::dynamic_pointer_cast<CharType>(destType))
                return builder.CreateIntCast(val, builder.getInt8Ty(), true, "to_i8");
            if (std::dynamic_pointer_cast<IntType>(destType))
                return builder.CreateIntCast(val, builder.getInt32Ty(), true, "to_i32");
            if (std::dynamic_pointer_cast<LongType>(destType))
                return builder.CreateIntCast(val, builder.getInt64Ty(), true, "to_i64");
        }

        // 3. LLVM Level Casts (Numeric Promotion)
        if (valTy->isIntegerTy() && std::dynamic_pointer_cast<DoubleType>(destType))
            return builder.CreateSIToFP(val, builder.getDoubleTy(), "cast_i2d");

        if (valTy->isDoubleTy() && std::dynamic_pointer_cast<IntType>(destType))
            return builder.CreateFPToSI(val, builder.getInt32Ty(), "cast_d2i");

        return val;
    }

    void CodeGenerator::generateDefaultConstructor(const std::string &className,
                                                   const std::string &parentName, // <--- NEW ARGUMENT
                                                   const std::vector<std::unique_ptr<StatementNode>> &members)
    {
        std::string funcName = className + "_constructor";
        if (module->getFunction(funcName))
            return;

        // 1. Determine Signature (Inherit from Parent or Default to Void)
        llvm::Function *parentCtor = nullptr;
        std::vector<llvm::Type *> argTypes;

        // Always start with 'this' pointer
        argTypes.push_back(builder.getPtrTy());

        if (!parentName.empty())
        {
            std::string parentCtorName = parentName + "_constructor";
            parentCtor = module->getFunction(parentCtorName);

            if (parentCtor)
            {
                // Copy Parent Args (Skip 'this' at index 0)
                for (size_t i = 1; i < parentCtor->arg_size(); i++)
                {
                    argTypes.push_back(parentCtor->getArg(i)->getType());
                }
            }
        }

        // 2. Define Function
        llvm::FunctionType *ft = llvm::FunctionType::get(builder.getVoidTy(), argTypes, false);
        llvm::Function *ctor = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, funcName, module.get());

        // 3. Setup Body
        llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", ctor);
        llvm::BasicBlock *oldBlock = builder.GetInsertBlock();
        builder.SetInsertPoint(entry);

        llvm::Value *thisPtr = ctor->getArg(0);
        thisPtr->setName("this");

        // 4. Call Parent Constructor (The Forwarding Magic)
        if (parentCtor)
        {
            std::vector<llvm::Value *> parentArgs;
            // Cast 'this' to Parent Type (usually implicit with opaque pointers)
            parentArgs.push_back(thisPtr);

            // Forward all other arguments
            for (size_t i = 1; i < ctor->arg_size(); i++)
            {
                parentArgs.push_back(ctor->getArg(i));
            }

            builder.CreateCall(parentCtor, parentArgs);
        }

        // 5. Initialize Own Fields (Standard logic)
        llvm::StructType *st = llvm::StructType::getTypeByName(context, className);
        if (classFieldInitializers.count(className))
        {
            for (auto *varDecl : classFieldInitializers[className])
            {
                if (varDecl->initializer)
                {
                    varDecl->initializer->accept(*this);
                    llvm::Value *initVal = lastValue;
                    initVal = emitCast(builder, module.get(), initVal, varDecl->initializer->resolvedType, varDecl->type->resolvedType);

                    int idx = classFieldIndices[className][varDecl->name.lexeme];
                    llvm::Value *fieldPtr = builder.CreateStructGEP(st, thisPtr, idx);

                    if (initVal->getType() != st->getElementType(idx))
                    {
                        if (initVal->getType()->isPointerTy())
                            initVal = builder.CreateBitCast(initVal, st->getElementType(idx));
                    }
                    builder.CreateStore(initVal, fieldPtr);
                }
            }
        }

        builder.CreateRetVoid();
        if (oldBlock)
            builder.SetInsertPoint(oldBlock);
    }

    llvm::Value *CodeGenerator::cloneObject(llvm::Value *src, const std::string &className)
    {
        llvm::StructType *st = llvm::StructType::getTypeByName(context, className);
        if (!st)
            return src;

        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *cloneBB = llvm::BasicBlock::Create(context, "clone_do", func);
        llvm::BasicBlock *skipBB = llvm::BasicBlock::Create(context, "clone_skip", func);
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "clone_merge", func);

        // Check if src is null
        llvm::Value *isNull = builder.CreateICmpEQ(src, llvm::ConstantPointerNull::get(builder.getPtrTy()));
        builder.CreateCondBr(isNull, skipBB, cloneBB);

        // --- Block: Do Clone ---
        builder.SetInsertPoint(cloneBB);
        const llvm::DataLayout &dl = module->getDataLayout();
        uint64_t size = dl.getTypeAllocSize(st);

        llvm::Function *mallocFunc = module->getFunction("malloc");
        llvm::Value *newPtr = builder.CreateCall(llvm::FunctionCallee(mallocFunc), {builder.getInt64(size)}, "clone_alloc");

        llvm::Value *srcPtr = builder.CreateBitCast(src, builder.getPtrTy());
        llvm::Value *destPtr = builder.CreateBitCast(newPtr, builder.getPtrTy());
        // MemCpy from src to newPtr
        builder.CreateMemCpy(destPtr, llvm::MaybeAlign(8), srcPtr, llvm::MaybeAlign(8), size);

        llvm::Value *clonedVal = builder.CreateBitCast(newPtr, llvm::PointerType::get(context, 0));
        builder.CreateBr(mergeBB);

        // --- Block: Skip (Return Null) ---
        builder.SetInsertPoint(skipBB);
        builder.CreateBr(mergeBB);

        // --- Block: Merge ---
        builder.SetInsertPoint(mergeBB);
        llvm::PHINode *phi = builder.CreatePHI(builder.getPtrTy(), 2, "clone_res");
        phi->addIncoming(clonedVal, cloneBB);
        phi->addIncoming(llvm::ConstantPointerNull::get(builder.getPtrTy()), skipBB);

        return phi;
    }

    // --- Constructor & Setup ---

    CodeGenerator::CodeGenerator() : builder(context)
    {
        module = std::make_unique<llvm::Module>("moksha_module", context);
        setupMokshaRuntime(module.get(), context);
        setupGlobalExceptionState();
        fnArrayGetUnsafe = module->getFunction("moksha_array_get_unsafe");
        fnStringGetChar = module->getFunction("moksha_string_get_char");
    }

    void CodeGenerator::setupGlobalExceptionState()
    {
        module->getOrInsertGlobal("__exception_flag", builder.getInt32Ty());
        globalExceptionFlag = module->getNamedGlobal("__exception_flag");
        globalExceptionFlag->setLinkage(llvm::GlobalValue::ExternalLinkage);

        module->getOrInsertGlobal("__exception_val", builder.getPtrTy());
        globalExceptionVal = module->getNamedGlobal("__exception_val");
        globalExceptionVal->setLinkage(llvm::GlobalValue::ExternalLinkage);
    }

    void CodeGenerator::generate(const std::vector<std::unique_ptr<StatementNode>> &statements)
    {
        llvm::FunctionType *funcType = llvm::FunctionType::get(builder.getInt32Ty(), false);
        llvm::Function *mainFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", module.get());

        llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", mainFunc);
        builder.SetInsertPoint(entry);

        std::vector<llvm::Type *> printfArgs;
        printfArgs.push_back(builder.getPtrTy());
        llvm::FunctionType *printfType = llvm::FunctionType::get(
            builder.getInt32Ty(), printfArgs, true);
        module->getOrInsertFunction("printf", printfType);

        for (const auto &stmt : statements)
        {
            if (stmt)
                stmt->accept(*this);

            // Safety: If a statement (like return) explicitly finished the block, stop generating.
            if (builder.GetInsertBlock()->getTerminator())
                break;
        }

        // Check the *Current* block, not the *Entry* block.
        // If the last block (e.g., after an 'if' or 'print') has no return, add "return 0".
        if (!builder.GetInsertBlock()->getTerminator())
        {
            builder.CreateRet(builder.getInt32(0));
        }

        if (llvm::verifyFunction(*mainFunc, &llvm::errs()))
        {
            std::cerr << "Error: Main function verification failed!" << std::endl;
        }
    }

    // --- Visitors ---

    void CodeGenerator::visit(VariableDeclarationNode &node)
    {
        llvm::Type *valType = getLLVMType(node.type->resolvedType, context, module.get());
        llvm::Function *currentFunction = builder.GetInsertBlock()->getParent();
        bool isGlobalScope = false;

        if (isGlobalScope && !node.isExtern)
        {
            llvm::Constant *initializer = nullptr;

            // NEW: Check if this is a hardware array (like the Multiboot header)
            if (auto arrayLit = dynamic_cast<ArrayLiteralNode *>(node.initializer.get()))
            {
                std::vector<llvm::Constant *> constants;
                for (auto &el : arrayLit->elements)
                {
                    el->accept(*this);
                    if (auto c = llvm::dyn_cast<llvm::Constant>(lastValue))
                    {
                        constants.push_back(c);
                    }
                }
                // Use the correct hardware array type
                llvm::ArrayType *arrTy = llvm::ArrayType::get(builder.getInt32Ty(), constants.size());
                initializer = llvm::ConstantArray::get(arrTy, constants);
                valType = arrTy; // Ensure the global variable uses the raw array type [3 x i32]
            }
            else
            {
                initializer = llvm::Constant::getNullValue(valType);
            }

            auto *gVar = new llvm::GlobalVariable(
                *module, valType, node.isConst,
                llvm::GlobalValue::ExternalLinkage, initializer, node.name.lexeme);

            if (node.alignment > 0)
                gVar->setAlignment(llvm::MaybeAlign(node.alignment));
            if (!node.section.empty())
                gVar->setSection(node.section);
            namedValues[node.name.lexeme] = gVar;
            return;
        }

        if (node.isVolatile)
        {
            volatileVars.insert(node.name.lexeme);
        }

        // 1. Handle EXTERN (Global Scope Linkage)
        if (node.isExtern)
        {
            llvm::GlobalVariable *gVar = new llvm::GlobalVariable(
                *module, valType, node.isConst,
                llvm::GlobalValue::ExternalLinkage, nullptr, node.name.lexeme);

            if (node.alignment > 0)
                gVar->setAlignment(llvm::MaybeAlign(node.alignment));
            if (!node.section.empty())
                gVar->setSection(node.section);

            namedValues[node.name.lexeme] = gVar;
            return; // Externs have no initializer or stack allocation
        }

        // 2. Handle Stack Allocation with Alignment
        llvm::AllocaInst *alloca = CreateEntryBlockAlloca(currentFunction, node.name.lexeme, valType);
        if (node.alignment > 0)
        {
            alloca->setAlignment(llvm::Align(node.alignment));
        }

        std::shared_ptr<Type> declaredType = node.type->resolvedType;
        if (!declaredType)
            declaredType = std::make_shared<IntType>();

        bool isFixedArray = false;
        if (auto arrType = std::dynamic_pointer_cast<ArrayType>(declaredType))
        {
            if (arrType->isFixedSize)
            {
                isFixedArray = true;
            }
        }

        if (std::dynamic_pointer_cast<StructType>(node.type->resolvedType))
        {
            alloca = CreateEntryBlockAlloca(currentFunction, node.name.lexeme, valType);
        }

        llvm::Value *initVal = nullptr;
        // 1. Evaluate Initializer
        if (node.initializer)
        {
            node.initializer->accept(*this);
            initVal = lastValue;
            initVal = emitCast(builder, module.get(), initVal, node.initializer->resolvedType, declaredType);

            if (auto cls = std::dynamic_pointer_cast<ClassInstanceType>(declaredType))
            {
                bool shouldClone = !isRefClassMap[cls->className] && !node.isShared;

                if (shouldClone)
                {
                    initVal = cloneObject(initVal, cls->className);
                }
            }
        }
        else if (!node.type->dimensions.empty())
        {
            if (isFixedArray)
            {
                // For Fixed-Size Arrays (int arr[5]), we do NOT allocate a heap array.
                uint64_t sizeBytes = module->getDataLayout().getTypeAllocSize(valType);
                builder.CreateMemSet(alloca, builder.getInt8(0), sizeBytes, llvm::MaybeAlign(node.alignment));
            }
            else
            {
                // For Dynamic Arrays (int[] arr), we create the Heap Object
                std::shared_ptr<Type> leafType = node.type->resolvedType;
                while (auto arrType = std::dynamic_pointer_cast<ArrayType>(leafType))
                {
                    leafType = arrType->elementType;
                }

                // Create BOXED Default Value for the array fill
                llvm::Value *leafDefault = nullptr;

                if (std::dynamic_pointer_cast<IntType>(leafType))
                {
                    llvm::Value *zero = builder.getInt32(0);
                    leafDefault = builder.CreateCall(module->getFunction("moksha_box_int"), {zero});
                }
                else if (std::dynamic_pointer_cast<DoubleType>(leafType))
                {
                    llvm::Value *zero = llvm::ConstantFP::get(context, llvm::APFloat(0.0));
                    leafDefault = builder.CreateCall(module->getFunction("moksha_box_double"), {zero});
                }
                else if (std::dynamic_pointer_cast<BooleanType>(leafType))
                {
                    llvm::Value *zero = builder.getInt32(0);
                    leafDefault = builder.CreateCall(module->getFunction("moksha_box_bool"), {zero});
                }
                else
                {
                    leafDefault = llvm::ConstantPointerNull::get(builder.getPtrTy());
                }

                if (leafDefault->getType() != builder.getPtrTy())
                {
                    leafDefault = builder.CreateBitCast(leafDefault, builder.getPtrTy());
                }

                // Collect Dimensions
                std::vector<ExpressionNode *> allDims;
                TypeNode *curr = node.type.get();
                while (curr && !curr->dimensions.empty())
                {
                    for (auto &d : curr->dimensions)
                    {
                        allDims.push_back(d.get());
                    }
                    curr = curr->elementType.get();
                }
                std::reverse(allDims.begin(), allDims.end());

                initVal = generateArrayAllocation(allDims, 0, leafDefault);
            }
        }
        else
        {
            // Default Initialization
            initVal = llvm::Constant::getNullValue(valType);
        }

        namedValues[node.name.lexeme] = alloca;
        if (initVal)
        {
            // Safety check to prevent bad stores
            if (initVal->getType() != valType)
            {
                if (valType->isPointerTy() && initVal->getType()->isPointerTy())
                    initVal = builder.CreateBitCast(initVal, valType);
                // If trying to store Ptr into ArrayType ([5xi32]), SKIP IT
                else if (valType->isArrayTy() && initVal->getType()->isPointerTy())
                    goto skip_store;
            }
            builder.CreateStore(initVal, alloca);
        skip_store:;
        }

        if (node.isShared)
        {
            sharedVars.insert(node.name.lexeme);
            // Treat shared variables as Reference Types (Heap Allocated)
            node.isHeapAllocated = true;
        }

        // --- HEAP PROMOTION LOGIC ---
        if (node.isHeapAllocated)
        {
            // 1. Create a pointer on the stack to hold the address of the Heap Slot (Box)
            alloca = CreateEntryBlockAlloca(currentFunction, node.name.lexeme, llvm::PointerType::get(context, 0));

            // ALLOCATION STRATEGY:
            const llvm::DataLayout &dl = module->getDataLayout();
            uint64_t size = dl.getTypeAllocSize(valType); // e.g., 8 bytes for a pointer
            llvm::Function *mallocFunc = module->getFunction("malloc");

            // Allocate the Box (Heap Slot)
            llvm::Value *heapMem = builder.CreateCall(llvm::FunctionCallee(mallocFunc), {builder.getInt64(size)}, "heap_var");

            // Store the Box Address in the Stack Variable
            builder.CreateStore(heapMem, alloca);

            // Initialize the value INSIDE the Box
            if (initVal)
                builder.CreateStore(initVal, heapMem);

            heapAllocatedVars.insert(node.name.lexeme);
        }
        else
        {
            // Standard Stack Allocation
            alloca = CreateEntryBlockAlloca(currentFunction, node.name.lexeme, valType);
            if (initVal)
                builder.CreateStore(initVal, alloca);
        }

        namedValues[node.name.lexeme] = alloca;
        if (auto cls = std::dynamic_pointer_cast<ClassInstanceType>(declaredType))
        {
            variableTypes[node.name.lexeme] = cls->className;
        }
    }

    void CodeGenerator::visit(FunctionDefinitionNode &node)
    {
        llvm::BasicBlock *previousBlock = builder.GetInsertBlock();
        auto oldNamedValues = namedValues;
        auto oldVariableTypes = variableTypes;

        globalFunctionDefs[node.name.lexeme] = &node;

        std::vector<llvm::Type *> argTypes;

        // 1. Handle Implicit 'this' for Methods
        bool isMethod = !currentClassName.empty();
        if (isMethod)
        {
            llvm::StructType *st = llvm::StructType::getTypeByName(context, currentClassName);
            if (st)
                argTypes.push_back(llvm::PointerType::get(context, 0));
            else
                argTypes.push_back(builder.getPtrTy());
        }

        // 2. Standard Arguments
        for (const auto &p : node.parameters)
        {
            // --- Check if parameter has a type annotation ---
            llvm::Type *t = nullptr;
            if (p->isRef)
            {
                t = builder.getPtrTy();
            }
            else if (p->type)
            {
                t = getLLVMType(p->type->resolvedType, context, module.get());
            }
            else
            {
                t = builder.getPtrTy();
            }
            argTypes.push_back(t);
        }

        // 3. Create Function
        std::shared_ptr<Type> resolvedRetType = node.returnType ? node.returnType->resolvedType : std::make_shared<VoidType>();
        llvm::Type *retType = getLLVMType(resolvedRetType, context, module.get());
        llvm::FunctionType *ft = llvm::FunctionType::get(retType, argTypes, node.isVariadic);

        std::string funcName = node.name.lexeme;
        if (isMethod && funcName != "main")
        {
            funcName = currentClassName + "_" + funcName;
        }

        llvm::Function *function = module->getFunction(funcName);
        if (!function)
        {
            auto linkage = node.isExtern ? llvm::Function::ExternalLinkage : llvm::Function::ExternalLinkage;
            function = llvm::Function::Create(ft, linkage, funcName, module.get());
        }

        if (node.isInterrupt)
        {
            function->setCallingConv(llvm::CallingConv::X86_INTR);

            if (function->arg_size() > 0)
            {
                llvm::Argument *firstArg = function->arg_begin();
                llvm::Type *frameType = builder.getInt64Ty(); // Default fallback
                firstArg->addAttr(llvm::Attribute::getWithByValType(context, frameType));
            }
        }

        if (!node.section.empty())
        {
            function->setSection(node.section);
        }

        if (node.isExtern || !node.body)
            return;

        // 4. Setup Body
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "entry", function);
        builder.SetInsertPoint(bb);

        unsigned argIdx = 0;

        if (isMethod)
        {
            llvm::Argument *thisArg = function->getArg(argIdx++);
            thisArg->setName("this");
            currentThis = thisArg;
        }
        else
        {
            currentThis = nullptr;
        }

        if (isMethod && node.name.lexeme == "constructor" && classFieldInitializers.count(currentClassName))
        {
            llvm::StructType *st = llvm::StructType::getTypeByName(context, currentClassName);

            for (auto *varDecl : classFieldInitializers[currentClassName])
            {
                if (varDecl->initializer)
                {
                    // Evaluate the expression
                    varDecl->initializer->accept(*this);
                    llvm::Value *initVal = lastValue;

                    // Cast/Clone logic
                    initVal = emitCast(builder, module.get(), initVal, varDecl->initializer->resolvedType, varDecl->type->resolvedType);
                    if (auto cls = std::dynamic_pointer_cast<ClassInstanceType>(varDecl->type->resolvedType))
                    {
                        if (!isRefClassMap[cls->className])
                        {
                            initVal = cloneObject(initVal, cls->className);
                        }
                    }

                    // Store
                    int idx = classFieldIndices[currentClassName][varDecl->name.lexeme];
                    llvm::Value *fieldPtr = builder.CreateStructGEP(st, currentThis, idx);

                    llvm::Type *fieldTy = st->getElementType(idx);
                    if (initVal->getType() != fieldTy)
                    {
                        if (initVal->getType()->isPointerTy() && fieldTy->isPointerTy())
                            initVal = builder.CreateBitCast(initVal, fieldTy);
                    }
                    builder.CreateStore(initVal, fieldPtr);
                }
            }
        }

        for (auto &param : node.parameters)
        {
            llvm::Argument *arg = function->getArg(argIdx++);
            arg->setName(param->name.lexeme);

            if (param->isRef)
            {
                namedValues[param->name.lexeme] = arg;
            }
            else
            {
                llvm::AllocaInst *alloca = CreateEntryBlockAlloca(function, param->name.lexeme, arg->getType());
                builder.CreateStore(arg, alloca);
                namedValues[param->name.lexeme] = alloca;
            }

            // Safe check for type
            if (param->type && param->type->resolvedType)
            {
                if (auto cls = std::dynamic_pointer_cast<ClassInstanceType>(param->type->resolvedType))
                {
                    variableTypes[param->name.lexeme] = cls->className;
                }
            }
        }

        if (node.body)
            node.body->accept(*this);

        if (!builder.GetInsertBlock()->getTerminator())
        {
            if (retType->isVoidTy())
                builder.CreateRetVoid();
            else
                builder.CreateRet(llvm::Constant::getNullValue(retType));
        }

        namedValues = oldNamedValues;
        variableTypes = oldVariableTypes;
        currentThis = nullptr;
        if (previousBlock)
            builder.SetInsertPoint(previousBlock);
    }

    void CodeGenerator::visit(EnumDefinitionNode &node)
    {
        int nextVal = 0;
        for (auto &member : node.members)
        {
            if (member.second)
            {
                // Has initializer (e.g., A = 5)
                member.second->accept(*this);
                if (auto c = llvm::dyn_cast<llvm::ConstantInt>(lastValue))
                {
                    nextVal = c->getSExtValue();
                }
            }

            // Register in Scoped Map (Status -> Idle -> 0)
            scopedEnumConstants[node.name.lexeme][member.first.lexeme] = nextVal;

            // Optional: Keep global map if you still want C-Style 'Idle' access
            enumConstants[member.first.lexeme] = nextVal;

            nextVal++;
        }
        // No runtime code needed for Enum def
        lastValue = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
    }

    bool isAnyType(std::shared_ptr<Type> t)
    {
        return std::dynamic_pointer_cast<AnyType>(t) != nullptr;
    }

    void CodeGenerator::visit(TypeCastNode &node)
    {
        node.expression->accept(*this);
        llvm::Value *val = lastValue;
        llvm::Type *destType = getLLVMType(node.targetType->resolvedType, context, module.get());

        // [FIX] Use high-level types to distinguish Raw Pointers from Objects (Any, String, etc.)
        std::shared_ptr<Type> srcMokshaType = node.expression->resolvedType;
        std::shared_ptr<Type> destMokshaType = node.targetType->resolvedType;

        // Check if they are strictly Raw Pointers (e.g. int*, char*)
        bool srcIsRawPtr = std::dynamic_pointer_cast<PointerType>(srcMokshaType) != nullptr;
        bool destIsRawPtr = std::dynamic_pointer_cast<PointerType>(destMokshaType) != nullptr;

        // Int -> Pointer (For OS/Embedded: (int*)0xB8000)
        // [FIX] Only allow if destination is explicitly a raw pointer
        if (val->getType()->isIntegerTy() && destType->isPointerTy() && destIsRawPtr)
        {
            bool isGlobal = builder.GetInsertBlock()->getParent()->getName() == "main";

            if (isGlobal)
            {
                if (auto *constInt = llvm::dyn_cast<llvm::ConstantInt>(val))
                {
                    lastValue = llvm::ConstantExpr::getIntToPtr(constInt, destType);
                    return;
                }
            }

            if (!insideUnsafe && !isGlobal)
            {
                std::cerr << "Error: Int-to-Pointer cast allowed only in 'unsafe' blocks.\n";
                return;
            }
            lastValue = builder.CreateIntToPtr(val, destType, "int_to_ptr");
            return;
        }

        // Pointer -> Int (For Pointer Arithmetic/Debug)
        // [FIX] Only allow if source is explicitly a raw pointer
        if (val->getType()->isPointerTy() && destType->isIntegerTy() && srcIsRawPtr)
        {
            if (!insideUnsafe)
            {
                std::cerr << "Error: Pointer-to-Int cast allowed only in 'unsafe' blocks.\n";
                return;
            }
            lastValue = builder.CreatePtrToInt(val, destType, "ptr_to_int");
            return;
        }

        // Fallthrough for Any, String, Primitives (Handled by Runtime)
        lastValue = emitCast(builder, module.get(), val, node.expression->resolvedType, node.targetType->resolvedType);
    }

    void CodeGenerator::visit(PrintStatementNode &node)
    {
        if (isFreestanding)
        {
            std::cerr << "[Error] 'print' statement is disabled in freestanding mode. Use a VGA driver or 'outb'.\n";
            return;
        }

        node.expression->accept(*this);
        llvm::Value *val = lastValue;
        llvm::Function *printfFunc = module->getFunction("printf");
        bool isNewLine = (node.variant.type == TokenType::KW_PRINTLN);
        std::string suffix = isNewLine ? "\n" : "";
        llvm::Value *formatStr;

        std::shared_ptr<Type> type = node.expression->resolvedType;

        if (val->getType()->isPointerTy() && (!type || !type->isNullable))
        {
            if (std::dynamic_pointer_cast<IntType>(type))
            {
                val = builder.CreateCall(module->getFunction("moksha_unbox_int"), {val});
            }
            else if (std::dynamic_pointer_cast<DoubleType>(type))
            {
                val = builder.CreateCall(module->getFunction("moksha_unbox_double"), {val});
            }
            else if (std::dynamic_pointer_cast<BooleanType>(type))
            {
                val = builder.CreateCall(module->getFunction("moksha_unbox_bool"), {val});
            }
        }

        bool isAny = isAnyType(type);
        bool isArray = std::dynamic_pointer_cast<ArrayType>(type) != nullptr;
        bool isTable = std::dynamic_pointer_cast<TableType>(type) != nullptr;
        bool isBool = std::dynamic_pointer_cast<BooleanType>(type) != nullptr;
        bool isBoxedPrimitive = false;

        if (val->getType()->isPointerTy())
        {
            if (std::dynamic_pointer_cast<IntType>(type) ||
                std::dynamic_pointer_cast<DoubleType>(type) ||
                std::dynamic_pointer_cast<BooleanType>(type))
            {
                isBoxedPrimitive = true;
            }
        }

        if (isAny || isArray || isTable || isBool || isBoxedPrimitive || (type && type->isNullable))
        {
            // Box primitives if we have a raw primitive value (e.g. printing a literal)
            if (val->getType()->isIntegerTy(32))
                val = builder.CreateCall(module->getFunction("moksha_box_int"), {val});
            else if (val->getType()->isDoubleTy())
                val = builder.CreateCall(module->getFunction("moksha_box_double"), {val});
            else if (val->getType()->isIntegerTy(1))
            {
                val = builder.CreateZExt(val, builder.getInt32Ty());
                val = builder.CreateCall(module->getFunction("moksha_box_bool"), {val});
            }
            else if (val->getType()->isFloatTy())
            {
                val = builder.CreateFPExt(val, builder.getDoubleTy(), "promote_f2d");
                val = builder.CreateCall(module->getFunction("moksha_box_double"), {val});
            }

            if (val->getType() != builder.getPtrTy())
                val = builder.CreateBitCast(val, builder.getPtrTy());

            llvm::Function *printFunc = module->getFunction("moksha_print_val");
            builder.CreateCall(printFunc, {val, builder.getInt32(isNewLine)});
            return;
        }
        else if (val->getType()->isFloatingPointTy())
        {
            // Printf expects doubles. Promote float -> double
            if (val->getType()->isFloatTy())
            {
                val = builder.CreateFPExt(val, builder.getDoubleTy(), "float_to_double");
            }
            formatStr = builder.CreateGlobalString("%f" + suffix);
        }
        else if (val->getType()->isPointerTy())
        {
            // 1. Handle Moksha Strings (Objects)
            if (std::dynamic_pointer_cast<StringType>(type))
            {
                llvm::Function *unboxFunc = module->getFunction("moksha_unbox_string");
                val = builder.CreateCall(llvm::FunctionCallee(unboxFunc), {val}, "print_unbox");
                formatStr = builder.CreateGlobalString("%s" + suffix);
            }
            // 2. [FIX] Handle Raw Char Pointers (char*)
            else if (auto ptrType = std::dynamic_pointer_cast<PointerType>(type))
            {
                if (std::dynamic_pointer_cast<CharType>(ptrType->pointeeType))
                {
                    // It is 'char*', so it's safe to print as a C-string
                    formatStr = builder.CreateGlobalString("%s" + suffix);
                }
                else
                {
                    // Other pointers (int*, void*) -> Print Address
                    formatStr = builder.CreateGlobalString("%p" + suffix);
                }
            }
            // 3. Fallback for Classes/Unknown
            else
            {
                llvm::Function *func = builder.GetInsertBlock()->getParent();
                llvm::BasicBlock *nullBB = llvm::BasicBlock::Create(context, "print_null", func);
                llvm::BasicBlock *objBB = llvm::BasicBlock::Create(context, "print_obj", func);
                llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "print_merge", func);

                // Check if pointer is null
                llvm::Value *isNull = builder.CreateIsNull(val, "is_obj_null");
                builder.CreateCondBr(isNull, nullBB, objBB);

                // Case: Null
                builder.SetInsertPoint(nullBB);
                llvm::Value *nullFmt = builder.CreateGlobalString("null" + suffix);
                builder.CreateCall(printfFunc, {nullFmt});
                builder.CreateBr(mergeBB);

                // Case: Object (Print Address)
                builder.SetInsertPoint(objBB);
                formatStr = builder.CreateGlobalString("<Object %p>" + suffix);
                builder.CreateCall(printfFunc, {formatStr, val});
                builder.CreateBr(mergeBB);

                builder.SetInsertPoint(mergeBB);
                return; // Return early as we handled the calls manually
            }
        }
        else
        {
            formatStr = builder.CreateGlobalString("%d" + suffix);
        }
        builder.CreateCall(printfFunc, {formatStr, val});
    }

    void CodeGenerator::visit(ReturnNode &node)
    {
        // 1. Execute Deferred Statements
        for (auto it = deferStack.rbegin(); it != deferStack.rend(); ++it)
        {
            StatementNode *s = const_cast<StatementNode *>(*it);
            s->accept(*this);
        }

        if (node.value)
        {
            node.value->accept(*this);
            llvm::Value *retVal = lastValue;

            // Get the expected return type of the function we are currently generating
            llvm::Function *parentFunc = builder.GetInsertBlock()->getParent();
            llvm::Type *funcRetType = parentFunc->getReturnType();

            // ---------------------------------------------------------
            // CASE A: BOXING (Primitive -> Pointer)
            // Expected: Any/Object (Ptr) | Actual: Int/Double/Bool
            // ---------------------------------------------------------
            if (funcRetType->isPointerTy() && !retVal->getType()->isPointerTy())
            {
                if (retVal->getType()->isIntegerTy(32))
                    retVal = builder.CreateCall(module->getFunction("moksha_box_int"), {retVal});
                else if (retVal->getType()->isDoubleTy())
                    retVal = builder.CreateCall(module->getFunction("moksha_box_double"), {retVal});
                else if (retVal->getType()->isIntegerTy(1))
                {
                    llvm::Value *b = builder.CreateZExt(retVal, builder.getInt32Ty());
                    retVal = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                }
            }
            // ---------------------------------------------------------
            // CASE B: UNBOXING (Pointer -> Primitive)
            // Expected: Int/Double/Bool | Actual: Any/Object (Ptr)
            // ---------------------------------------------------------
            else if (!funcRetType->isPointerTy() && retVal->getType()->isPointerTy())
            {
                if (funcRetType->isIntegerTy(32))
                {
                    retVal = builder.CreateCall(module->getFunction("moksha_unbox_int"), {retVal});
                }
                else if (funcRetType->isDoubleTy())
                {
                    retVal = builder.CreateCall(module->getFunction("moksha_unbox_double"), {retVal});
                }
                else if (funcRetType->isIntegerTy(1)) // Bool
                {
                    // Unbox returns i32, so we trunc to i1
                    llvm::Value *unboxed = builder.CreateCall(module->getFunction("moksha_unbox_bool"), {retVal});
                    retVal = builder.CreateTrunc(unboxed, builder.getInt1Ty());
                }
            }

            // ---------------------------------------------------------
            // CASE C: TYPE CASTING (Safety Fallback)
            // e.g. String* -> i8*, or i32 -> i64
            // ---------------------------------------------------------
            if (retVal->getType() != funcRetType)
            {
                if (retVal->getType()->isPointerTy() && funcRetType->isPointerTy())
                {
                    retVal = builder.CreateBitCast(retVal, funcRetType);
                }
                else if (retVal->getType()->isIntegerTy() && funcRetType->isIntegerTy())
                {
                    retVal = builder.CreateIntCast(retVal, funcRetType, true); // Signed cast
                }
                // If we are here, and types still don't match, LLVM will likely verify-error.
                // This usually means trying to return a String in an Int function without unboxing logic possible.
            }

            builder.CreateRet(retVal);
        }
        else
        {
            // Handle 'return;' (Void or Null)
            llvm::Type *retType = builder.GetInsertBlock()->getParent()->getReturnType();
            if (retType->isVoidTy())
            {
                builder.CreateRetVoid();
            }
            else
            {
                // If the function expects a value but we return nothing, return null/zero
                builder.CreateRet(llvm::Constant::getNullValue(retType));
            }
        }
    }

    void CodeGenerator::visit(DeleteStatementNode &node)
    {
        // Check if we are deleting a property: delete(obj[key])
        if (auto indexNode = dynamic_cast<IndexNode *>(node.target.get()))
        {
            // Evaluate the Table/Object
            indexNode->callee->accept(*this);
            llvm::Value *target = lastValue;

            // Evaluate the Key
            indexNode->index->accept(*this);
            llvm::Value *idx = lastValue;

            // Check if it is a Table
            if (std::dynamic_pointer_cast<TableType>(indexNode->callee->resolvedType))
            {
                // BOX THE KEY (Tables use void* keys)
                if (idx->getType()->isIntegerTy(32))
                    idx = builder.CreateCall(module->getFunction("moksha_box_int"), {idx});
                else if (idx->getType()->isDoubleTy())
                    idx = builder.CreateCall(module->getFunction("moksha_box_double"), {idx});
                else if (idx->getType()->isIntegerTy(1))
                {
                    llvm::Value *b = builder.CreateZExt(idx, builder.getInt32Ty());
                    idx = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                }
                else if (idx->getType()->isPointerTy())
                {
                    if (idx->getType() != builder.getPtrTy())
                    {
                        idx = builder.CreateBitCast(idx, builder.getPtrTy());
                    }
                }

                // Call the specific Table Delete function
                builder.CreateCall(module->getFunction("moksha_table_delete"), {target, idx});
                return;
            }
            else if (std::dynamic_pointer_cast<ArrayType>(indexNode->callee->resolvedType))
            {
                // Ensure index is i32
                if (idx->getType()->isDoubleTy())
                    idx = builder.CreateFPToSI(idx, builder.getInt32Ty());
                if (!idx->getType()->isIntegerTy(32))
                    idx = builder.CreateZExtOrTrunc(idx, builder.getInt32Ty());

                // Call the runtime array delete
                builder.CreateCall(module->getFunction("moksha_array_delete"), {target, idx});
                return;
            }
        }

        // 2. Default: Object Deletion (delete(var))
        node.target->accept(*this);
        llvm::Value *ptr = lastValue;

        // --- NEW: Call Destructor if it exists ---
        if (auto clsType = std::dynamic_pointer_cast<ClassInstanceType>(node.target->resolvedType))
        {
            // Mangled Name: ClassName_destructor (e.g., Vector_destructor)
            std::string dtorName = clsType->className + "_destructor";

            if (llvm::Function *dtor = module->getFunction(dtorName))
            {
                // Ensure pointer type matches (Class* vs void*)
                llvm::Value *objPtr = ptr;
                if (ptr->getType() != builder.getPtrTy())
                {
                    objPtr = builder.CreateBitCast(ptr, builder.getPtrTy());
                }
                builder.CreateCall(dtor, {objPtr});
            }
        }

        // 3. Free Memory
        llvm::Function *freeFunc = module->getFunction("moksha_free");

        // Ensure pointer is void*
        if (ptr->getType() != builder.getPtrTy())
        {
            ptr = builder.CreateBitCast(ptr, builder.getPtrTy());
        }

        builder.CreateCall(llvm::FunctionCallee(freeFunc), {ptr});
    }

    void CodeGenerator::visit(BlockStatementNode &node)
    {
        auto oldNamedValues = namedValues;
        auto oldVariableTypes = variableTypes;

        std::vector<std::string> localsToFree;

        for (auto &s : node.statements)
        {
            if (s)
            {
                // Identify Reference Types declared in this scope
                if (auto varDecl = dynamic_cast<VariableDeclarationNode *>(s.get()))
                {
                    std::shared_ptr<Type> t = varDecl->type->resolvedType;
                    // Check if type is a Heap Object (Array, Table, String, or Class)
                    bool isRef = std::dynamic_pointer_cast<ArrayType>(t) ||
                                 std::dynamic_pointer_cast<TableType>(t) ||
                                 std::dynamic_pointer_cast<StringType>(t) ||
                                 std::dynamic_pointer_cast<ClassInstanceType>(t);

                    // Externs and Shared vars have different lifecycles
                    if (isRef && !varDecl->isExtern && !varDecl->isShared)
                    {
                        localsToFree.push_back(varDecl->name.lexeme);
                    }
                }

                s->accept(*this);

                // [NEW] FAIL-FAST CHECK
                if (!builder.GetInsertBlock()->getTerminator())
                {
                    llvm::Function *func = builder.GetInsertBlock()->getParent();
                    llvm::BasicBlock *nextStmtBB = llvm::BasicBlock::Create(context, "next_stmt", func);
                    llvm::BasicBlock *unwindBB = llvm::BasicBlock::Create(context, "unwind", func);

                    llvm::Value *flag = builder.CreateLoad(builder.getInt32Ty(), globalExceptionFlag);
                    llvm::Value *hasError = builder.CreateICmpNE(flag, builder.getInt32(0));

                    builder.CreateCondBr(hasError, unwindBB, nextStmtBB);

                    // Unwind Path
                    builder.SetInsertPoint(unwindBB);
                    if (!exceptionStack.empty())
                    {
                        // Jump to local Try-Catch handler
                        builder.CreateBr(exceptionStack.top());
                    }
                    else
                    {
                        // No local handler? Return immediately (Bubble up to caller)
                        llvm::Type *retType = func->getReturnType();
                        if (retType->isVoidTy())
                            builder.CreateRetVoid();
                        else
                            builder.CreateRet(llvm::Constant::getNullValue(retType));
                    }

                    // Continue Path
                    builder.SetInsertPoint(nextStmtBB);
                }
            }
        }

        llvm::Function *freeFunc = module->getFunction("moksha_free");

        for (auto it = localsToFree.rbegin(); it != localsToFree.rend(); ++it)
        {
            std::string name = *it;
            if (namedValues.count(name))
            {
                llvm::Value *stackSlot = namedValues[name];
                if (stackSlot->getType()->isPointerTy())
                {
                    // Assuming opaque pointers (LLVM 15+), simply load ptr
                    llvm::Value *heapPtr = builder.CreateLoad(builder.getPtrTy(), stackSlot);

                    // Call moksha_free(heapPtr)
                    builder.CreateCall(freeFunc, {heapPtr});
                }
            }
        }

        namedValues = oldNamedValues;
        variableTypes = oldVariableTypes;
    }

    void CodeGenerator::visit(ExpressionStatementNode &node)
    {
        if (node.expression)
            node.expression->accept(*this);
    }

    void CodeGenerator::visit(DeferStatementNode &node)
    {
        deferStack.push_back(node.statement.get());
    }

    void CodeGenerator::visit(ThrowStatementNode &node)
    {
        if (isFreestanding)
        {
            std::cerr << "[Error] 'print' statement is disabled in freestanding mode. Use a VGA driver or 'outb'.\n";
            return;
        }

        node.expression->accept(*this);
        llvm::Value *exceptionVal = lastValue;

        // 1. Call Runtime Throw (Sets up internal state if needed)
        llvm::Function *throwFunc = module->getFunction("moksha_throw_exception");
        llvm::Value *voidPtr = builder.CreateBitCast(exceptionVal, llvm::PointerType::get(context, 0));
        builder.CreateCall(llvm::FunctionCallee(throwFunc), {voidPtr});

        // 2. Set Global Flags manually (Compulsory for your Control Flow check)
        builder.CreateStore(exceptionVal, globalExceptionVal);
        builder.CreateStore(builder.getInt32(1), globalExceptionFlag);

        // 3. [FIX] Unwind Immediately (Don't use Unreachable)
        if (!exceptionStack.empty())
        {
            // If inside a Try-Catch, jump directly to the handler
            builder.CreateBr(exceptionStack.top());
        }
        else
        {
            // If function scope, return immediately to bubble up
            llvm::Function *parent = builder.GetInsertBlock()->getParent();
            llvm::Type *retType = parent->getReturnType();

            if (retType->isVoidTy())
            {
                builder.CreateRetVoid();
            }
            else
            {
                builder.CreateRet(llvm::Constant::getNullValue(retType));
            }
        }

        // Create a dead block so builder has a place to sit if it continues generation
        llvm::BasicBlock *deadBB = llvm::BasicBlock::Create(context, "dead_code", builder.GetInsertBlock()->getParent());
        builder.SetInsertPoint(deadBB);
    }

    void CodeGenerator::visit(TryStatementNode &node)
    {
        if (isFreestanding)
        {
            std::cerr << "[Error] 'print' statement is disabled in freestanding mode. Use a VGA driver or 'outb'.\n";
            return;
        }

        llvm::Function *func = builder.GetInsertBlock()->getParent();

        llvm::BasicBlock *tryBB = llvm::BasicBlock::Create(context, "try_block", func);
        llvm::BasicBlock *checkBB = llvm::BasicBlock::Create(context, "exception_check", func);

        llvm::BasicBlock *catchBB = node.catchBlock ? llvm::BasicBlock::Create(context, "catch_block", func) : nullptr;
        llvm::BasicBlock *finallyBB = node.finallyBlock ? llvm::BasicBlock::Create(context, "finally_block", func) : nullptr;
        llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "try_cont", func);

        llvm::BasicBlock *effectiveFinallyBB = finallyBB ? finallyBB : afterBB;

        // Target to jump to if exception occurs inside TRY
        llvm::BasicBlock *errorTargetBB = catchBB ? catchBB : effectiveFinallyBB;

        // 1. Generate TRY Block
        builder.CreateBr(tryBB);
        builder.SetInsertPoint(tryBB);

        // [NEW] Push the handler target
        exceptionStack.push(errorTargetBB);

        if (node.tryBlock)
            node.tryBlock->accept(*this);

        // [NEW] Pop after finishing try block generation
        exceptionStack.pop();

        // Jump to check (only if not already returned/jumped)
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(checkBB);

        // 2. Generate CHECK Block (For uncaught exceptions bubbling up)
        builder.SetInsertPoint(checkBB);
        llvm::Value *flag = builder.CreateLoad(builder.getInt32Ty(), globalExceptionFlag);
        llvm::Value *hasError = builder.CreateICmpNE(flag, builder.getInt32(0));
        builder.CreateCondBr(hasError, errorTargetBB, effectiveFinallyBB);

        // 3. Generate CATCH Block
        if (catchBB)
        {
            builder.SetInsertPoint(catchBB);

            // --- SCOPE FIX START ---
            auto oldNamedValues = namedValues;
            auto oldVariableTypes = variableTypes;
            // -----------------------

            // Reset Flag (Critical Fix for Segfault)
            builder.CreateStore(builder.getInt32(0), globalExceptionFlag);

            // Handle Exception Variable (e.g., catch(e))
            if (!node.errorVariable.lexeme.empty())
            {
                llvm::Value *errVal = builder.CreateLoad(builder.getPtrTy(), globalExceptionVal);
                llvm::AllocaInst *errVar = CreateEntryBlockAlloca(func, node.errorVariable.lexeme, builder.getPtrTy());
                builder.CreateStore(errVal, errVar);

                namedValues[node.errorVariable.lexeme] = errVar;
            }

            if (node.catchBlock)
                node.catchBlock->accept(*this);

            // --- SCOPE FIX END ---
            namedValues = oldNamedValues;
            variableTypes = oldVariableTypes;
            // ---------------------

            if (!builder.GetInsertBlock()->getTerminator())
                builder.CreateBr(effectiveFinallyBB);
        }

        // 4. Generate FINALLY Block
        if (finallyBB)
        {
            builder.SetInsertPoint(finallyBB);
            if (node.finallyBlock)
                node.finallyBlock->accept(*this);
            if (!builder.GetInsertBlock()->getTerminator())
                builder.CreateBr(afterBB);
        }

        // 5. Continue
        builder.SetInsertPoint(afterBB);
    }

    void CodeGenerator::visit(WithStatementNode &node)
    {
        if (node.declaration)
            node.declaration->accept(*this);
        if (node.body)
            node.body->accept(*this);
    }

    void CodeGenerator::visit(LiteralNode &node)
    {
        if (node.token.type == TokenType::INTEGER_LITERAL)
        {
            try
            {
                long long val = std::stoll(node.token.lexeme, nullptr, 0);
                lastValue = llvm::ConstantInt::get(context, llvm::APInt(32, (uint64_t)val, false));
            }
            catch (...)
            {
                std::cerr << "CodeGen Error: Failed to parse integer '" << node.token.lexeme << "'\n";
                lastValue = builder.getInt32(0);
            }
        }
        else if (node.token.type == TokenType::FLOAT_LITERAL)
        {
            lastValue = llvm::ConstantFP::get(context, llvm::APFloat(std::stod(node.token.lexeme)));
        }
        else if (node.token.type == TokenType::BOOLEAN_LITERAL)
        {
            lastValue = llvm::ConstantInt::get(context, llvm::APInt(1, (node.token.lexeme == "true"), false));
        }
        else if (node.token.type == TokenType::STRING_LITERAL || node.token.type == TokenType::STRING_CHUNK)
        {
            llvm::Value *rawStr = builder.CreateGlobalString(node.token.lexeme);

            if (isFreestanding)
            {
                lastValue = rawStr; // Return raw char* in kernel mode
            }
            else
            {
                // 1. Generate a unique name for this literal's cache variable
                static int strLitId = 0;
                std::string globalCacheName = "__str_lit_cache_" + std::to_string(strLitId++);

                // 2. Create a global variable: static void* cache = NULL;
                module->getOrInsertGlobal(globalCacheName, builder.getPtrTy());
                llvm::GlobalVariable *gCache = module->getNamedGlobal(globalCacheName);
                gCache->setLinkage(llvm::GlobalValue::InternalLinkage); // Private to this module
                gCache->setInitializer(llvm::ConstantPointerNull::get(builder.getPtrTy()));

                // 3. Get or Declare the runtime helper function
                llvm::Function *getStaticFunc = module->getFunction("moksha_get_static_string");
                if (!getStaticFunc)
                {
                    llvm::FunctionType *ft = llvm::FunctionType::get(
                        builder.getPtrTy(),                       // Returns: void* (String Object)
                        {builder.getPtrTy(), builder.getPtrTy()}, // Args: (void** cachePtr, char* content)
                        false);
                    getStaticFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "moksha_get_static_string", module.get());
                }

                // 4. Call helper: moksha_get_static_string(&cache, rawStr)
                // This will check 'cache'. If null, it allocates. If not null, it returns the cached value.
                lastValue = builder.CreateCall(getStaticFunc, {gCache, rawStr});
            }
        }
        else if (node.token.type == TokenType::CHAR_LITERAL)
        {
            char c = node.token.lexeme.empty() ? 0 : node.token.lexeme[0];
            lastValue = builder.getInt8(c);
        }
        else if (node.token.type == TokenType::KW_NULL)
        {
            lastValue = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
        }
        else
        {
            // --- THE TRAP ---
            // If we hit this, it means the token type is wrong.
            std::cerr << "[[CRITICAL ERROR]] Unknown Literal Token: '" << node.token.lexeme
                      << "' (Enum Value: " << (int)node.token.type << ")\n";
            lastValue = llvm::ConstantInt::get(context, llvm::APInt(32, 0, true));
        }
    }

    void CodeGenerator::visit(VariableNode &node)
    {
        // 1. Handle Enum Constants (Pure Values)
        if (enumConstants.count(node.name.lexeme))
        {
            lastValue = builder.getInt32(enumConstants[node.name.lexeme]);
            return;
        }

        // 2. Handle Named Variables (Stack, Heap, or Parameters)
        if (namedValues.count(node.name.lexeme))
        {
            llvm::Value *addr = namedValues[node.name.lexeme];
            llvm::Type *valType = getLLVMType(node.resolvedType, context, module.get());
            bool volatileAccess = isVolatile(node.name.lexeme);

            if (!wantAddress && valType->isArrayTy())
            {
                lastValue = builder.CreateBitCast(addr, builder.getPtrTy(), "array_decay");
                return;
            }

            if (heapAllocatedVars.count(node.name.lexeme))
            {
                llvm::Value *heapAddr = builder.CreateLoad(builder.getPtrTy(), addr, "heap_addr");
                lastValue = wantAddress ? heapAddr : builder.CreateLoad(valType, heapAddr, volatileAccess);
            }
            else
            {
                lastValue = wantAddress ? addr : builder.CreateLoad(valType, addr, volatileAccess);
            }
            return;
        }

        // 3. Handle Function Pointers
        if (llvm::Function *func = module->getFunction(node.name.lexeme))
        {
            lastValue = func;
            return;
        }

        // 4. Fallback Error Handling
        std::cerr << "Codegen Error: Unknown variable " << node.name.lexeme << "\n";
        lastValue = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
    }

    void CodeGenerator::visit(GroupingNode &node)
    {
        if (node.expression)
            node.expression->accept(*this);
    }

    void CodeGenerator::visit(BinaryOpNode &node)
    {
        // 1. Handle Assignment (=)
        if (node.op.type == TokenType::ASSIGN)
        {
            if (auto indexNode = dynamic_cast<IndexNode *>(node.left.get()))
            {
                bool isFixedArray = false;
                std::shared_ptr<Type> leftType = indexNode->callee->resolvedType;
                if (auto arr = std::dynamic_pointer_cast<ArrayType>(leftType))
                {
                    if (arr->isFixedSize)
                        isFixedArray = true;
                }

                if (isFixedArray)
                {
                    // 1. Evaluate Array Base (as Address)
                    bool oldWantAddr = wantAddress;
                    wantAddress = true; // Force getting the alloca
                    indexNode->callee->accept(*this);
                    llvm::Value *target = lastValue;
                    wantAddress = oldWantAddr;

                    // 2. Evaluate Index
                    indexNode->index->accept(*this);
                    llvm::Value *idx = lastValue;

                    // 3. Evaluate RHS (Value)
                    node.right->accept(*this);
                    llvm::Value *rhsVal = lastValue;

                    // 4. Store
                    auto arrType = std::dynamic_pointer_cast<ArrayType>(leftType);
                    llvm::Type *elemTy = getLLVMType(arrType->elementType, context, module.get());

                    if (!insideUnsafe)
                    {
                        llvm::Function *func = builder.GetInsertBlock()->getParent();
                        llvm::BasicBlock *boundsOK = llvm::BasicBlock::Create(context, "bounds_ok", func);
                        llvm::BasicBlock *boundsFail = llvm::BasicBlock::Create(context, "bounds_fail", func);

                        // Normalize index to i32
                        llvm::Value *idx32 = idx;
                        if (idx->getType()->isDoubleTy())
                            idx32 = builder.CreateFPToSI(idx, builder.getInt32Ty());
                        else if (idx->getType() != builder.getInt32Ty())
                            idx32 = builder.CreateIntCast(idx, builder.getInt32Ty(), true);

                        llvm::Value *sizeVal = builder.getInt32(arrType->fixedSize);

                        // Check: idx < 0 || idx >= size
                        llvm::Value *tooSmall = builder.CreateICmpSLT(idx32, builder.getInt32(0));
                        llvm::Value *tooBig = builder.CreateICmpSGE(idx32, sizeVal);
                        llvm::Value *outOfBounds = builder.CreateOr(tooSmall, tooBig);

                        builder.CreateCondBr(outOfBounds, boundsFail, boundsOK);

                        // FAIL BLOCK
                        builder.SetInsertPoint(boundsFail);
                        llvm::Function *boxString = module->getFunction("moksha_box_string");
                        llvm::Function *throwEx = module->getFunction("moksha_throw_exception");
                        llvm::Value *errMsg = builder.CreateGlobalString("IndexOutOfBoundsException: Array index out of range");
                        llvm::Value *boxedErr = builder.CreateCall(boxString, {errMsg});
                        builder.CreateCall(throwEx, {boxedErr});
                        builder.CreateStore(boxedErr, globalExceptionVal);
                        builder.CreateStore(builder.getInt32(1), globalExceptionFlag);

                        if (!exceptionStack.empty())
                            builder.CreateBr(exceptionStack.top());
                        else
                        {
                            // Safe return for void/int/ptr
                            llvm::Type *retTy = func->getReturnType();
                            if (retTy->isVoidTy())
                                builder.CreateRetVoid();
                            else
                                builder.CreateRet(llvm::Constant::getNullValue(retTy));
                        }

                        // OK BLOCK
                        builder.SetInsertPoint(boundsOK);
                    }

                    // Cast Array Ptr to Element Ptr (e.g., [2 x i1]* -> i1*)
                    if (target->getType() != builder.getPtrTy())
                    {
                        target = builder.CreateBitCast(target, builder.getPtrTy());
                    }

                    llvm::Value *ptr = builder.CreateGEP(elemTy, target, idx);

                    // Helper: Handle type mismatch (e.g. i32 vs i1) if any
                    if (rhsVal->getType() != elemTy)
                    {
                        if (rhsVal->getType()->isIntegerTy() && elemTy->isIntegerTy())
                            rhsVal = builder.CreateIntCast(rhsVal, elemTy, false);
                        else if (rhsVal->getType()->isDoubleTy() && elemTy->isIntegerTy())
                            rhsVal = builder.CreateFPToSI(rhsVal, elemTy);
                    }

                    builder.CreateStore(rhsVal, ptr);
                    lastValue = rhsVal;
                    return;
                }

                // A. Evaluate RHS
                node.right->accept(*this);
                llvm::Value *rhsVal = lastValue;

                // If we are assigning a Char (i8) to a String (ptr), we must box it.
                std::shared_ptr<Type> lhsType = indexNode->callee->resolvedType; // Simplified lookup
                if (std::dynamic_pointer_cast<ArrayType>(lhsType))
                {
                    lhsType = std::dynamic_pointer_cast<ArrayType>(lhsType)->elementType;
                }

                bool lhsIsString = std::dynamic_pointer_cast<StringType>(lhsType) != nullptr;
                bool rhsIsChar = rhsVal->getType()->isIntegerTy(8); // Char is i8

                if (lhsIsString && rhsIsChar)
                {
                    llvm::Function *func = builder.GetInsertBlock()->getParent();

                    // 1. Create stack buffer [2 x i8] for "c\0"
                    llvm::Type *bufType = llvm::ArrayType::get(builder.getInt8Ty(), 2);
                    llvm::AllocaInst *charBuf = CreateEntryBlockAlloca(func, "char_tmp", bufType);

                    // 2. Store the char at index 0
                    llvm::Value *zero = builder.getInt32(0);
                    llvm::Value *idx0 = builder.CreateInBoundsGEP(bufType, charBuf, {zero, zero});
                    builder.CreateStore(rhsVal, idx0);

                    // 3. Store null terminator at index 1
                    llvm::Value *one = builder.getInt32(1);
                    llvm::Value *idx1 = builder.CreateInBoundsGEP(bufType, charBuf, {zero, one});
                    builder.CreateStore(builder.getInt8(0), idx1);

                    // 4. Decay to ptr (char*) and Box it into a Moksha String Object
                    llvm::Value *cStr = builder.CreateBitCast(charBuf, builder.getPtrTy());
                    rhsVal = builder.CreateCall(module->getFunction("moksha_box_string"), {cStr});
                }

                if (std::dynamic_pointer_cast<PointerType>(leftType))
                {
                    bool oldWantAddr = wantAddress;
                    wantAddress = true;
                    node.left->accept(*this);
                    llvm::Value *ptr = lastValue; // This is the address
                    wantAddress = oldWantAddr;

                    node.right->accept(*this);
                    llvm::Value *val = lastValue; // This is the value

                    builder.CreateStore(val, ptr);
                    return;
                }

                // B. Box Value
                llvm::Value *boxedVal = rhsVal;
                if (rhsVal->getType()->isIntegerTy(32))
                    boxedVal = builder.CreateCall(module->getFunction("moksha_box_int"), {rhsVal});
                else if (rhsVal->getType()->isDoubleTy())
                    boxedVal = builder.CreateCall(module->getFunction("moksha_box_double"), {rhsVal});
                else if (rhsVal->getType()->isIntegerTy(1))
                {
                    llvm::Value *b = builder.CreateZExt(rhsVal, builder.getInt32Ty());
                    boxedVal = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                }
                if (boxedVal->getType() != builder.getPtrTy())
                    boxedVal = builder.CreateBitCast(boxedVal, builder.getPtrTy());

                // C. Evaluate LHS
                indexNode->callee->accept(*this);
                llvm::Value *target = lastValue;
                indexNode->index->accept(*this);
                llvm::Value *idx = lastValue;

                // D. Set in Table or Array
                if (std::dynamic_pointer_cast<TableType>(leftType) || std::dynamic_pointer_cast<AnyType>(leftType))
                {
                    // Box Key for Tables
                    if (idx->getType()->isIntegerTy(32))
                        idx = builder.CreateCall(module->getFunction("moksha_box_int"), {idx});
                    else if (idx->getType()->isDoubleTy())
                        idx = builder.CreateCall(module->getFunction("moksha_box_double"), {idx});
                    if (idx->getType() != builder.getPtrTy())
                        idx = builder.CreateBitCast(idx, builder.getPtrTy());

                    builder.CreateCall(module->getFunction("moksha_table_set"), {target, idx, boxedVal});
                }
                else
                {
                    // Array indices must be int32 (implicit in moksha_array_set)
                    builder.CreateCall(module->getFunction("moksha_array_set"), {target, idx, boxedVal});
                }
                lastValue = rhsVal;
                return;
            }

            // Standard Assignment

            // Detect if the Left-Hand Side target is a 'shared' variable
            bool targetIsShared = false;
            VariableNode *lhsVarNode = dynamic_cast<VariableNode *>(node.left.get());

            if (lhsVarNode)
            {
                if (sharedVars.count(lhsVarNode->name.lexeme))
                    targetIsShared = true;
            }

            node.right->accept(*this);
            llvm::Value *val = lastValue;

            // 3. Evaluate LHS (Address to store into)
            llvm::Value *ptr = nullptr;

            // If not a shared variable assignment (e.g. normal var, field access), use standard logic

            if (!ptr)
            {
                bool oldWantAddress = wantAddress;
                wantAddress = true;
                node.left->accept(*this);
                wantAddress = oldWantAddress;
                ptr = lastValue;
            }

            if (ptr && val)
            {
                bool volatileAccess = false;
                if (auto varNode = dynamic_cast<VariableNode *>(node.left.get()))
                {
                    volatileAccess = isVolatile(varNode->name.lexeme);
                }

                bool isStructField = false;
                std::string structName;

                if (auto memAccess = dynamic_cast<MemberAccessNode *>(node.left.get()))
                {
                    bool oldWantAddress = wantAddress;
                    wantAddress = true;
                    memAccess->accept(*this);
                    llvm::Value *fieldPtr = lastValue; // This is the memory address (GEP result)
                    wantAddress = oldWantAddress;

                    std::string sName = getStructName(memAccess->object->resolvedType);
                    auto &meta = structBitfields[sName][memAccess->name.lexeme];

                    if (meta.bitWidth > 0)
                    {
                        // Use global maps to find the type and index
                        int fieldIdx = structFieldIndices[sName][memAccess->name.lexeme];

                        // Use structLLVMTypes[sName] instead of 'stType'
                        llvm::Type *containerType = structLLVMTypes[sName]->getStructElementType(fieldIdx);

                        // 1. Load the current full integer container
                        llvm::Value *oldContainer = builder.CreateLoad(containerType, fieldPtr, "bf_old");

                        // 2. Get the new value to store
                        node.right->accept(*this);
                        llvm::Value *newVal = lastValue;

                        // 3. Bitwise Masking: Clear old bits and OR in new bits
                        uint64_t mask = ((1ULL << meta.bitWidth) - 1) << meta.bitOffset;
                        llvm::Value *cleared = builder.CreateAnd(oldContainer, llvm::ConstantInt::get(containerType, ~mask));

                        llvm::Value *maskedNewVal = builder.CreateAnd(newVal, llvm::ConstantInt::get(newVal->getType(), (1ULL << meta.bitWidth) - 1));

                        // Ensure types match before shifting
                        if (maskedNewVal->getType() != containerType)
                            maskedNewVal = builder.CreateZExt(maskedNewVal, containerType);

                        llvm::Value *shiftedNewVal = builder.CreateShl(maskedNewVal, meta.bitOffset);
                        llvm::Value *finalContainer = builder.CreateOr(cleared, shiftedNewVal);

                        // 4. Store the updated container back to the address
                        builder.CreateStore(finalContainer, fieldPtr);
                        return;
                    }
                }

                if (!structName.empty() && structLLVMTypes.count(structName) && !structIsUnion[structName])
                {
                    // It IS a struct! Get the REAL hardware type (e.g. i32)
                    int idx = structFieldIndices[structName][dynamic_cast<MemberAccessNode *>(node.left.get())->name.lexeme];
                    llvm::Type *hardwareType = structLLVMTypes[structName]->getElementType(idx);

                    // Force 'val' to match hardwareType (e.g. i32) without boxing
                    if (val->getType() != hardwareType)
                    {
                        if (val->getType()->isIntegerTy() && hardwareType->isIntegerTy())
                            val = builder.CreateIntCast(val, hardwareType, true); // i64 -> i32
                        else if (val->getType()->isDoubleTy() && hardwareType->isIntegerTy())
                            val = builder.CreateFPToSI(val, hardwareType); // double -> i32
                        else if (val->getType()->isPointerTy() && hardwareType->isIntegerTy())
                            val = builder.CreatePtrToInt(val, hardwareType); // ptr -> i32 (rare)
                    }

                    auto &meta = structBitfields[structName][dynamic_cast<MemberAccessNode *>(node.left.get())->name.lexeme];

                    if (meta.bitWidth > 0)
                    {
                        llvm::Value *currentVal = builder.CreateLoad(hardwareType, ptr);
                        uint64_t mask = ~(((1ULL << meta.bitWidth) - 1) << meta.bitOffset);
                        llvm::Value *cleared = builder.CreateAnd(currentVal, llvm::ConstantInt::get(hardwareType, mask));

                        llvm::Value *newValMasked = builder.CreateAnd(val, (1ULL << meta.bitWidth) - 1);
                        llvm::Value *newValShifted = builder.CreateShl(newValMasked, meta.bitOffset);

                        llvm::Value *combined = builder.CreateOr(cleared, newValShifted);
                        builder.CreateStore(combined, ptr);
                        lastValue = val;
                        return;
                    }

                    // Raw Store (4 bytes)
                    builder.CreateStore(val, ptr);
                    lastValue = val;
                    return; // DONE. Skip standard logic.
                }

                val = emitCast(builder, module.get(), val, node.right->resolvedType, node.left->resolvedType);

                if (auto cls = std::dynamic_pointer_cast<ClassInstanceType>(node.left->resolvedType))
                {
                    // Do NOT clone if the target is shared.
                    if (!isRefClassMap[cls->className] && !targetIsShared)
                    {
                        val = cloneObject(val, cls->className);
                    }
                }

                llvm::Type *neededType = getLLVMType(node.left->resolvedType, context, module.get());
                if (val->getType() != neededType)
                {
                    if (val->getType()->isPointerTy() && neededType->isPointerTy())
                        val = builder.CreateBitCast(val, neededType);
                }

                builder.CreateStore(val, ptr, volatileAccess);
            }
            lastValue = val;
            return;
        }

        bool oldWantAddr = wantAddress;
        wantAddress = false;

        // 2. Evaluate Operands
        node.left->accept(*this);
        llvm::Value *L = lastValue;
        node.right->accept(*this);
        llvm::Value *R = lastValue;

        if (auto *constL = llvm::dyn_cast<llvm::ConstantInt>(L))
        {
            if (auto *constR = llvm::dyn_cast<llvm::ConstantInt>(R))
            {
                // We have two constants! Calculate NOW.
                if (node.op.lexeme == "+")
                {
                    lastValue = llvm::ConstantInt::get(context, constL->getValue() + constR->getValue());
                    return;
                }
                else if (node.op.lexeme == "-")
                {
                    lastValue = llvm::ConstantInt::get(context, constL->getValue() - constR->getValue());
                    return;
                }
                else if (node.op.lexeme == "*")
                {
                    lastValue = llvm::ConstantInt::get(context, constL->getValue() * constR->getValue());
                    return;
                }
                else if (node.op.lexeme == "/")
                {
                    // CRITICAL: Check for Zero before dividing!
                    if (constR->getValue() == 0)
                    {
                        std::cerr << "[Warning] Compile-time division by zero detected. Optimization skipped.\n";
                    }
                    else
                    {
                        // Use 'sdiv' for Signed Integer Division
                        lastValue = llvm::ConstantInt::get(context, constL->getValue().sdiv(constR->getValue()));
                        return;
                    }
                }
                else if (node.op.lexeme == "%")
                {
                    // CRITICAL: Check for Zero before modulo!
                    if (constR->getValue() == 0)
                    {
                        std::cerr << "[Warning] Compile-time modulo by zero detected. Optimization skipped.\n";
                    }
                    else
                    {
                        // Use 'srem' for Signed Remainder (Modulo)
                        lastValue = llvm::ConstantInt::get(context, constL->getValue().srem(constR->getValue()));
                        return;
                    }
                }
            }
        }

        wantAddress = oldWantAddr;

        if (!L || !R)
        {
            lastValue = nullptr;
            return;
        }

        if (insideUnsafe && (node.op.lexeme == "+" || node.op.lexeme == "-"))
        {
            bool L_isPtr = L->getType()->isPointerTy();
            bool R_isPtr = R->getType()->isPointerTy();

            // Ptr + Int
            if (L_isPtr && R->getType()->isIntegerTy())
            {
                llvm::Type *elemType = nullptr;
                auto lhsType = node.left->resolvedType;
                // 1. Check if it's a PointerType (int*)
                if (auto ptrType = std::dynamic_pointer_cast<PointerType>(lhsType))
                {
                    elemType = getLLVMType(ptrType->pointeeType, context, module.get());
                }
                // 2. Check if it's an ArrayType (int[]) used as a pointer
                else if (auto arrType = std::dynamic_pointer_cast<ArrayType>(lhsType))
                {
                    elemType = getLLVMType(arrType->elementType, context, module.get());
                }
                // Fallback to i8 (bytes) only if type info is missing (shouldn't happen in valid code)
                if (elemType)
                {
                    if (node.op.lexeme == "-")
                        R = builder.CreateNeg(R);

                    // Now GEP will jump by sizeof(elemType) instead of 1 byte
                    lastValue = builder.CreateGEP(elemType, L, R, "ptr_math");
                    return;
                }
            }
        }

        // 3. Null Coalescing (??)
        if (node.op.type == TokenType::QUESTION_QUESTION)
        {
            llvm::Function *func = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock *notNullBB = llvm::BasicBlock::Create(context, "coalesce_not_null", func);
            llvm::BasicBlock *nullBB = llvm::BasicBlock::Create(context, "coalesce_null", func);
            llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "coalesce_merge", func);

            llvm::Value *isNull = builder.CreateICmpEQ(L, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(L->getType())));
            builder.CreateCondBr(isNull, nullBB, notNullBB);

            builder.SetInsertPoint(notNullBB);
            llvm::Value *valL = L;
            // Unbox L if result expects primitive
            if (std::dynamic_pointer_cast<IntType>(node.resolvedType))
                valL = builder.CreateCall(module->getFunction("moksha_unbox_int"), {L});
            else if (std::dynamic_pointer_cast<DoubleType>(node.resolvedType))
                valL = builder.CreateCall(module->getFunction("moksha_unbox_double"), {L});
            builder.CreateBr(mergeBB);

            builder.SetInsertPoint(nullBB);
            builder.CreateBr(mergeBB);

            builder.SetInsertPoint(mergeBB);
            llvm::PHINode *phi = builder.CreatePHI(valL->getType(), 2, "coalesce_res");
            phi->addIncoming(valL, notNullBB);
            phi->addIncoming(R, nullBB);
            lastValue = phi;
            return;
        }

        // 4. String Concatenation (+)
        bool lIsString = std::dynamic_pointer_cast<StringType>(node.left->resolvedType) != nullptr;
        bool rIsString = std::dynamic_pointer_cast<StringType>(node.right->resolvedType) != nullptr;
        bool lIsPtr = L->getType()->isPointerTy();
        bool rIsPtr = R->getType()->isPointerTy();

        if (node.op.lexeme == "+" && (lIsString || rIsString))
        {
            auto stringify = [&](llvm::Value *v, std::shared_ptr<Type> t) -> llvm::Value *
            {
                if (std::dynamic_pointer_cast<StringType>(t))
                    return v;
                llvm::Value *rawStr = nullptr;
                if (std::dynamic_pointer_cast<PointerType>(t))
                {
                    rawStr = builder.CreateCall(module->getFunction("moksha_ptr_to_str"), {v});
                }
                // Convert primitives to string for concat
                else if (v->getType()->isPointerTy())
                {
                    if (v->getType() != builder.getPtrTy())
                        v = builder.CreateBitCast(v, builder.getPtrTy());
                    rawStr = builder.CreateCall(module->getFunction("moksha_any_to_str"), {v});
                }
                else if (v->getType()->isIntegerTy(32))
                    rawStr = builder.CreateCall(module->getFunction("moksha_int_to_str"), {v});
                else if (v->getType()->isDoubleTy())
                    rawStr = builder.CreateCall(module->getFunction("moksha_double_to_str"), {v});
                else if (v->getType()->isIntegerTy(1))
                {
                    // 1. Promote i1 (bool) to i32
                    llvm::Value *i32Val = builder.CreateZExt(v, builder.getInt32Ty());
                    // 2. Box it
                    llvm::Value *boxed = builder.CreateCall(module->getFunction("moksha_box_bool"), {i32Val});
                    // 3. Convert to String ("true"/"false")
                    rawStr = builder.CreateCall(module->getFunction("moksha_any_to_str"), {boxed});
                }

                if (rawStr)
                    return builder.CreateCall(module->getFunction("moksha_box_string"), {rawStr});
                return v;
            };
            llvm::Value *strL = stringify(L, node.left->resolvedType);
            llvm::Value *strR = stringify(R, node.right->resolvedType);
            lastValue = builder.CreateCall(module->getFunction("moksha_string_concat"), {strL, strR});
            return;
        }

        // --- 5. NEW: Operator Overloading Support ---
        // Check if LHS is a Class Instance and if "Class_op" function exists
        if (auto clsType = std::dynamic_pointer_cast<ClassInstanceType>(node.left->resolvedType))
        {
            std::string funcName = clsType->className + "_" + node.op.lexeme;

            if (llvm::Function *opFunc = module->getFunction(funcName))
            {
                // ROBUST ARGUMENT MATCHING (FIXED)
                if (opFunc->arg_size() > 1)
                {
                    llvm::Type *expectedType = opFunc->getArg(1)->getType();
                    llvm::Type *rType = R->getType();

                    // Case 1: Operator expects Primitive (e.g. double), but we have Pointer (Boxed)
                    if (!expectedType->isPointerTy() && rType->isPointerTy())
                    {
                        if (expectedType->isDoubleTy())
                            R = builder.CreateCall(module->getFunction("moksha_unbox_double"), {R});
                        else if (expectedType->isIntegerTy(32))
                            R = builder.CreateCall(module->getFunction("moksha_unbox_int"), {R});
                        else if (expectedType->isIntegerTy(1))
                            R = builder.CreateCall(module->getFunction("moksha_unbox_bool"), {R});
                    }
                    // Case 2: Operator expects Pointer (e.g. Vector), but we have Primitive
                    else if (expectedType->isPointerTy() && !rType->isPointerTy())
                    {
                        if (rType->isIntegerTy(32))
                            R = builder.CreateCall(module->getFunction("moksha_box_int"), {R});
                        else if (rType->isDoubleTy())
                            R = builder.CreateCall(module->getFunction("moksha_box_double"), {R});
                        else if (rType->isIntegerTy(1))
                        {
                            llvm::Value *b = builder.CreateZExt(R, builder.getInt32Ty());
                            R = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                        }
                    }
                    // Case 3: Operator expects Pointer, We have Pointer (Bitcast check)
                    else if (expectedType->isPointerTy() && rType->isPointerTy())
                    {
                        if (rType != expectedType)
                            R = builder.CreateBitCast(R, expectedType);
                    }
                    // Case 4: Operator expects Primitive, We have Primitive (Promote/Cast)
                    else if (!expectedType->isPointerTy() && !rType->isPointerTy())
                    {
                        if (rType->isIntegerTy() && expectedType->isDoubleTy())
                            R = builder.CreateSIToFP(R, expectedType);
                    }
                }

                lastValue = builder.CreateCall(opFunc, {L, R}, "op_call");
                return;
            }
        }

        // --- 6. Auto-Unbox Pointers for Arithmetic ---
        // If L or R are Pointers (Any) and we are doing Math/Comparison, we MUST unbox them.
        bool isFloatMath = (node.op.lexeme == "+" || node.op.lexeme == "-" || node.op.lexeme == "*" || node.op.lexeme == "/" || node.op.lexeme == "**");
        bool isIntMath = (node.op.lexeme == "%" || node.op.lexeme == "<<" || node.op.lexeme == ">>" || node.op.lexeme == "&" || node.op.lexeme == "|" || node.op.lexeme == "^");
        bool isCmp = (node.op.lexeme == "==" || node.op.lexeme == "!=" || node.op.lexeme == "<" || node.op.lexeme == ">" || node.op.lexeme == "<=" || node.op.lexeme == ">=");

        if (isFloatMath || isCmp)
        {
            // Unbox to Double for precision
            if (L->getType()->isPointerTy())
            {
                llvm::Value *valPtr = builder.CreateConstInBoundsGEP1_32(builder.getInt8Ty(), L, 8);
                L = builder.CreateLoad(builder.getDoubleTy(), builder.CreateBitCast(valPtr, builder.getPtrTy()));
            }
            if (R->getType()->isPointerTy())
            {
                llvm::Value *valPtr = builder.CreateConstInBoundsGEP1_32(builder.getInt8Ty(), R, 8);
                R = builder.CreateLoad(builder.getDoubleTy(), builder.CreateBitCast(valPtr, builder.getPtrTy()));
            }
        }
        else if (isIntMath)
        {
            // Unbox to Int for bitwise/modulo
            if (L->getType()->isPointerTy())
            {

                llvm::Value *valPtr = builder.CreateConstInBoundsGEP1_32(builder.getInt8Ty(), L, 8);
                L = builder.CreateLoad(builder.getInt32Ty(), builder.CreateBitCast(valPtr, builder.getPtrTy()));
            }
            if (R->getType()->isPointerTy())
            {

                llvm::Value *valPtr = builder.CreateConstInBoundsGEP1_32(builder.getInt8Ty(), R, 8);
                R = builder.CreateLoad(builder.getInt32Ty(), builder.CreateBitCast(valPtr, builder.getPtrTy()));
            }
        }

        if (node.op.lexeme == "in")
        {
            // 1. Handle Table Membership (Key Check)
            if (std::dynamic_pointer_cast<TableType>(node.right->resolvedType))
            {
                // Box the key if necessary (Tables use void* keys)
                llvm::Value *key = L;
                if (key->getType()->isIntegerTy(32))
                    key = builder.CreateCall(module->getFunction("moksha_box_int"), {key});
                else if (key->getType()->isDoubleTy())
                    key = builder.CreateCall(module->getFunction("moksha_box_double"), {key});
                else if (key->getType()->isIntegerTy(1))
                {
                    llvm::Value *b = builder.CreateZExt(key, builder.getInt32Ty());
                    key = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                }
                if (key->getType() != builder.getPtrTy())
                    key = builder.CreateBitCast(key, builder.getPtrTy());

                // Check if get returns null
                llvm::Value *res = builder.CreateCall(module->getFunction("moksha_table_get"), {R, key});
                lastValue = builder.CreateICmpNE(res, llvm::ConstantPointerNull::get(builder.getPtrTy()));
                return;
            }
            // 2. Handle Array Membership (Linear Search)
            else if (std::dynamic_pointer_cast<ArrayType>(node.right->resolvedType))
            {
                llvm::Function *func = builder.GetInsertBlock()->getParent();
                llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "in_cond", func);
                llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "in_body", func);
                llvm::BasicBlock *incBB = llvm::BasicBlock::Create(context, "in_inc", func);
                llvm::BasicBlock *endBB = llvm::BasicBlock::Create(context, "in_end", func);

                // Get Length
                llvm::Value *len = builder.CreateCall(module->getFunction("moksha_get_length"), {R});

                // Iterator 'i'
                llvm::AllocaInst *idxVar = CreateEntryBlockAlloca(func, "in_idx", builder.getInt32Ty());
                builder.CreateStore(builder.getInt32(0), idxVar);

                // Result 'found' (default false)
                llvm::AllocaInst *foundVar = CreateEntryBlockAlloca(func, "in_found", builder.getInt1Ty());
                builder.CreateStore(builder.getInt1(0), foundVar);

                builder.CreateBr(condBB);

                // -- Condition --
                builder.SetInsertPoint(condBB);
                llvm::Value *currIdx = builder.CreateLoad(builder.getInt32Ty(), idxVar);
                llvm::Value *cmp = builder.CreateICmpSLT(currIdx, len);
                builder.CreateCondBr(cmp, bodyBB, endBB);

                // -- Body --
                builder.SetInsertPoint(bodyBB);
                llvm::Value *elemPtr = builder.CreateCall(module->getFunction("moksha_array_get"), {R, currIdx});

                // Compare L (search val) vs elemPtr (array val)
                llvm::Value *isMatch = nullptr;

                // Case A: Integers (Unbox and compare)
                if (L->getType()->isIntegerTy(32))
                {
                    llvm::Value *elemVal = builder.CreateCall(module->getFunction("moksha_unbox_int"), {elemPtr});
                    isMatch = builder.CreateICmpEQ(L, elemVal);
                }
                // Case B: Strings (Strcmp)
                else if (std::dynamic_pointer_cast<StringType>(node.left->resolvedType))
                {
                    llvm::Value *elemStr = builder.CreateCall(module->getFunction("moksha_unbox_string"), {elemPtr});
                    llvm::Value *diff = builder.CreateCall(module->getFunction("strcmp"), {L, elemStr});
                    isMatch = builder.CreateICmpEQ(diff, builder.getInt32(0));
                }
                // Case C: Default pointer equality
                else
                {
                    isMatch = builder.CreateICmpEQ(builder.CreateBitCast(L, builder.getPtrTy()), elemPtr);
                }

                // If match, store true and break
                llvm::BasicBlock *matchBB = llvm::BasicBlock::Create(context, "in_match", func);
                builder.CreateCondBr(isMatch, matchBB, incBB);

                builder.SetInsertPoint(matchBB);
                builder.CreateStore(builder.getInt1(1), foundVar);
                builder.CreateBr(endBB);

                // -- Increment --
                builder.SetInsertPoint(incBB);
                llvm::Value *nextIdx = builder.CreateAdd(currIdx, builder.getInt32(1));
                builder.CreateStore(nextIdx, idxVar);
                builder.CreateBr(condBB);

                // -- End --
                builder.SetInsertPoint(endBB);
                lastValue = builder.CreateLoad(builder.getInt1Ty(), foundVar);
                return;
            }
        }

        // 6. Standard Math/Logic
        bool isFloat = L->getType()->isDoubleTy() || R->getType()->isDoubleTy();

        // --- FIX: DIVISION & MODULO SAFETY ---
        if (node.op.type == TokenType::SLASH || node.op.type == TokenType::MODULO)
        {
            // 1. Check for Zero
            llvm::Value *isZero;
            if (R->getType()->isFloatingPointTy())
            {
                isZero = builder.CreateFCmpOEQ(R, llvm::ConstantFP::get(R->getType(), 0.0));
            }
            else
            {
                isZero = builder.CreateICmpEQ(R, llvm::ConstantInt::get(R->getType(), 0));
            }

            llvm::Function *func = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock *errorBB = llvm::BasicBlock::Create(context, "math_error", func);
            llvm::BasicBlock *safeBB = llvm::BasicBlock::Create(context, "math_safe", func);
            llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "math_merge", func);

            builder.CreateCondBr(isZero, errorBB, safeBB);

            // 2. Error Block
            builder.SetInsertPoint(errorBB);
            llvm::Function *boxStringFunc = module->getFunction("moksha_box_string");
            llvm::Function *throwFunc = module->getFunction("moksha_throw_exception");

            llvm::Value *msg = builder.CreateGlobalString("ArithmeticException: Division by zero");
            llvm::Value *boxedMsg = builder.CreateCall(boxStringFunc, {msg});
            builder.CreateCall(throwFunc, {boxedMsg});

            // Set Flags
            builder.CreateStore(boxedMsg, globalExceptionVal);
            builder.CreateStore(builder.getInt32(1), globalExceptionFlag);

            // Branch to merge instead of unreachable
            builder.CreateBr(mergeBB);

            // 3. Safe Block
            builder.SetInsertPoint(safeBB);
            llvm::Value *calcRes = nullptr;

            if (L->getType()->isFloatingPointTy() || R->getType()->isFloatingPointTy())
            {
                // If one is double, promote both to double
                if (!L->getType()->isFloatingPointTy())
                    L = builder.CreateSIToFP(L, builder.getDoubleTy());
                if (!R->getType()->isFloatingPointTy())
                    R = builder.CreateSIToFP(R, builder.getDoubleTy());

                // Use floating point division/remainder
                if (node.op.type == TokenType::SLASH)
                    calcRes = builder.CreateFDiv(L, R, "fdivtmp");
                else
                    calcRes = builder.CreateFRem(L, R, "fremtmp");
            }
            else
            {
                // Both are integers, safe to use SDiv/SRem
                if (node.op.type == TokenType::SLASH)
                    calcRes = builder.CreateSDiv(L, R, "sdivtmp");
                else
                    calcRes = builder.CreateSRem(L, R, "sremtmp");
            }
            builder.CreateBr(mergeBB);

            // 4. Merge Block
            builder.SetInsertPoint(mergeBB);
            llvm::PHINode *phi = builder.CreatePHI(L->getType(), 2, "math_res");

            // On error, return 0 (dummy value), execution will stop due to exception flag anyway
            phi->addIncoming(llvm::Constant::getNullValue(L->getType()), errorBB);
            phi->addIncoming(calcRes, safeBB);

            lastValue = phi;
            return;
        }

        if (L->getType() != R->getType())
        {
            if (isFloat)
            {
                if (!L->getType()->isDoubleTy())
                    L = (L->getType()->isIntegerTy()) ? builder.CreateSIToFP(L, builder.getDoubleTy()) : builder.CreateFPExt(L, builder.getDoubleTy());
                if (!R->getType()->isDoubleTy())
                    R = (R->getType()->isIntegerTy()) ? builder.CreateSIToFP(R, builder.getDoubleTy()) : builder.CreateFPExt(R, builder.getDoubleTy());
            }
            else if (L->getType()->isIntegerTy() && R->getType()->isIntegerTy())
            {
                if (L->getType()->getIntegerBitWidth() < 32)
                {
                    L = builder.CreateSExt(L, builder.getInt32Ty(), "promote_l");
                }
                if (R->getType()->getIntegerBitWidth() < 32)
                {
                    R = builder.CreateSExt(R, builder.getInt32Ty(), "promote_r");
                }
                unsigned wL = L->getType()->getIntegerBitWidth();
                unsigned wR = R->getType()->getIntegerBitWidth();
                if (wL > wR)
                    R = builder.CreateZExt(R, L->getType());
                else if (wR > wL)
                    L = builder.CreateZExt(L, R->getType());
            }
        }

        if (isFloat)
        {
            if (isCmp)
            {
                llvm::CmpInst::Predicate Pred = llvm::CmpInst::Predicate::BAD_FCMP_PREDICATE;
                if (node.op.lexeme == "==")
                    Pred = llvm::CmpInst::Predicate::FCMP_OEQ;
                else if (node.op.lexeme == "!=")
                    Pred = llvm::CmpInst::Predicate::FCMP_ONE;
                else if (node.op.lexeme == "<")
                    Pred = llvm::CmpInst::Predicate::FCMP_OLT;
                else if (node.op.lexeme == ">")
                    Pred = llvm::CmpInst::Predicate::FCMP_OGT;
                else if (node.op.lexeme == "<=")
                    Pred = llvm::CmpInst::Predicate::FCMP_OLE;
                else if (node.op.lexeme == ">=")
                    Pred = llvm::CmpInst::Predicate::FCMP_OGE;
                lastValue = builder.CreateFCmp(Pred, L, R, "fcmptmp");
            }
            else if (node.op.lexeme == "+")
                lastValue = builder.CreateFAdd(L, R, "faddtmp");
            else if (node.op.lexeme == "-")
                lastValue = builder.CreateFSub(L, R, "fsubtmp");
            else if (node.op.lexeme == "*")
                lastValue = builder.CreateFMul(L, R, "fmultmp");
            else if (node.op.lexeme == "/")
                lastValue = builder.CreateFDiv(L, R, "fdivtmp");
            else if (node.op.lexeme == "**")
            {
                auto powFunc = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::pow, builder.getDoubleTy());
                lastValue = builder.CreateCall(powFunc, {L, R}, "powtmp");
            }
        }
        else
        {
            if (isCmp)
            {
                llvm::CmpInst::Predicate Pred = llvm::CmpInst::Predicate::BAD_ICMP_PREDICATE;
                if (node.op.lexeme == "==")
                    Pred = llvm::CmpInst::Predicate::ICMP_EQ;
                else if (node.op.lexeme == "!=")
                    Pred = llvm::CmpInst::Predicate::ICMP_NE;
                else if (node.op.lexeme == "<")
                    Pred = llvm::CmpInst::Predicate::ICMP_SLT;
                else if (node.op.lexeme == ">")
                    Pred = llvm::CmpInst::Predicate::ICMP_SGT;
                else if (node.op.lexeme == "<=")
                    Pred = llvm::CmpInst::Predicate::ICMP_SLE;
                else if (node.op.lexeme == ">=")
                    Pred = llvm::CmpInst::Predicate::ICMP_SGE;
                lastValue = builder.CreateICmp(Pred, L, R, "icmptmp");
            }
            else if (node.op.lexeme == "/")
            {
                // [FIX] Check for Division by Zero
                llvm::Value *isZero = builder.CreateICmpEQ(R, builder.getInt32(0));

                llvm::Function *func = builder.GetInsertBlock()->getParent();
                llvm::BasicBlock *errorBB = llvm::BasicBlock::Create(context, "div_error", func);
                llvm::BasicBlock *safeBB = llvm::BasicBlock::Create(context, "div_safe", func);
                llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "div_merge", func);

                builder.CreateCondBr(isZero, errorBB, safeBB);

                // Error Path
                builder.SetInsertPoint(errorBB);
                llvm::Function *boxStringFunc = module->getFunction("moksha_box_string");
                llvm::Value *msg = builder.CreateGlobalString("ArithmeticException: Division by zero");
                llvm::Value *boxedMsg = builder.CreateCall(boxStringFunc, {msg});

                // Call runtime throw
                llvm::Function *throwFunc = module->getFunction("moksha_throw_exception");
                builder.CreateCall(throwFunc, {builder.CreateBitCast(boxedMsg, builder.getPtrTy())});

                // Set Global Flag
                builder.CreateStore(boxedMsg, globalExceptionVal);
                builder.CreateStore(builder.getInt32(1), globalExceptionFlag);

                // Return dummy 0.0 and continue (Fail-Fast check will catch this later)
                builder.CreateBr(mergeBB);

                // Safe Path
                builder.SetInsertPoint(safeBB);
                llvm::Value *L_double = builder.CreateSIToFP(L, builder.getDoubleTy());
                llvm::Value *R_double = builder.CreateSIToFP(R, builder.getDoubleTy());
                llvm::Value *divRes = builder.CreateFDiv(L_double, R_double, "f_div_from_int");
                builder.CreateBr(mergeBB);

                // Merge
                builder.SetInsertPoint(mergeBB);
                llvm::PHINode *phi = builder.CreatePHI(builder.getDoubleTy(), 2, "div_res");
                phi->addIncoming(llvm::ConstantFP::get(builder.getDoubleTy(), 0.0), errorBB);
                phi->addIncoming(divRes, safeBB);
                lastValue = phi;
            }
            else if (node.op.lexeme == "//")
                lastValue = builder.CreateSDiv(L, R, "idivtmp");
            else if (node.op.lexeme == "+")
                lastValue = builder.CreateAdd(L, R, "addtmp");
            else if (node.op.lexeme == "-")
                lastValue = builder.CreateSub(L, R, "subtmp");
            else if (node.op.lexeme == "*")
                lastValue = builder.CreateMul(L, R, "multmp");
            else if (node.op.lexeme == "%")
                lastValue = builder.CreateSRem(L, R, "sremtmp");
            else if (node.op.lexeme == "<<")
                lastValue = builder.CreateShl(L, R, "shltmp");
            else if (node.op.lexeme == ">>")
                lastValue = builder.CreateAShr(L, R, "ashrtmp");
            else if (node.op.lexeme == "&")
                lastValue = builder.CreateAnd(L, R, "andtmp");
            else if (node.op.lexeme == "|")
                lastValue = builder.CreateOr(L, R, "ortmp");
            else if (node.op.lexeme == "^")
                lastValue = builder.CreateXor(L, R, "xortmp");
            else if (node.op.lexeme == "**")
            {
                llvm::Value *L_d = builder.CreateSIToFP(L, builder.getDoubleTy());
                llvm::Value *R_d = builder.CreateSIToFP(R, builder.getDoubleTy());
                auto powFunc = llvm::Intrinsic::getOrInsertDeclaration(module.get(), llvm::Intrinsic::pow, builder.getDoubleTy());
                llvm::Value *res = builder.CreateCall(powFunc, {L_d, R_d}, "powtmp");
                lastValue = builder.CreateFPToSI(res, builder.getInt32Ty(), "pow_int_cast");
            }
        }
    }

    void CodeGenerator::visit(UnaryOpNode &node)
    {
        // 1. Address-Of Operator (&x)
        if (node.op.lexeme == "&")
        {
            if (!insideUnsafe)
            {
                std::cerr << "Error: Address-of (&) is only allowed in 'unsafe' blocks.\n";
                lastValue = llvm::ConstantPointerNull::get(builder.getPtrTy());
                return;
            }

            if (auto varNode = dynamic_cast<VariableNode *>(node.right.get()))
            {
                // If it's NOT a local variable, check if it's a Function
                if (namedValues.find(varNode->name.lexeme) == namedValues.end())
                {
                    if (llvm::Function *func = module->getFunction(varNode->name.lexeme))
                    {
                        // Return the Function* directly (which is a pointer)
                        lastValue = func;
                        return;
                    }
                }
            }

            bool oldWantAddress = wantAddress;
            wantAddress = true;
            node.right->accept(*this);
            wantAddress = oldWantAddress;
            return; // lastValue now holds the address
        }

        // 2. Dereference Operator (*ptr)
        if (node.op.lexeme == "*")
        {
            if (!insideUnsafe)
            {
                std::cerr << "Error: Dereference (*) is only allowed in 'unsafe' blocks.\n";
                lastValue = llvm::Constant::getNullValue(getLLVMType(node.resolvedType, context, module.get()));
                return;
            }

            // Evaluate the pointer expression (e.g., getting 0xB8000)
            bool oldWantAddress = wantAddress;
            wantAddress = false; // We want the VALUE of the pointer (the address it points to)
            node.right->accept(*this);
            wantAddress = oldWantAddress;

            llvm::Value *ptrVal = lastValue;

            // If we are assigning to this (*ptr = val), we return the pointer itself.
            if (wantAddress)
            {
                lastValue = ptrVal;
            }
            // If we are reading from this (val = *ptr), we Load from the pointer.
            else
            {
                // We must know what type to read (int? byte?). Semantic Analyzer provides this.
                llvm::Type *loadType = getLLVMType(node.resolvedType, context, module.get());
                lastValue = builder.CreateLoad(loadType, ptrVal);
            }
            return;
        }
        node.right->accept(*this);
        llvm::Value *V = lastValue;
        if (!V)
        {
            lastValue = nullptr;
            return;
        }

        if (node.op.type == TokenType::KW_AWAIT)
        {
            // For MVP, simply return the value (Synchronous execution)
            lastValue = V;
            return;
        }

        if (node.op.lexeme == "!")
        {
            if (V->getType()->isIntegerTy(1))
            {
                lastValue = builder.CreateNot(V, "nottmp");
            }
            else if (V->getType()->isIntegerTy())
            {
                lastValue = builder.CreateICmpEQ(V, builder.getInt32(0), "nottmp");
            }
            else
            {
                lastValue = builder.CreateICmpEQ(V, llvm::Constant::getNullValue(V->getType()), "nottmp");
            }
        }
        else if (node.op.lexeme == "-")
        {
            if (V->getType()->isDoubleTy())
            {
                lastValue = builder.CreateFNeg(V, "negtmp");
            }
            else
            {
                lastValue = builder.CreateNeg(V, "negtmp");
            }
        }
        else if (node.op.lexeme == "~")
        {
            lastValue = builder.CreateNot(V, "bitnot_tmp");
        }
    }

    void CodeGenerator::visit(UpdateNode &node)
    {
        // 1. Get the address using the Visitor Pattern (supports all L-values)
        bool oldWantAddress = wantAddress;
        wantAddress = true;
        node.argument->accept(*this);
        llvm::Value *ptr = lastValue;
        wantAddress = oldWantAddress;

        if (!ptr)
        {
            std::cerr << "Error: l-value required for update operator." << std::endl;
            return;
        }

        // 2. Load the current value
        llvm::Type *valType = getLLVMType(node.argument->resolvedType, context, module.get());
        llvm::Value *oldVal = builder.CreateLoad(valType, ptr, "oldval");

        // 3. Perform the calculation (increment/decrement)
        llvm::Value *newVal;
        if (valType->isIntegerTy())
        {
            if (node.op.type == TokenType::PLUS_PLUS)
                newVal = builder.CreateAdd(oldVal, builder.getInt32(1), "inc");
            else
                newVal = builder.CreateSub(oldVal, builder.getInt32(1), "dec");
        }
        else
        {
            // Floating point support
            if (node.op.type == TokenType::PLUS_PLUS)
                newVal = builder.CreateFAdd(oldVal, llvm::ConstantFP::get(valType, 1.0), "finc");
            else
                newVal = builder.CreateFSub(oldVal, llvm::ConstantFP::get(valType, 1.0), "fdec");
        }

        // 4. Store the new value back to the address
        builder.CreateStore(newVal, ptr);

        // 5. Result of the expression: prefix returns newVal, postfix returns oldVal
        lastValue = node.isPrefix ? newVal : oldVal;
    }

    void CodeGenerator::visit(TernaryOpNode &node)
    {
        node.condition->accept(*this);
        llvm::Value *condV = lastValue;
        if (!condV->getType()->isIntegerTy(1))
        {
            if (std::dynamic_pointer_cast<StringType>(node.condition->resolvedType))
            {
                // Strings are true if length > 0
                llvm::Value *len = builder.CreateCall(module->getFunction("moksha_strlen"), {condV});
                condV = builder.CreateICmpNE(len, builder.getInt32(0), "str_truth");
            }
            else if (condV->getType()->isFloatingPointTy())
            {
                condV = builder.CreateFCmpONE(condV, llvm::ConstantFP::get(context, llvm::APFloat(0.0)));
            }
            else if (condV->getType()->isPointerTy())
            {
                condV = builder.CreateICmpNE(condV, llvm::Constant::getNullValue(condV->getType()));
            }
            else if (condV->getType()->isIntegerTy())
            {
                llvm::Value *zero = llvm::ConstantInt::get(condV->getType(), 0);
                condV = builder.CreateICmpNE(condV, zero);
            }
        }

        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *trueBB = llvm::BasicBlock::Create(context, "truecase", func);
        llvm::BasicBlock *falseBB = llvm::BasicBlock::Create(context, "falsecase", func);
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ternmerge", func);

        builder.CreateCondBr(condV, trueBB, falseBB);

        builder.SetInsertPoint(trueBB);
        node.thenBranch->accept(*this);
        llvm::Value *trueV = lastValue;
        builder.CreateBr(mergeBB);
        trueBB = builder.GetInsertBlock();

        builder.SetInsertPoint(falseBB);
        node.elseBranch->accept(*this);
        llvm::Value *falseV = lastValue;
        builder.CreateBr(mergeBB);
        falseBB = builder.GetInsertBlock();

        builder.SetInsertPoint(mergeBB);
        llvm::PHINode *PN = builder.CreatePHI(trueV->getType(), 2, "ternresult");
        PN->addIncoming(trueV, trueBB);
        PN->addIncoming(falseV, falseBB);
        lastValue = PN;
    }

    void CodeGenerator::visit(CallNode &node)
    {
        // 1. Extract the function name if the callee is an identifier
        std::string calleeName = "";
        if (auto varNode = dynamic_cast<VariableNode *>(node.callee.get()))
        {
            calleeName = varNode->name.lexeme;
            if (calleeName == "readInt")
                calleeName = "moksha_read_int";
            else if (calleeName == "readDouble")
                calleeName = "moksha_read_double";
            else if (calleeName == "readBool")
                calleeName = "moksha_read_bool";
            else if (calleeName == "read")
                calleeName = "moksha_read_string";
        }

        // --- CASE 1: Member Access (obj.method()) ---
        if (auto memAccess = dynamic_cast<MemberAccessNode *>(node.callee.get()))
        {
            // A. Evaluate the Object (objPtr) FIRST
            memAccess->object->accept(*this);
            llvm::Value *objPtr = lastValue;

            if (!objPtr->getType()->isPointerTy())
            {
                std::cerr << "CodeGen Error: Method call on non-pointer object.\n";
                lastValue = llvm::ConstantPointerNull::get(builder.getPtrTy());
                return;
            }

            if (objPtr->getType() != builder.getPtrTy())
                objPtr = builder.CreateBitCast(objPtr, builder.getPtrTy());

            // Null Safety Check
            if (!memAccess->isOptional)
            {
                llvm::Value *isNull = builder.CreateICmpEQ(objPtr, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(objPtr->getType())));

                llvm::Function *func = builder.GetInsertBlock()->getParent();
                llvm::BasicBlock *errorBB = llvm::BasicBlock::Create(context, "call_error", func);
                llvm::BasicBlock *safeBB = llvm::BasicBlock::Create(context, "call_safe", func);

                builder.CreateCondBr(isNull, errorBB, safeBB);

                // Error Path
                builder.SetInsertPoint(errorBB);
                llvm::Function *boxStringFunc = module->getFunction("moksha_box_string");
                llvm::Value *msg = builder.CreateGlobalString("NullReferenceException: Attempted access on null object");
                llvm::Value *boxedMsg = builder.CreateCall(boxStringFunc, {msg});

                builder.CreateStore(boxedMsg, globalExceptionVal);
                builder.CreateStore(builder.getInt32(1), globalExceptionFlag);

                // Unwind / Fail Fast
                if (!exceptionStack.empty())
                {
                    builder.CreateBr(exceptionStack.top());
                }
                else
                {
                    // Return the correct zero-value for the current function
                    llvm::Type *retType = func->getReturnType();
                    if (retType->isVoidTy())
                        builder.CreateRetVoid();
                    else
                        builder.CreateRet(llvm::Constant::getNullValue(retType));
                }
                // Safe Path
                builder.SetInsertPoint(safeBB);
            }

            // --- Optional Chaining Setup ---
            llvm::BasicBlock *continueBB = nullptr;
            llvm::BasicBlock *nullBB = nullptr;
            llvm::BasicBlock *callBB = nullptr;
            llvm::BasicBlock *accessBB = nullptr; // Used for Phi incoming
            llvm::PHINode *phi = nullptr;

            if (memAccess->isOptional)
            {
                llvm::Function *func = builder.GetInsertBlock()->getParent();
                callBB = llvm::BasicBlock::Create(context, "opt_call_exec", func);
                nullBB = llvm::BasicBlock::Create(context, "opt_call_null", func);
                continueBB = llvm::BasicBlock::Create(context, "opt_call_cont", func);

                llvm::Value *isNull = builder.CreateICmpEQ(objPtr, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(objPtr->getType())));
                builder.CreateCondBr(isNull, nullBB, callBB);
                builder.SetInsertPoint(callBB);
            }

            // B. Resolve Class Name
            std::string objClassName = "";
            if (auto v = dynamic_cast<VariableNode *>(memAccess->object.get()))
            {
                if (variableTypes.count(v->name.lexeme))
                    objClassName = variableTypes[v->name.lexeme];
            }
            else if (auto clsType = std::dynamic_pointer_cast<ClassInstanceType>(memAccess->object->resolvedType))
            {
                objClassName = clsType->className;
            }
            else if (dynamic_cast<ThisExpression *>(memAccess->object.get()))
            {
                objClassName = currentClassName;
            }
            else if (auto newExpr = dynamic_cast<NewExpression *>(memAccess->object.get()))
            {
                objClassName = newExpr->className.lexeme;
            }

            // C. Prepare Arguments (argsV)
            // 'this' must be the first argument
            std::vector<llvm::Value *> argsV;
            argsV.push_back(objPtr);

            for (auto &arg : node.arguments)
            {
                arg->accept(*this);
                argsV.push_back(lastValue);
            }

            // D. Method Resolution Variables
            llvm::Value *result = nullptr;
            bool methodFound = false;

            if (!objClassName.empty())
            {
                int vtableIdx = SemanticAnalyzer::getVTableIndex(objClassName, memAccess->name.lexeme);

                // --- Virtual Dispatch ---
                if (vtableIdx != -1)
                {
                    // 1. Get VTable Address
                    llvm::StructType *st = llvm::StructType::getTypeByName(context, objClassName);
                    if (st)
                    {
                        llvm::Value *vptrSlot = builder.CreateStructGEP(st, objPtr, 0);
                        llvm::Value *vtableBase = builder.CreateLoad(builder.getPtrTy(), vptrSlot, "vptr");

                        // 2. Get Function Pointer
                        llvm::Value *funcPtrSlot = builder.CreateConstInBoundsGEP1_32(builder.getPtrTy(), vtableBase, vtableIdx);
                        llvm::Value *rawFuncPtr = builder.CreateLoad(builder.getPtrTy(), funcPtrSlot, "func_ptr");

                        // 3. Get Signature
                        const auto &vtableEntries = SemanticAnalyzer::getVTable(objClassName);
                        // SAFETY CHECK: Ensure index is valid
                        if (vtableIdx < vtableEntries.size())
                        {
                            auto &entry = vtableEntries[vtableIdx];
                            std::shared_ptr<FunctionType> sig = entry.signature;

                            llvm::Type *retType = getLLVMType(sig->returnType, context, module.get());

                            // 4. Construct Function Type
                            std::vector<llvm::Type *> paramTypes;
                            paramTypes.push_back(builder.getPtrTy()); // this
                            for (auto &pt : sig->parameterTypes)
                                paramTypes.push_back(getLLVMType(pt, context, module.get()));

                            llvm::FunctionType *ft = llvm::FunctionType::get(retType, paramTypes, sig->isVariadic);

                            // 5. Cast Arguments
                            std::vector<llvm::Value *> finalArgs;
                            finalArgs.push_back(argsV[0]); // this
                            for (size_t i = 0; i < sig->parameterTypes.size(); i++)
                            {
                                if (i + 1 >= argsV.size())
                                    break;
                                llvm::Value *val = argsV[i + 1];
                                llvm::Type *exp = paramTypes[i + 1];
                                if (val->getType() != exp)
                                {
                                    if (val->getType()->isIntegerTy() && exp->isFloatingPointTy())
                                        val = builder.CreateSIToFP(val, exp);
                                    else if (exp->isPointerTy() && val->getType()->isPointerTy())
                                        val = builder.CreateBitCast(val, exp);
                                    else if (val->getType()->isPointerTy() != exp->isPointerTy())
                                    {
                                        // Critical Mismatch (e.g. int vs ptr) - Handle or Warn
                                        std::cerr << "Warning: Type mismatch in virtual call arg " << i << "\n";
                                    }
                                }
                                finalArgs.push_back(val);
                            }

                            // 6. Call
                            std::string callName = retType->isVoidTy() ? "" : "virt_call";
                            result = builder.CreateCall(ft, rawFuncPtr, finalArgs, callName);
                            methodFound = true;
                        }
                    }
                }

                // --- Static Dispatch Fallback ---
                if (!methodFound)
                {
                    std::string mangled = objClassName + "_" + memAccess->name.lexeme;
                    if (llvm::Function *func = module->getFunction(mangled))
                    {
                        // Simple casting loop for static calls
                        for (size_t i = 0; i < argsV.size(); ++i)
                        {
                            if (i < func->arg_size() && argsV[i]->getType() != func->getArg(i)->getType())
                            {
                                if (func->getArg(i)->getType()->isPointerTy())
                                    argsV[i] = builder.CreateBitCast(argsV[i], func->getArg(i)->getType());
                            }
                        }
                        result = builder.CreateCall(func, argsV, func->getReturnType()->isVoidTy() ? "" : "call");
                        methodFound = true;
                    }
                }
            }

            if (!methodFound)
                result = llvm::ConstantPointerNull::get(builder.getPtrTy());

            // --- Optional Merge ---
            if (memAccess->isOptional)
            {
                llvm::Value *phiValue = result;
                if (result->getType()->isVoidTy())
                {
                    phiValue = llvm::ConstantPointerNull::get(builder.getPtrTy());
                }
                else
                {
                    if (result->getType()->isIntegerTy(32))
                        result = builder.CreateCall(module->getFunction("moksha_box_int"), {result});

                    if (result->getType() != builder.getPtrTy())
                        phiValue = builder.CreateBitCast(result, builder.getPtrTy());
                    else
                        phiValue = result;
                }
                builder.CreateBr(continueBB);
                builder.SetInsertPoint(nullBB);
                llvm::Value *nullVal = llvm::ConstantPointerNull::get(builder.getPtrTy());
                builder.CreateBr(continueBB);
                builder.SetInsertPoint(continueBB);
                phi = builder.CreatePHI(builder.getPtrTy(), 2, "opt_call_res");
                phi->addIncoming(nullVal, nullBB);
                phi->addIncoming(phiValue, callBB);
                result = phi;
            }
            lastValue = result;
            return;
        }

        // --- CASE 2: Named Global Function Call (Direct) ---
        if (auto varNode = dynamic_cast<VariableNode *>(node.callee.get()))
        {
            std::string name = varNode->name.lexeme;
            if (classFieldIndices.count(name))
                return;

            llvm::Function *calleeFunc = module->getFunction(name);

            // Check for implicit method call (this.method())
            if (!calleeFunc && !currentClassName.empty())
            {
                std::string mangled = currentClassName + "_" + name;
                calleeFunc = module->getFunction(mangled);
                if (calleeFunc)
                {
                    std::vector<llvm::Value *> argsV;
                    if (!currentThis)
                    {
                        lastValue = builder.getInt32(0);
                        return;
                    }
                    argsV.push_back(currentThis);
                    for (size_t i = 0; i < node.arguments.size(); ++i)
                    {
                        node.arguments[i]->accept(*this);
                        llvm::Value *val = lastValue;
                        if ((i + 1) < calleeFunc->arg_size())
                        {
                            llvm::Type *expectedType = calleeFunc->getArg(i + 1)->getType();
                            if (expectedType->isPointerTy() && !val->getType()->isPointerTy())
                            {
                                if (val->getType()->isIntegerTy(32))
                                    val = builder.CreateCall(module->getFunction("moksha_box_int"), {val});
                                else if (val->getType()->isDoubleTy())
                                    val = builder.CreateCall(module->getFunction("moksha_box_double"), {val});
                                else if (val->getType()->isIntegerTy(1))
                                {
                                    llvm::Value *b = builder.CreateZExt(val, builder.getInt32Ty());
                                    val = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                                }
                            }
                            if (val->getType() != expectedType && expectedType->isPointerTy())
                                val = builder.CreateBitCast(val, expectedType);
                        }
                        argsV.push_back(val);
                    }
                    lastValue = builder.CreateCall(llvm::FunctionCallee(calleeFunc), argsV, calleeFunc->getReturnType()->isVoidTy() ? "" : "method_call");
                    return;
                }
            }

            if (calleeFunc)
            {
                std::vector<llvm::Value *> argsV;
                FunctionDefinitionNode *funcDef = nullptr;
                if (globalFunctionDefs.count(name))
                {
                    funcDef = globalFunctionDefs[name];
                }

                for (size_t i = 0; i < node.arguments.size(); ++i)
                {
                    bool isRefParam = false;
                    if (funcDef && i < funcDef->parameters.size())
                    {
                        isRefParam = funcDef->parameters[i]->isRef;
                    }

                    if (isRefParam)
                    {
                        bool oldWantAddress = wantAddress;
                        wantAddress = true;
                        node.arguments[i]->accept(*this);
                        wantAddress = oldWantAddress;

                        llvm::Value *addr = lastValue;
                        if (!addr->getType()->isPointerTy())
                        {
                            llvm::AllocaInst *temp = CreateEntryBlockAlloca(builder.GetInsertBlock()->getParent(), "ref_tmp", addr->getType());
                            builder.CreateStore(addr, temp);
                            addr = temp;
                        }
                        if (addr->getType() != builder.getPtrTy())
                            addr = builder.CreateBitCast(addr, builder.getPtrTy());

                        argsV.push_back(addr);
                    }
                    else
                    {
                        node.arguments[i]->accept(*this);
                        llvm::Value *val = lastValue;

                        // String unboxing for C Runtime functions
                        bool isStringObj = std::dynamic_pointer_cast<StringType>(node.arguments[i]->resolvedType) != nullptr;
                        bool isCRuntime = (calleeName == "printf" || calleeName == "sprintf" || calleeName == "puts" ||
                                           calleeName == "strcmp" || calleeName == "strcpy"); // Add others as needed

                        if (isCRuntime && isStringObj)
                        {
                            val = builder.CreateCall(module->getFunction("moksha_unbox_string"), {val}, "auto_unbox_str");
                        }
                        else if (calleeName == "printf" && val->getType() == builder.getPtrTy())
                        {
                            // Fallback legacy check
                            val = builder.CreateCall(module->getFunction("moksha_unbox_string"), {val}, "auto_unbox_str");
                        }

                        if (i < calleeFunc->arg_size())
                        {
                            llvm::Type *expectedType = calleeFunc->getArg(i)->getType();

                            if (val->getType()->isIntegerTy() && expectedType->isIntegerTy())
                            {
                                if (val->getType()->getIntegerBitWidth() < expectedType->getIntegerBitWidth())
                                    val = builder.CreateZExt(val, expectedType);
                                else if (val->getType()->getIntegerBitWidth() > expectedType->getIntegerBitWidth())
                                    val = builder.CreateTrunc(val, expectedType);
                            }

                            // Boxing/Unboxing logic...
                            if (expectedType->isPointerTy() && !val->getType()->isPointerTy())
                            {
                                // Box primitives
                                if (val->getType()->isIntegerTy(32))
                                    val = builder.CreateCall(module->getFunction("moksha_box_int"), {val});
                                else if (val->getType()->isDoubleTy())
                                    val = builder.CreateCall(module->getFunction("moksha_box_double"), {val});
                                else if (val->getType()->isIntegerTy(1))
                                {
                                    llvm::Value *b = builder.CreateZExt(val, builder.getInt32Ty());
                                    val = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                                }
                            }
                            else if (!expectedType->isPointerTy() && val->getType()->isPointerTy())
                            {
                                // Unbox
                                if (expectedType->isIntegerTy(32))
                                    val = builder.CreateCall(module->getFunction("moksha_unbox_int"), {val});
                                else if (expectedType->isDoubleTy())
                                    val = builder.CreateCall(module->getFunction("moksha_unbox_double"), {val});
                            }

                            if (expectedType->isPointerTy() && val->getType() != expectedType)
                            {
                                val = builder.CreateBitCast(val, expectedType);
                            }
                        }
                        argsV.push_back(val);
                    }
                }

                // Default Arguments
                if (globalFunctionDefs.count(name))
                {
                    FunctionDefinitionNode *def = globalFunctionDefs[name];
                    if (node.arguments.size() < def->parameters.size())
                    {
                        for (size_t i = node.arguments.size(); i < def->parameters.size(); ++i)
                        {
                            if (def->parameters[i]->defaultValue)
                            {
                                def->parameters[i]->defaultValue->accept(*this);
                                argsV.push_back(lastValue);
                            }
                            else
                            {
                                argsV.push_back(llvm::Constant::getNullValue(calleeFunc->getArg(i)->getType()));
                            }
                        }
                    }
                }
                lastValue = builder.CreateCall(llvm::FunctionCallee(calleeFunc), argsV, calleeFunc->getReturnType()->isVoidTy() ? "" : "call");
                return;
            }
        }

        // --- CASE 3: Indirect / Closure Call ---
        node.callee->accept(*this);
        llvm::Value *calleeVal = lastValue;

        if (!calleeVal)
        {
            lastValue = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
            return;
        }

        std::vector<llvm::Value *> argsV;
        for (auto &arg : node.arguments)
        {
            arg->accept(*this);
            llvm::Value *val = lastValue;
            // Closures expect generic pointers
            if (val->getType()->isIntegerTy(32))
                val = builder.CreateCall(module->getFunction("moksha_box_int"), {val});
            else if (val->getType()->isDoubleTy())
                val = builder.CreateCall(module->getFunction("moksha_box_double"), {val});
            else if (val->getType()->isIntegerTy(1))
            {
                llvm::Value *b = builder.CreateZExt(val, builder.getInt32Ty());
                val = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
            }
            if (val->getType() != builder.getPtrTy())
                val = builder.CreateBitCast(val, builder.getPtrTy());

            argsV.push_back(val);
        }

        llvm::Function *getFuncFunc = module->getFunction("moksha_get_closure_func");
        llvm::Function *getEnvFunc = module->getFunction("moksha_get_closure_env");

        if (getFuncFunc && getEnvFunc && calleeVal->getType() == builder.getPtrTy())
        {
            llvm::Value *rawFuncPtr = builder.CreateCall(llvm::FunctionCallee(getFuncFunc), {calleeVal}, "raw_func_ptr");
            llvm::Value *envPtr = builder.CreateCall(llvm::FunctionCallee(getEnvFunc), {calleeVal}, "env_ptr");
            std::vector<llvm::Value *> finalArgs;
            finalArgs.push_back(envPtr);
            for (auto &arg : argsV)
                finalArgs.push_back(arg);

            std::vector<llvm::Type *> finalParamTypes;
            finalParamTypes.push_back(builder.getPtrTy());
            for (size_t i = 0; i < argsV.size(); i++)
                finalParamTypes.push_back(builder.getPtrTy());

            llvm::FunctionType *ft = llvm::FunctionType::get(builder.getPtrTy(), finalParamTypes, false);
            llvm::Value *typedFuncPtr = builder.CreateBitCast(rawFuncPtr, llvm::PointerType::get(context, 0));
            lastValue = builder.CreateCall(ft, typedFuncPtr, finalArgs, "closure_call");
        }
        else
        {
            // Raw pointer call fallback
            std::vector<llvm::Type *> paramTypes;
            for (auto &a : argsV)
                paramTypes.push_back(a->getType());

            if (calleeVal->getType()->isPointerTy())
            {
                llvm::FunctionType *ft = llvm::FunctionType::get(builder.getPtrTy(), paramTypes, false);
                llvm::Value *castedCallee = builder.CreateBitCast(calleeVal, llvm::PointerType::get(context, 0));
                lastValue = builder.CreateCall(ft, castedCallee, argsV, "indirect_call");
            }
        }
    }

    void CodeGenerator::visit(InputExpression &node)
    {
        llvm::Function *func = nullptr;
        std::string funcName;

        // 1. Map token to the correct runtime function name
        if (node.token.type == TokenType::KW_READINT)
        {
            funcName = "moksha_read_int";
        }
        else if (node.token.type == TokenType::KW_READDOUBLE)
        {
            funcName = "moksha_read_double";
        }
        else if (node.token.type == TokenType::KW_READBOOL)
        {
            funcName = "moksha_read_bool";
        }
        else if (node.token.type == TokenType::KW_READ)
        {
            funcName = "moksha_read_string";
        }

        // 2. Retrieve the function
        func = module->getFunction(funcName);

        // 3. SAFETY CHECK
        if (!func)
        {
            std::cerr << "[CodeGen Error] Function '" << funcName << "' not found. Did you declare it in the constructor?" << std::endl;
            // FIX 1: Assign to lastValue instead of symbolTable.addTemp
            lastValue = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
            return;
        }

        // 4. Generate the Call
        llvm::Value *result = builder.CreateCall(func, {});

        // 5. Post-Processing
        if (node.token.type == TokenType::KW_READBOOL)
        {
            result = builder.CreateTrunc(result, builder.getInt1Ty(), "bool_trunc");
        }

        // FIX 2: Assign result to lastValue so parent nodes can use it
        lastValue = result;
    }

    void CodeGenerator::visit(TemplateLiteralNode &node)
    {
        llvm::Function *concatFunc = module->getFunction("moksha_string_concat");
        llvm::Function *i2sFunc = module->getFunction("moksha_int_to_str");
        llvm::Function *d2sFunc = module->getFunction("moksha_double_to_str");
        llvm::Function *anyToStrFunc = module->getFunction("moksha_any_to_str");
        llvm::Function *boxStringFunc = module->getFunction("moksha_box_string");

        // 1. Create an empty base string (Boxed)
        llvm::Value *currentStr = builder.CreateGlobalString("");
        currentStr = builder.CreateCall(llvm::FunctionCallee(boxStringFunc), {currentStr});

        for (auto &part : node.parts)
        {
            part->accept(*this);
            llvm::Value *val = lastValue;

            std::shared_ptr<Type> type = part->resolvedType;
            bool isAny = std::dynamic_pointer_cast<AnyType>(type) != nullptr;
            bool isArray = std::dynamic_pointer_cast<ArrayType>(type) != nullptr;
            bool isTable = std::dynamic_pointer_cast<TableType>(type) != nullptr;

            // 2. Convert to String Object (Pointer)
            if (std::dynamic_pointer_cast<StringType>(type))
            {
                // Already a string object, no conversion needed
            }
            else if (isAny || isArray || isTable || val->getType()->isPointerTy())
            {
                if (val->getType() != builder.getPtrTy())
                    val = builder.CreateBitCast(val, builder.getPtrTy());

                // any_to_str returns char*, so we MUST BOX IT
                val = builder.CreateCall(llvm::FunctionCallee(anyToStrFunc), {val}, "any_to_s");
                val = builder.CreateCall(llvm::FunctionCallee(boxStringFunc), {val}, "box_s");
            }
            else if (val->getType()->isIntegerTy(1)) // Bool
            {
                llvm::Value *i32Val = builder.CreateZExt(val, builder.getInt32Ty(), "bool_ext");
                llvm::Function *boxBoolFunc = module->getFunction("moksha_box_bool");
                llvm::Value *boxedVal = builder.CreateCall(llvm::FunctionCallee(boxBoolFunc), {i32Val}, "box_b");

                // any_to_str returns char*, so we MUST BOX IT
                val = builder.CreateCall(llvm::FunctionCallee(anyToStrFunc), {boxedVal}, "b2s");
                val = builder.CreateCall(llvm::FunctionCallee(boxStringFunc), {val}, "box_s");
            }
            else if (val->getType()->isIntegerTy()) // Int
            {
                // int_to_str returns char*, so we MUST BOX IT
                val = builder.CreateCall(llvm::FunctionCallee(i2sFunc), {val}, "i2s");
                val = builder.CreateCall(llvm::FunctionCallee(boxStringFunc), {val}, "box_s");
            }
            else if (val->getType()->isDoubleTy()) // Double
            {
                // double_to_str returns char*, so we MUST BOX IT
                val = builder.CreateCall(llvm::FunctionCallee(d2sFunc), {val}, "d2s");
                val = builder.CreateCall(llvm::FunctionCallee(boxStringFunc), {val}, "box_s");
            }

            // 3. Concatenate (Expects two Boxed Strings)
            currentStr = builder.CreateCall(llvm::FunctionCallee(concatFunc), {currentStr, val});
        }
        lastValue = currentStr;
    }

    void CodeGenerator::visit(ArrayLiteralNode &node)
    {
        llvm::Function *arrayAllocFunc = module->getFunction("moksha_alloc_array");
        llvm::Function *arrayPushFunc = module->getFunction("moksha_array_push_val");
        llvm::Function *spreadFunc = module->getFunction("moksha_array_spread");

        llvm::Function *boxInt = module->getFunction("moksha_box_int");
        llvm::Function *boxDouble = module->getFunction("moksha_box_double");
        llvm::Function *boxBool = module->getFunction("moksha_box_bool");

        // 1. Allocate initial array
        llvm::Value *arrPtr = builder.CreateCall(llvm::FunctionCallee(arrayAllocFunc), {builder.getInt32(node.elements.size())});

        for (auto &el : node.elements)
        {
            if (dynamic_cast<SpreadElementNode *>(el.get()))
            {
                el->accept(*this);
                // Spread also might realloc, so update arrPtr
                arrPtr = builder.CreateCall(llvm::FunctionCallee(spreadFunc), {arrPtr, lastValue});
            }
            else
            {
                el->accept(*this);
                llvm::Value *val = lastValue;
                if (!val)
                    val = llvm::Constant::getNullValue(builder.getPtrTy());

                llvm::Value *boxedVal = val;

                // Boxing logic
                if (val->getType()->isIntegerTy(32))
                    boxedVal = builder.CreateCall(llvm::FunctionCallee(boxInt), {val});
                else if (val->getType()->isIntegerTy(1))
                {
                    llvm::Value *i32bool = builder.CreateZExt(val, builder.getInt32Ty());
                    boxedVal = builder.CreateCall(llvm::FunctionCallee(boxBool), {i32bool});
                }
                else if (val->getType()->isDoubleTy())
                    boxedVal = builder.CreateCall(llvm::FunctionCallee(boxDouble), {val});
                else if (val->getType()->isPointerTy() && val->getType() != builder.getPtrTy())
                    boxedVal = builder.CreateBitCast(val, builder.getPtrTy());

                std::vector<llvm::Value *> args = {arrPtr, boxedVal};
                arrPtr = builder.CreateCall(llvm::FunctionCallee(arrayPushFunc), args);
            }
        }
        lastValue = arrPtr;
    }

    void CodeGenerator::visit(SpreadElementNode &node) { node.expression->accept(*this); }

    // CodeGenerator.cpp

    void CodeGenerator::visit(TableLiteralNode &node)
    {
        llvm::Function *tableAllocFunc = module->getFunction("moksha_alloc_table");
        llvm::Function *tableSetFunc = module->getFunction("moksha_table_set");

        // Get Boxing Functions
        llvm::Function *boxInt = module->getFunction("moksha_box_int");
        llvm::Function *boxDouble = module->getFunction("moksha_box_double");
        llvm::Function *boxBool = module->getFunction("moksha_box_bool");
        llvm::Function *boxString = module->getFunction("moksha_box_string");

        llvm::Value *tablePtr = builder.CreateCall(llvm::FunctionCallee(tableAllocFunc), {builder.getInt32(node.entries.size())});

        for (auto &entry : node.entries)
        {
            // --- 1. Process Key ---
            entry.first->accept(*this);
            llvm::Value *keyVal = lastValue;

            // BOX THE KEY
            if (keyVal->getType()->isIntegerTy(32))
            {
                keyVal = builder.CreateCall(llvm::FunctionCallee(boxInt), {keyVal});
            }
            else if (keyVal->getType()->isDoubleTy())
            {
                keyVal = builder.CreateCall(llvm::FunctionCallee(boxDouble), {keyVal});
            }
            else if (keyVal->getType()->isPointerTy())
            {
                if (keyVal->getType() != builder.getPtrTy())
                {
                    keyVal = builder.CreateBitCast(keyVal, builder.getPtrTy());
                }
            }

            // --- 2. Process Value ---
            entry.second->accept(*this);
            llvm::Value *valVal = lastValue;

            // BOX THE VALUE
            if (valVal->getType()->isIntegerTy(32))
            {
                valVal = builder.CreateCall(llvm::FunctionCallee(boxInt), {valVal});
            }
            else if (valVal->getType()->isDoubleTy())
            {
                valVal = builder.CreateCall(llvm::FunctionCallee(boxDouble), {valVal});
            }
            else if (valVal->getType()->isIntegerTy(1))
            { // Bool
                llvm::Value *b = builder.CreateZExt(valVal, builder.getInt32Ty());
                valVal = builder.CreateCall(llvm::FunctionCallee(boxBool), {b});
            }
            else if (valVal->getType()->isPointerTy())
            {
                if (valVal->getType() != builder.getPtrTy())
                {
                    valVal = builder.CreateBitCast(valVal, builder.getPtrTy());
                }
            }

            std::vector<llvm::Value *> args = {tablePtr, keyVal, valVal};
            builder.CreateCall(llvm::FunctionCallee(tableSetFunc), args);
        }
        lastValue = tablePtr;
    }

    void CodeGenerator::visit(LengthExpressionNode &node)
    {
        // 1. Compile the target expression
        node.target->accept(*this);
        llvm::Value *targetPtr = lastValue;

        // Safety: Ensure pointer is generic char* (i8*) before math
        if (targetPtr->getType() != builder.getPtrTy()) {
            targetPtr = builder.CreateBitCast(targetPtr, builder.getPtrTy());
        }

        // Determine offset based on type
        int offset = -1;
        if (std::dynamic_pointer_cast<ArrayType>(node.target->resolvedType)) {
            offset = 12; // Array Length Offset
        } else if (std::dynamic_pointer_cast<StringType>(node.target->resolvedType)) {
            offset = 16; // String Length Offset
        }

        if (offset != -1) {
            // --- OPTIMIZATION: INLINE LOAD WITH NULL CHECK ---
            
            llvm::Function *func = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock *safeBB = llvm::BasicBlock::Create(context, "len_safe", func);
            llvm::BasicBlock *nullBB = llvm::BasicBlock::Create(context, "len_null", func);
            llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "len_merge", func);

            // 1. Check for Null (Fast integer comparison)
            llvm::Value *isNull = builder.CreateICmpEQ(targetPtr, llvm::ConstantPointerNull::get(builder.getPtrTy()));
            builder.CreateCondBr(isNull, nullBB, safeBB);

            // 2. Null Block: Return 0 (Safe fallback)
            builder.SetInsertPoint(nullBB);
            builder.CreateBr(mergeBB);

            // 3. Safe Block: Load Length directly from memory
            builder.SetInsertPoint(safeBB);
            llvm::Value *lenPtr = builder.CreateConstInBoundsGEP1_32(builder.getInt8Ty(), targetPtr, offset);
            llvm::Value *lenTypedPtr = builder.CreateBitCast(lenPtr, llvm::PointerType::get(context, 0));
            llvm::Value *realLen = builder.CreateLoad(builder.getInt32Ty(), lenTypedPtr, "inline_len");
            builder.CreateBr(mergeBB);

            // 4. Merge results using PHI node
            builder.SetInsertPoint(mergeBB);
            llvm::PHINode *phi = builder.CreatePHI(builder.getInt32Ty(), 2, "len_res");
            phi->addIncoming(builder.getInt32(0), nullBB);
            phi->addIncoming(realLen, safeBB);

            lastValue = phi;
            return;
        }

        // Fallback for unknown types
        lastValue = builder.CreateCall(module->getFunction("moksha_get_length"), {targetPtr});
    }

    void CodeGenerator::visit(IndexNode &node)
    {
        // 1. Evaluate Callee (the object to access)
        bool oldWantAddress = wantAddress;
        bool isFixedArray = false;
        if (auto arr = std::dynamic_pointer_cast<ArrayType>(node.callee->resolvedType))
        {
            if (arr->isFixedSize)
                isFixedArray = true;
        }
        if (isFixedArray)
            wantAddress = true;

        node.callee->accept(*this);
        llvm::Value *target = lastValue;
        std::shared_ptr<Type> type = node.callee->resolvedType;
        wantAddress = oldWantAddress;

        if (std::dynamic_pointer_cast<PointerType>(type))
        {
            bool idxWantAddr = wantAddress;
            wantAddress = false;
            node.index->accept(*this);
            llvm::Value *idx = lastValue;
            wantAddress = idxWantAddr;

            llvm::Type *pointeeTy = getLLVMType(std::static_pointer_cast<PointerType>(type)->pointeeType, context, module.get());
            llvm::Value *elementPtr = builder.CreateGEP(pointeeTy, target, idx, "ptr_idx");

            if (wantAddress)
            {
                lastValue = elementPtr;
            }
            else
            {
                lastValue = builder.CreateLoad(pointeeTy, elementPtr, "ptr_val");
            }
            return;
        }

        if (!target)
            return;

        // --- OPTIONAL CHAINING LOGIC ---
        llvm::BasicBlock *continueBB = nullptr;
        llvm::BasicBlock *nullBB = nullptr;
        llvm::BasicBlock *accessBB = nullptr;
        llvm::PHINode *phi = nullptr;

        if (node.isOptional)
        {
            llvm::Function *func = builder.GetInsertBlock()->getParent();
            accessBB = llvm::BasicBlock::Create(context, "opt_idx_access", func);
            nullBB = llvm::BasicBlock::Create(context, "opt_idx_null", func);
            continueBB = llvm::BasicBlock::Create(context, "opt_idx_cont", func);

            llvm::Value *isNull = builder.CreateICmpEQ(target, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(target->getType())));
            builder.CreateCondBr(isNull, nullBB, accessBB);
            builder.SetInsertPoint(accessBB);
        }

        // 2. Evaluate Index
        bool idxSavedWantAddress = wantAddress;
        wantAddress = false;
        node.index->accept(*this);
        wantAddress = idxSavedWantAddress;

        llvm::Value *idx = lastValue;

        // 3. Resolve Types
        bool isTable = std::dynamic_pointer_cast<TableType>(type) != nullptr;
        bool isAny = std::dynamic_pointer_cast<AnyType>(type) != nullptr;
        bool isString = std::dynamic_pointer_cast<StringType>(type) != nullptr;
        bool isPointer = std::dynamic_pointer_cast<PointerType>(type) != nullptr;

        llvm::Value *result = nullptr;

        if (isFixedArray)
        {
            auto arrType = std::dynamic_pointer_cast<ArrayType>(type);
            llvm::Type *elemTy = getLLVMType(arrType->elementType, context, module.get());

            if (idx->getType()->isDoubleTy())
                idx = builder.CreateFPToSI(idx, builder.getInt32Ty());
            else if (idx->getType() != builder.getInt32Ty())
                idx = builder.CreateIntCast(idx, builder.getInt32Ty(), true);

            if (!insideUnsafe)
            {
                llvm::Function *func = builder.GetInsertBlock()->getParent();
                llvm::BasicBlock *boundsOK = llvm::BasicBlock::Create(context, "bounds_ok", func);
                llvm::BasicBlock *boundsFail = llvm::BasicBlock::Create(context, "bounds_fail", func);

                llvm::Value *sizeVal = builder.getInt32(arrType->fixedSize);

                llvm::Value *tooSmall = builder.CreateICmpSLT(idx, builder.getInt32(0));
                llvm::Value *tooBig = builder.CreateICmpSGE(idx, sizeVal);
                llvm::Value *outOfBounds = builder.CreateOr(tooSmall, tooBig);

                builder.CreateCondBr(outOfBounds, boundsFail, boundsOK);

                // FAIL BLOCK
                builder.SetInsertPoint(boundsFail);
                llvm::Function *boxString = module->getFunction("moksha_box_string");
                llvm::Function *throwEx = module->getFunction("moksha_throw_exception");
                llvm::Value *errMsg = builder.CreateGlobalString("IndexOutOfBoundsException: Array index out of range");
                llvm::Value *boxedErr = builder.CreateCall(boxString, {errMsg});
                builder.CreateCall(throwEx, {boxedErr});
                builder.CreateStore(boxedErr, globalExceptionVal);
                builder.CreateStore(builder.getInt32(1), globalExceptionFlag);

                if (!exceptionStack.empty())
                    builder.CreateBr(exceptionStack.top());
                else
                {
                    llvm::Type *retTy = func->getReturnType();
                    if (retTy->isVoidTy())
                        builder.CreateRetVoid();
                    else
                        builder.CreateRet(llvm::Constant::getNullValue(retTy));
                }

                // OK BLOCK
                builder.SetInsertPoint(boundsOK);
            }

            if (target->getType() != builder.getPtrTy())
            {
                target = builder.CreateBitCast(target, builder.getPtrTy());
            }

            llvm::Value *elementPtr = builder.CreateGEP(elemTy, target, idx, "fixed_idx");

            if (wantAddress)
            {
                result = elementPtr;
            }
            else
            {
                result = builder.CreateLoad(elemTy, elementPtr, "fixed_val");
            }
        }
        // --- UNSAFE OPTIMIZATION (Arrays Only) ---
        else if (insideUnsafe && !isTable && !isAny && !isString)
        {
            llvm::Type *targetType = target->getType();
            if (targetType->isPointerTy())
            {
                target = builder.CreateBitCast(target, builder.getPtrTy(), "array_decay");
            }
            if (isFixedArray)
            {
                std::shared_ptr<Type> elemTypePtr = nullptr;
                if (auto arr = std::dynamic_pointer_cast<ArrayType>(type))
                    elemTypePtr = arr->elementType;
                llvm::Type *llvmElemType = getLLVMType(elemTypePtr, context, module.get());

                llvm::Value *elementPtr = builder.CreateGEP(llvmElemType, target, idx, "fixed_arr_idx");
                result = builder.CreateLoad(llvmElemType, elementPtr);
            }
            else
            {
                llvm::Function *getRawPtrFunc = module->getFunction("moksha_get_raw_ptr");
                if (!getRawPtrFunc)
                {
                    // Fallback if not declared in module yet (add declaration)
                    llvm::FunctionType *ft = llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy()}, false);
                    getRawPtrFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "moksha_get_raw_ptr", module.get());
                }

                // This returns void** (pointer to the first element of the vector)
                llvm::Value *rawDataPtr = builder.CreateCall(getRawPtrFunc, {target}, "raw_vec_ptr");

                // Handle Unboxing the Index (if needed)
                if (idx->getType()->isPointerTy())
                    idx = builder.CreateCall(module->getFunction("moksha_unbox_int"), {idx});

                // NOW you can do pointer arithmetic (GEP)
                llvm::Value *elementPtr = builder.CreateGEP(builder.getPtrTy(), rawDataPtr, idx, "unsafe_idx");
                result = builder.CreateLoad(builder.getPtrTy(), elementPtr);
            }
        }
        else if (isTable || isAny)
        {
            if (idx->getType()->isIntegerTy(32))
                idx = builder.CreateCall(module->getFunction("moksha_box_int"), {idx});
            else if (idx->getType()->isDoubleTy())
                idx = builder.CreateCall(module->getFunction("moksha_box_double"), {idx});
            else if (idx->getType()->isIntegerTy(1))
            {
                llvm::Value *b = builder.CreateZExt(idx, builder.getInt32Ty());
                idx = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
            }
            if (idx->getType() != builder.getPtrTy())
                idx = builder.CreateBitCast(idx, builder.getPtrTy());

            result = builder.CreateCall(module->getFunction("moksha_table_get"), {target, idx});
        }
        else if (isString)
        {
            if (idx->getType()->isDoubleTy())
                idx = builder.CreateFPToSI(idx, builder.getInt32Ty());
            result = builder.CreateCall(llvm::FunctionCallee(fnStringGetChar), {target, idx});
        }
        else {
            // 1. Get Array Length (Inline Load at Offset 12)
            llvm::Value* rawTarget = target; 
            if (rawTarget->getType() != builder.getPtrTy()) 
                rawTarget = builder.CreateBitCast(rawTarget, builder.getPtrTy());

            llvm::Value* lenPtr = builder.CreateConstInBoundsGEP1_32(builder.getInt8Ty(), rawTarget, 12);
            llvm::Value* lenTypedPtr = builder.CreateBitCast(lenPtr, llvm::PointerType::get(context, 0));
            llvm::Value* length = builder.CreateLoad(builder.getInt32Ty(), lenTypedPtr, "arr_len");

            // 2. Normalize Index to i32
            llvm::Value* idx32 = idx;
            if (idx->getType()->isDoubleTy()) idx32 = builder.CreateFPToSI(idx, builder.getInt32Ty());
            else if (idx->getType() != builder.getInt32Ty()) idx32 = builder.CreateIntCast(idx, builder.getInt32Ty(), true);

            // 3. Create Blocks for Control Flow
            llvm::Function* func = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock* boundsOK = llvm::BasicBlock::Create(context, "bounds_ok", func);
            llvm::BasicBlock* boundsFail = llvm::BasicBlock::Create(context, "bounds_fail", func);

            // 4. Bounds Check: (idx >= length)
            llvm::Value* outOfBounds = builder.CreateICmpUGE(idx32, length);
            builder.CreateCondBr(outOfBounds, boundsFail, boundsOK);

            // 5. Fail Block: Throw Exception
            builder.SetInsertPoint(boundsFail);
            llvm::Function* throwEx = module->getFunction("moksha_throw_exception");
            llvm::Function* boxStr = module->getFunction("moksha_box_string");
            
            llvm::Value* msg = builder.CreateGlobalString("IndexOutOfBounds");
            llvm::Value* errObj = builder.CreateCall(boxStr, {msg});
            
            builder.CreateCall(throwEx, {errObj});
            builder.CreateStore(errObj, globalExceptionVal);
            builder.CreateStore(builder.getInt32(1), globalExceptionFlag);
            
            // Return dummy to satisfy LLVM block termination
            if (func->getReturnType()->isVoidTy()) builder.CreateRetVoid();
            else builder.CreateRet(llvm::Constant::getNullValue(func->getReturnType()));

            // 6. Success Block: Load Data
            builder.SetInsertPoint(boundsOK);
            llvm::Value* dataBaseAddr = builder.CreateConstInBoundsGEP1_32(builder.getInt8Ty(), rawTarget, 16);
            
            // Cast to void** (array of pointers)
            llvm::Value* dataPtr = builder.CreateBitCast(dataBaseAddr, llvm::PointerType::get(context, 0));

            // Load element at index
            llvm::Value* elemPtr = builder.CreateGEP(builder.getPtrTy(), dataPtr, idx32);
            result = builder.CreateLoad(builder.getPtrTy(), elemPtr, "fast_elem");
        }

        // 4. Handle Optional Chaining Merge
        if (node.isOptional)
        {
            if (result->getType()->isIntegerTy(32))
                result = builder.CreateCall(module->getFunction("moksha_box_int"), {result});
            if (result->getType() != builder.getPtrTy())
                result = builder.CreateBitCast(result, builder.getPtrTy());

            builder.CreateBr(continueBB);
            builder.SetInsertPoint(nullBB);
            llvm::Value *nullVal = llvm::ConstantPointerNull::get(builder.getPtrTy());
            builder.CreateBr(continueBB);
            builder.SetInsertPoint(continueBB);
            phi = builder.CreatePHI(builder.getPtrTy(), 2, "opt_idx_res");
            phi->addIncoming(nullVal, nullBB);
            phi->addIncoming(result, accessBB);
            result = phi;
        }

        lastValue = result;
    }

    void CodeGenerator::visit(LambdaNode &node)
    {
        llvm::BasicBlock *previousBlock = builder.GetInsertBlock();
        auto oldNamedValues = namedValues;
        auto oldDeferStack = deferStack;
        auto oldVariableTypes = variableTypes;
        auto oldHeapVars = heapAllocatedVars; // Save heap state

        // 1. Define Environment Struct
        std::vector<llvm::Type *> envFields;
        envFields.push_back(builder.getPtrTy()); // Parent Env

        for (auto &cap : node.captures)
        {
            // If capturing by ref (heap var), we store the POINTER
            if (cap.isByRef)
                envFields.push_back(builder.getPtrTy());
            else
                envFields.push_back(getLLVMType(cap.type, context, module.get()));
        }

        static int envCount = 0;
        llvm::StructType *envStructType = llvm::StructType::create(context, envFields, "Env_" + std::to_string(envCount++));

        // 2. Generate Lambda Function
        std::vector<llvm::Type *> argTypes;
        argTypes.push_back(builder.getPtrTy());
        for (size_t i = 0; i < node.parameters.size(); i++)
            argTypes.push_back(builder.getPtrTy());

        llvm::FunctionType *ft = llvm::FunctionType::get(builder.getPtrTy(), argTypes, false);
        static int lambdaCount = 0;
        std::string funcName = "__lambda_" + std::to_string(lambdaCount++);
        llvm::Function *lambdaFunc = llvm::Function::Create(ft, llvm::Function::InternalLinkage, funcName, module.get());

        llvm::BasicBlock *entry = llvm::BasicBlock::Create(context, "entry", lambdaFunc);
        builder.SetInsertPoint(entry);
        namedValues.clear();
        heapAllocatedVars.clear(); // Clear for inner scope
        lastValue = nullptr;

        // A. Unpack Environment
        if (!node.captures.empty())
        {
            llvm::Argument *rawEnv = lambdaFunc->getArg(0);
            llvm::Value *typedEnv = builder.CreateBitCast(rawEnv, llvm::PointerType::get(context, 0));

            int capIdx = 1;
            for (auto &cap : node.captures)
            {
                llvm::Value *fieldPtr = builder.CreateStructGEP(envStructType, typedEnv, capIdx++);

                if (cap.isByRef)
                {
                    // Ref Capture: The struct holds the POINTER to the heap variable
                    llvm::Value *heapAddr = builder.CreateLoad(builder.getPtrTy(), fieldPtr, "cap_ptr");

                    // We create a stack slot to hold this pointer
                    llvm::AllocaInst *shadowAlloca = CreateEntryBlockAlloca(lambdaFunc, cap.name, builder.getPtrTy());
                    builder.CreateStore(heapAddr, shadowAlloca);

                    namedValues[cap.name] = shadowAlloca;
                    heapAllocatedVars.insert(cap.name); // Mark as heap var in inner scope!
                }
                else
                {
                    // Value Capture (Copy)
                    llvm::Type *capType = getLLVMType(cap.type, context, module.get());
                    llvm::AllocaInst *shadowAlloca = CreateEntryBlockAlloca(lambdaFunc, cap.name, capType);
                    llvm::Value *val = builder.CreateLoad(capType, fieldPtr, "cap_val");
                    builder.CreateStore(val, shadowAlloca);
                    namedValues[cap.name] = shadowAlloca;
                }
            }
        }

        // B. Handle Parameters
        int argIdx = 1;
        for (auto &param : node.parameters)
        {
            llvm::Argument *arg = lambdaFunc->getArg(argIdx++);
            arg->setName(param->name.lexeme);

            // --- FIX START: Robust Type Detection ---
            // If Semantic Analysis didn't resolve the type, fall back to the Parser token.
            std::shared_ptr<Type> type = nullptr;
            if (param->type && param->type->resolvedType)
            {
                type = param->type->resolvedType;
            }
            else if (param->type)
            {
                // Fallback: Check token directly
                if (param->type->baseTypeToken.type == TokenType::KW_INT)
                    type = std::make_shared<IntType>();
                else if (param->type->baseTypeToken.type == TokenType::KW_DOUBLE)
                    type = std::make_shared<DoubleType>();
                else if (param->type->baseTypeToken.type == TokenType::KW_BOOLEAN)
                    type = std::make_shared<BooleanType>();
                else
                    type = std::make_shared<AnyType>();
            }
            else
            {
                type = std::make_shared<AnyType>();
            }
            // --- FIX END ---

            llvm::Value *storedVal = arg;
            llvm::Type *storageType = builder.getPtrTy();

            // Perform Unboxing based on the detected type
            if (std::dynamic_pointer_cast<IntType>(type))
            {
                storageType = builder.getInt32Ty();
                storedVal = builder.CreateCall(module->getFunction("moksha_unbox_int"), {arg});
            }
            else if (std::dynamic_pointer_cast<DoubleType>(type))
            {
                storageType = builder.getDoubleTy();
                storedVal = builder.CreateCall(module->getFunction("moksha_unbox_double"), {arg});
            }

            // ... (Rest of the parameter logic: alloca, store, namedValues) ...
            llvm::AllocaInst *alloca = CreateEntryBlockAlloca(lambdaFunc, param->name.lexeme, storageType);
            builder.CreateStore(storedVal, alloca);
            namedValues[param->name.lexeme] = alloca;
        }

        if (node.body)
            node.body->accept(*this);

        // Implicit Return Logic (Keep your existing working logic)
        if (!builder.GetInsertBlock()->getTerminator())
        {
            // If we have a value from the last statement, return it!
            if (lastValue && !lastValue->getType()->isVoidTy())
            {
                llvm::Value *retVal = lastValue;

                // BOX IT (Implicit returns need boxing too!)
                if (retVal->getType()->isIntegerTy(32))
                    retVal = builder.CreateCall(module->getFunction("moksha_box_int"), {retVal});
                else if (retVal->getType()->isDoubleTy())
                    retVal = builder.CreateCall(module->getFunction("moksha_box_double"), {retVal});
                else if (retVal->getType()->isIntegerTy(1))
                {
                    llvm::Value *b = builder.CreateZExt(retVal, builder.getInt32Ty());
                    retVal = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                }

                if (retVal->getType() != builder.getPtrTy())
                    retVal = builder.CreateBitCast(retVal, builder.getPtrTy());

                builder.CreateRet(retVal);
            }
            else
            {
                // Return null if no value produced
                builder.CreateRet(llvm::ConstantPointerNull::get(builder.getPtrTy()));
            }
        }

        builder.SetInsertPoint(previousBlock);
        namedValues = oldNamedValues;
        heapAllocatedVars = oldHeapVars; // Restore outer scope state

        // Instantiate Closure
        const llvm::DataLayout &dl = module->getDataLayout();
        uint64_t envSize = dl.getTypeAllocSize(envStructType);
        llvm::Function *mallocFunc = module->getFunction("malloc");
        llvm::Value *allocRaw = builder.CreateCall(llvm::FunctionCallee(mallocFunc), {builder.getInt64(envSize)}, "env_alloc");
        llvm::Value *allocTyped = builder.CreateBitCast(allocRaw, llvm::PointerType::get(context, 0));

        int capIdx = 1;
        for (auto &cap : node.captures)
        {
            llvm::Value *fieldPtr = builder.CreateStructGEP(envStructType, allocTyped, capIdx++);

            if (namedValues.count(cap.name))
            {
                llvm::Value *outerAddr = namedValues[cap.name];

                if (cap.isByRef)
                {
                    if (heapAllocatedVars.count(cap.name))
                    {
                        llvm::Value *heapPtr = builder.CreateLoad(builder.getPtrTy(), outerAddr);
                        builder.CreateStore(heapPtr, fieldPtr);
                    }
                    else
                    {
                        // Should not happen if Semantic Analysis is correct, but safe fallback:
                        std::cerr << "Codegen Error: Capture by ref expected for " << cap.name << " but not marked heap allocated.\n";
                    }
                }
                else
                {
                    // Value Capture
                    llvm::Type *varType = getLLVMType(cap.type, context, module.get());
                    llvm::Value *val = builder.CreateLoad(varType, outerAddr);
                    builder.CreateStore(val, fieldPtr);
                }
            }
            else
            {
                // Handle missing variables (e.g. initialize to null)
                llvm::Type *storeType = cap.isByRef ? builder.getPtrTy() : getLLVMType(cap.type, context, module.get());
                builder.CreateStore(llvm::Constant::getNullValue(storeType), fieldPtr);
            }
        }

        llvm::Function *createClosureFunc = module->getFunction("moksha_create_closure");
        llvm::Value *voidFuncPtr = builder.CreateBitCast(lambdaFunc, builder.getPtrTy());

        lastValue = builder.CreateCall(llvm::FunctionCallee(createClosureFunc),
                                       {voidFuncPtr, builder.getInt32(envSize), allocRaw},
                                       "closure");
    }

    // ---- Control Flow ---

    void CodeGenerator::visit(IfStatementNode &node)
    {
        node.condition->accept(*this);
        llvm::Value *condV = lastValue;

        if (!condV)
        {
            std::cerr << "Codegen Error: IF condition evaluated to null/void." << std::endl;
            lastValue = llvm::ConstantInt::get(context, llvm::APInt(32, 0));
            return;
        }

        // DEBUG: Ensure we aren't comparing mismatched types
        if (!condV->getType()->isIntegerTy(1))
        {
            if (std::dynamic_pointer_cast<StringType>(node.condition->resolvedType))
            {
                // Strings are true if length > 0
                llvm::Value *len = builder.CreateCall(module->getFunction("moksha_strlen"), {condV});
                condV = builder.CreateICmpNE(len, builder.getInt32(0), "str_truth");
            }
            else if (condV->getType()->isFloatingPointTy())
            {
                condV = builder.CreateFCmpONE(condV, llvm::ConstantFP::get(context, llvm::APFloat(0.0)));
            }
            else if (condV->getType()->isPointerTy())
            {
                condV = builder.CreateICmpNE(condV, llvm::Constant::getNullValue(condV->getType()));
            }
            else if (condV->getType()->isIntegerTy())
            {
                llvm::Value *zero = llvm::ConstantInt::get(condV->getType(), 0);
                condV = builder.CreateICmpNE(condV, zero);
            }
        }

        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *thenBB = llvm::BasicBlock::Create(context, "then", func);
        llvm::BasicBlock *elseBB = llvm::BasicBlock::Create(context, "else", func);
        llvm::BasicBlock *mergeBB = llvm::BasicBlock::Create(context, "ifcont", func);

        builder.CreateCondBr(condV, thenBB, elseBB);

        builder.SetInsertPoint(thenBB);
        if (node.thenBranch)
            node.thenBranch->accept(*this);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(mergeBB);

        builder.SetInsertPoint(elseBB);
        if (node.elseBranch)
            node.elseBranch->accept(*this);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(mergeBB);

        builder.SetInsertPoint(mergeBB);
    }

    void CodeGenerator::visit(WhileNode &node)
    {
        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "whilecond", func);
        llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(context, "whileloop", func);
        llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "whileafter", func);

        loopStack.push_back({condBB, afterBB});

        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);
        node.condition->accept(*this);

        // --- FIX: Logic Check ---
        llvm::Value *condV = lastValue;
        if (!condV->getType()->isIntegerTy(1))
        {
            if (condV->getType()->isFloatingPointTy())
                condV = builder.CreateFCmpONE(condV, llvm::ConstantFP::get(context, llvm::APFloat(0.0)));
            else if (condV->getType()->isPointerTy())
                condV = builder.CreateICmpNE(condV, llvm::Constant::getNullValue(condV->getType()));
            else
                condV = builder.CreateICmpNE(condV, builder.getInt32(0));
        }
        // ------------------------

        builder.CreateCondBr(condV, loopBB, afterBB);

        builder.SetInsertPoint(loopBB);
        if (node.body)
            node.body->accept(*this);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(condBB);

        builder.SetInsertPoint(afterBB);
        loopStack.pop_back();
    }

    void CodeGenerator::visit(ForNode &node)
    {
        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "forcond", func);
        llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(context, "forloop", func);
        llvm::BasicBlock *incBB = llvm::BasicBlock::Create(context, "forinc", func);
        llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "forafter", func);

        loopStack.push_back({incBB, afterBB});

        if (node.initializer)
            node.initializer->accept(*this);

        builder.CreateBr(condBB);
        builder.SetInsertPoint(condBB);
        if (node.condition)
        {
            node.condition->accept(*this);
            // --- FIX: Logic Check ---
            llvm::Value *condV = lastValue;
            if (!condV->getType()->isIntegerTy(1))
            {
                if (condV->getType()->isFloatingPointTy())
                    condV = builder.CreateFCmpONE(condV, llvm::ConstantFP::get(context, llvm::APFloat(0.0)));
                else if (condV->getType()->isPointerTy())
                    condV = builder.CreateICmpNE(condV, llvm::Constant::getNullValue(condV->getType()));
                else
                    condV = builder.CreateICmpNE(condV, builder.getInt32(0));
            }
            // ------------------------
            builder.CreateCondBr(condV, loopBB, afterBB);
        }
        else
        {
            builder.CreateBr(loopBB);
        }

        builder.SetInsertPoint(loopBB);
        if (node.body)
            node.body->accept(*this);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(incBB);

        builder.SetInsertPoint(incBB);
        if (node.increment)
            node.increment->accept(*this);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(condBB);

        builder.SetInsertPoint(afterBB);
        loopStack.pop_back();
    }

    void CodeGenerator::visit(DoWhileNode &node)
    {
        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(context, "dowhileloop", func);
        llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "dowhilecond", func);
        llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "dowhileafter", func);

        loopStack.push_back({condBB, afterBB});

        builder.CreateBr(loopBB);
        builder.SetInsertPoint(loopBB);
        if (node.body)
            node.body->accept(*this);
        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(condBB);

        builder.SetInsertPoint(condBB);
        node.condition->accept(*this);

        // --- FIX: Logic Check ---
        llvm::Value *condV = lastValue;
        if (!condV->getType()->isIntegerTy(1))
        {
            if (condV->getType()->isFloatingPointTy())
                condV = builder.CreateFCmpONE(condV, llvm::ConstantFP::get(context, llvm::APFloat(0.0)));
            else if (condV->getType()->isPointerTy())
                condV = builder.CreateICmpNE(condV, llvm::Constant::getNullValue(condV->getType()));
            else
                condV = builder.CreateICmpNE(condV, builder.getInt32(0));
        }
        // ------------------------
        builder.CreateCondBr(condV, loopBB, afterBB);

        builder.SetInsertPoint(afterBB);
        loopStack.pop_back();
    }

    void CodeGenerator::visit(SwitchStatementNode &node)
    {
        node.condition->accept(*this);
        llvm::Value *condVal = lastValue;
        llvm::Function *func = builder.GetInsertBlock()->getParent();
        llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "switchend", func);

        loopStack.push_back({nullptr, afterBB}); // Allow 'break'

        // Check if we can use a hardware switch (Integers only)
        bool isIntegerSwitch = condVal->getType()->isIntegerTy() && !condVal->getType()->isIntegerTy(1);

        // --- STRATEGY 1: Integer Switch (Efficient Jump Table) ---
        if (isIntegerSwitch)
        {
            llvm::BasicBlock *defaultBB = llvm::BasicBlock::Create(context, "default", func);

            // Pre-create blocks for fallthrough logic
            std::vector<llvm::BasicBlock *> caseBlocks;
            for (size_t i = 0; i < node.cases.size(); i++)
                caseBlocks.push_back(llvm::BasicBlock::Create(context, "case", func));

            llvm::SwitchInst *switchInst = builder.CreateSwitch(condVal, defaultBB, node.cases.size());

            for (size_t i = 0; i < node.cases.size(); ++i)
            {
                auto &caseNode = node.cases[i];
                llvm::BasicBlock *currentBB = caseBlocks[i];

                // Register Values
                if (caseNode)
                {
                    llvm::BasicBlock *saved = builder.GetInsertBlock();
                    builder.SetInsertPoint(currentBB); // Temp move to evaluate constants (hacky but works for literals)

                    caseNode->value->accept(*this);
                    llvm::ConstantInt *startVal = llvm::dyn_cast<llvm::ConstantInt>(lastValue);

                    if (caseNode->type == CaseType::RANGE && caseNode->rangeEnd)
                    {
                        caseNode->rangeEnd->accept(*this);
                        llvm::ConstantInt *endVal = llvm::dyn_cast<llvm::ConstantInt>(lastValue);
                        if (startVal && endVal)
                        {
                            int64_t s = startVal->getSExtValue();
                            int64_t e = endVal->getSExtValue();
                            for (int64_t k = s; k <= e; ++k)
                                switchInst->addCase(llvm::ConstantInt::get(builder.getInt32Ty(), k), currentBB);
                        }
                    }
                    else if (startVal)
                    {
                        switchInst->addCase(startVal, currentBB);
                    }
                    builder.SetInsertPoint(saved);
                }

                // Generate Body
                builder.SetInsertPoint(currentBB);
                for (auto &stmt : caseNode->statements)
                    stmt->accept(*this);

                // Fallthrough logic: If empty body, jump to next case. Else break.
                if (!builder.GetInsertBlock()->getTerminator())
                {
                    if (caseNode->statements.empty() && i + 1 < caseBlocks.size())
                    {
                        builder.CreateBr(caseBlocks[i + 1]);
                    }
                    else
                    {
                        builder.CreateBr(afterBB);
                    }
                }
            }

            // Default Case
            builder.SetInsertPoint(defaultBB);
            if (node.defaultCase)
            {
                for (auto &stmt : node.defaultCase->statements)
                    stmt->accept(*this);
            }
            if (!builder.GetInsertBlock()->getTerminator())
                builder.CreateBr(afterBB);
        }
        // --- STRATEGY 2: Non-Integer Switch (If-Else Cascade) ---
        else
        {
            // Used for Doubles, Strings, Pointers, Structs
            llvm::BasicBlock *nextCaseCheckBB = llvm::BasicBlock::Create(context, "check_case_0", func);
            builder.CreateBr(nextCaseCheckBB);

            for (size_t i = 0; i < node.cases.size(); ++i)
            {
                builder.SetInsertPoint(nextCaseCheckBB);
                auto &caseNode = node.cases[i];

                llvm::BasicBlock *executeCaseBB = llvm::BasicBlock::Create(context, "exec_case", func);
                llvm::BasicBlock *nextCheckBB = llvm::BasicBlock::Create(context, "check_case_" + std::to_string(i + 1), func);

                // 1. Evaluate Match Condition
                llvm::Value *isMatch = nullptr;

                caseNode->value->accept(*this);
                llvm::Value *valStart = lastValue;

                if (caseNode->type == CaseType::RANGE && caseNode->rangeEnd)
                {
                    caseNode->rangeEnd->accept(*this);
                    llvm::Value *valEnd = lastValue;

                    if (condVal->getType()->isDoubleTy())
                    {
                        llvm::Value *ge = builder.CreateFCmpOGE(condVal, valStart);
                        llvm::Value *le = builder.CreateFCmpOLE(condVal, valEnd);
                        isMatch = builder.CreateAnd(ge, le);
                    }
                    // String range support here
                    else if (condVal->getType()->isPointerTy())
                    {
                        llvm::Function *strcmpFunc = module->getFunction("strcmp");
                        if (strcmpFunc)
                        {
                            // Check: cond >= start
                            llvm::Value *res1 = builder.CreateCall(llvm::FunctionCallee(strcmpFunc), {condVal, valStart});
                            llvm::Value *ge = builder.CreateICmpSGE(res1, builder.getInt32(0));

                            // Check: cond <= end
                            llvm::Value *res2 = builder.CreateCall(llvm::FunctionCallee(strcmpFunc), {condVal, valEnd});
                            llvm::Value *le = builder.CreateICmpSLE(res2, builder.getInt32(0));

                            isMatch = builder.CreateAnd(ge, le);
                        }
                    }
                }
                else
                {
                    // Equality Logic
                    if (condVal->getType()->isDoubleTy())
                        isMatch = builder.CreateFCmpOEQ(condVal, valStart);
                    else if (condVal->getType()->isPointerTy())
                    {
                        // Use strcmp for strings
                        llvm::Function *strcmpFunc = module->getFunction("strcmp");
                        if (strcmpFunc)
                        {
                            llvm::Value *cmpRes = builder.CreateCall(llvm::FunctionCallee(strcmpFunc), {condVal, valStart});
                            isMatch = builder.CreateICmpEQ(cmpRes, builder.getInt32(0), "strs_eq");
                        }
                        else
                        {
                            // Fallback to pointer equality
                            isMatch = builder.CreateICmpEQ(condVal, valStart);
                        }
                    }
                    else
                        isMatch = builder.CreateICmpEQ(condVal, valStart);
                }

                if (!isMatch)
                    isMatch = builder.getInt1(false); // Safety fallback

                builder.CreateCondBr(isMatch, executeCaseBB, nextCheckBB);

                // 2. Generate Body
                builder.SetInsertPoint(executeCaseBB);
                for (auto &stmt : caseNode->statements)
                    stmt->accept(*this);

                if (!builder.GetInsertBlock()->getTerminator())
                    builder.CreateBr(afterBB);

                // Prepare next iteration
                nextCaseCheckBB = nextCheckBB;
            }

            // Default Case logic (at the end of the chain)
            builder.SetInsertPoint(nextCaseCheckBB);
            if (node.defaultCase)
            {
                for (auto &stmt : node.defaultCase->statements)
                    stmt->accept(*this);
            }
            if (!builder.GetInsertBlock()->getTerminator())
                builder.CreateBr(afterBB);
        }

        builder.SetInsertPoint(afterBB);
        loopStack.pop_back();
    }

    void CodeGenerator::visit(BreakStatementNode &node)
    {
        if (!loopStack.empty())
            builder.CreateBr(loopStack.back().afterBB);
    }

    void CodeGenerator::visit(ContinueStatementNode &node)
    {
        if (!loopStack.empty())
            builder.CreateBr(loopStack.back().checkBB);
    }

    void CodeGenerator::visit(ForInNode &node)
    {
        llvm::Function *func = builder.GetInsertBlock()->getParent();

        // 1. Evaluate the Iterable Expression
        node.iterable->accept(*this);
        llvm::Value *iterablePtr = lastValue; // The original Array, String, or Table
        std::shared_ptr<Type> type = node.iterable->resolvedType;

        auto arrType = std::dynamic_pointer_cast<ArrayType>(type);
        if (arrType && arrType->isFixedSize)
        {
            // 1. Setup Blocks
            llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "fast_cond", func);
            llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "fast_body", func);
            llvm::BasicBlock *incBB = llvm::BasicBlock::Create(context, "fast_inc", func);
            llvm::BasicBlock *endBB = llvm::BasicBlock::Create(context, "fast_end", func);
            loopStack.push_back({incBB, endBB});

            // 2. Loop Variable (int i = 0)
            llvm::Value *idxVar = CreateEntryBlockAlloca(func, "fast_idx", builder.getInt32Ty());
            builder.CreateStore(builder.getInt32(0), idxVar);
            builder.CreateBr(condBB);

            // 3. Condition (i < CONSTANT)
            builder.SetInsertPoint(condBB);
            llvm::Value *currIdx = builder.CreateLoad(builder.getInt32Ty(), idxVar);
            llvm::Value *limit = builder.getInt32(arrType->fixedSize); // Constant!
            llvm::Value *cond = builder.CreateICmpSLT(currIdx, limit);
            builder.CreateCondBr(cond, bodyBB, endBB);

            // 4. Body (Pure Pointer Math)
            builder.SetInsertPoint(bodyBB);

            // GEP: base[i]
            llvm::Type *elemTy = getLLVMType(arrType->elementType, context, module.get());
            llvm::Value *elemPtr = builder.CreateGEP(elemTy, iterablePtr, currIdx, "fast_ptr");
            llvm::Value *val = builder.CreateLoad(elemTy, elemPtr, "fast_val");

            // 5. Assign to User Vars
            if (node.hasValueVar)
            {
                // for (idx, val in arr)
                llvm::AllocaInst *kAlloc = CreateEntryBlockAlloca(func, node.keyVar.lexeme, builder.getInt32Ty());
                builder.CreateStore(currIdx, kAlloc);
                namedValues[node.keyVar.lexeme] = kAlloc;

                llvm::AllocaInst *vAlloc = CreateEntryBlockAlloca(func, node.valVar.lexeme, val->getType());
                builder.CreateStore(val, vAlloc);
                namedValues[node.valVar.lexeme] = vAlloc;
            }
            else
            {
                // for (val in arr)
                llvm::AllocaInst *vAlloc = CreateEntryBlockAlloca(func, node.keyVar.lexeme, val->getType());
                builder.CreateStore(val, vAlloc);
                namedValues[node.keyVar.lexeme] = vAlloc;
            }

            node.body->accept(*this);
            builder.CreateBr(incBB);

            // 6. Increment
            builder.SetInsertPoint(incBB);
            llvm::Value *nextIdx = builder.CreateAdd(currIdx, builder.getInt32(1));
            builder.CreateStore(nextIdx, idxVar);
            builder.CreateBr(condBB);

            // 7. Cleanup
            builder.SetInsertPoint(endBB);
            loopStack.pop_back();
            return; // DONE! Skip the slow path.
        }

        bool isString = std::dynamic_pointer_cast<StringType>(type) != nullptr;
        bool isTable = std::dynamic_pointer_cast<TableType>(type) != nullptr;

        llvm::Value *loopTargetPtr = iterablePtr;

        if (isTable)
        {
            llvm::Function *capFunc = module->getFunction("moksha_table_capacity");
            if (!capFunc)
            {
                // Register if missing
                llvm::FunctionType *ft = llvm::FunctionType::get(builder.getInt32Ty(), {builder.getPtrTy()}, false);
                capFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "moksha_table_capacity", module.get());
            }

            llvm::Function *getKeyFunc = module->getFunction("moksha_table_get_entry_key");
            if (!getKeyFunc)
            {
                llvm::FunctionType *ft = llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt32Ty()}, false);
                getKeyFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "moksha_table_get_entry_key", module.get());
            }

            llvm::Function *getValFunc = module->getFunction("moksha_table_get_entry_val");
            if (!getValFunc)
            {
                llvm::FunctionType *ft = llvm::FunctionType::get(builder.getPtrTy(), {builder.getPtrTy(), builder.getInt32Ty()}, false);
                getValFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, "moksha_table_get_entry_val", module.get());
            }

            llvm::Value *cap = builder.CreateCall(capFunc, {iterablePtr}, "tbl_cap");

            // Define Blocks
            llvm::AllocaInst *idxVar = CreateEntryBlockAlloca(func, "tbl_idx", builder.getInt32Ty());
            builder.CreateStore(builder.getInt32(0), idxVar);

            llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "tbl_cond", func);
            llvm::BasicBlock *checkBB = llvm::BasicBlock::Create(context, "tbl_check", func); // Check if bucket occupied
            llvm::BasicBlock *bodyBB = llvm::BasicBlock::Create(context, "tbl_body", func);
            llvm::BasicBlock *incBB = llvm::BasicBlock::Create(context, "tbl_inc", func);
            llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "tbl_after", func);

            loopStack.push_back({incBB, afterBB});

            builder.CreateBr(condBB);

            // 1. Loop Condition (i < capacity)
            builder.SetInsertPoint(condBB);
            llvm::Value *currIdx = builder.CreateLoad(builder.getInt32Ty(), idxVar);
            llvm::Value *cmp = builder.CreateICmpSLT(currIdx, cap);
            builder.CreateCondBr(cmp, checkBB, afterBB);

            // 2. Check Bucket (key != null)
            builder.SetInsertPoint(checkBB);
            llvm::Value *keyPtr = builder.CreateCall(getKeyFunc, {iterablePtr, currIdx});
            llvm::Value *isOccupied = builder.CreateICmpNE(keyPtr, llvm::ConstantPointerNull::get(builder.getPtrTy()));
            builder.CreateCondBr(isOccupied, bodyBB, incBB); // If empty, skip to increment

            // 3. Loop Body
            builder.SetInsertPoint(bodyBB);
            llvm::Value *valPtr = builder.CreateCall(getValFunc, {iterablePtr, currIdx});

            if (node.hasValueVar)
            {
                // for (k, v in tab)
                llvm::AllocaInst *kAlloc = CreateEntryBlockAlloca(func, node.keyVar.lexeme, builder.getPtrTy());
                builder.CreateStore(keyPtr, kAlloc);
                namedValues[node.keyVar.lexeme] = kAlloc;

                llvm::AllocaInst *vAlloc = CreateEntryBlockAlloca(func, node.valVar.lexeme, builder.getPtrTy());
                builder.CreateStore(valPtr, vAlloc);
                namedValues[node.valVar.lexeme] = vAlloc;
            }
            else
            {
                // for (k in tab)
                llvm::AllocaInst *kAlloc = CreateEntryBlockAlloca(func, node.keyVar.lexeme, builder.getPtrTy());
                builder.CreateStore(keyPtr, kAlloc); // Only Key
                namedValues[node.keyVar.lexeme] = kAlloc;
            }

            node.body->accept(*this);

            if (!builder.GetInsertBlock()->getTerminator())
                builder.CreateBr(incBB);

            // 4. Increment
            builder.SetInsertPoint(incBB);
            llvm::Value *nextIdx = builder.CreateAdd(currIdx, builder.getInt32(1));
            builder.CreateStore(nextIdx, idxVar);
            builder.CreateBr(condBB);

            // 5. End
            builder.SetInsertPoint(afterBB);
            loopStack.pop_back();
            return;
        }

        // 2. Get Length of the Loop Target
        llvm::Value *len = nullptr;
        if (isString)
        {
            llvm::Function *strlenFunc = module->getFunction("moksha_strlen");
            len = builder.CreateCall(llvm::FunctionCallee(strlenFunc), {loopTargetPtr}, "str_len");
        }
        else
        {
            // Arrays (and the Keys Array for tables) use standard length
            llvm::Function *lenFunc = module->getFunction("moksha_get_length");
            len = builder.CreateCall(llvm::FunctionCallee(lenFunc), {loopTargetPtr}, "len");
        }

        // 3. Set up Loop Structure (Basic Blocks)
        llvm::AllocaInst *idxVar = CreateEntryBlockAlloca(func, "forin_idx", builder.getInt32Ty());
        builder.CreateStore(builder.getInt32(0), idxVar);

        llvm::BasicBlock *condBB = llvm::BasicBlock::Create(context, "forin_cond", func);
        llvm::BasicBlock *loopBB = llvm::BasicBlock::Create(context, "forin_body", func);
        llvm::BasicBlock *incBB = llvm::BasicBlock::Create(context, "forin_inc", func);
        llvm::BasicBlock *afterBB = llvm::BasicBlock::Create(context, "forin_after", func);
        loopStack.push_back({incBB, afterBB});

        builder.CreateBr(condBB);

        // -- Condition Block --
        builder.SetInsertPoint(condBB);
        llvm::Value *currIdx = builder.CreateLoad(builder.getInt32Ty(), idxVar);
        builder.CreateCondBr(builder.CreateICmpSLT(currIdx, len), loopBB, afterBB);

        // -- Loop Body Block --
        builder.SetInsertPoint(loopBB);

        llvm::Value *keyVal = nullptr; // Represents Index (Array/String) or Key (Table)
        llvm::Value *valVal = nullptr; // Represents Value

        if (isString)
        {
            keyVal = currIdx; // Index
            llvm::Function *getCharFunc = module->getFunction("moksha_string_get_char");
            valVal = builder.CreateCall(llvm::FunctionCallee(getCharFunc), {loopTargetPtr, currIdx}, "char_val");
        }
        else if (isTable)
        {
            // 1. Get Key from Keys Array
            llvm::Function *arrGet = module->getFunction("moksha_array_get");
            keyVal = builder.CreateCall(llvm::FunctionCallee(arrGet), {loopTargetPtr, currIdx}, "key_val");
            // 2. Get Value from Original Table using Key
            llvm::Function *tblGet = module->getFunction("moksha_table_get");
            valVal = builder.CreateCall(llvm::FunctionCallee(tblGet), {iterablePtr, keyVal}, "real_val");
        }
        else
        {
            // Dynamic Array
            keyVal = currIdx;
            llvm::Value *rawArrPtr = loopTargetPtr;
            if (rawArrPtr->getType() != builder.getPtrTy())
                rawArrPtr = builder.CreateBitCast(rawArrPtr, builder.getPtrTy());

            // 1. Get address of 'data' field at offset 16
            llvm::Value *dataFieldAddr = builder.CreateConstInBoundsGEP1_32(builder.getInt8Ty(), rawArrPtr, 16);
            
            // 2. Cast to void** (because 'data' is a flexible array of void*)
            llvm::Value *dataFieldPtr = builder.CreateBitCast(dataFieldAddr, llvm::PointerType::get(context, 0));

            // 3. Get the element at 'currIdx' directly
            llvm::Value *elemPtr = builder.CreateGEP(builder.getPtrTy(), dataFieldPtr, currIdx, "fast_elem_ptr");
            valVal = builder.CreateLoad(builder.getPtrTy(), elemPtr, "fast_val");

            if (arrType)
            {
                if (std::dynamic_pointer_cast<IntType>(arrType->elementType))
                    valVal = builder.CreateCall(module->getFunction("moksha_unbox_int"), {valVal}, "unbox_i");
                else if (std::dynamic_pointer_cast<DoubleType>(arrType->elementType))
                    valVal = builder.CreateCall(module->getFunction("moksha_unbox_double"), {valVal}, "unbox_d");
                else if (std::dynamic_pointer_cast<BooleanType>(arrType->elementType))
                {
                    llvm::Value *b = builder.CreateCall(module->getFunction("moksha_unbox_bool"), {valVal}, "unbox_b");
                    valVal = builder.CreateTrunc(b, builder.getInt1Ty());
                }
            }
        }

        // 4. Assign to User Variables
        if (node.hasValueVar)
        {
            // Syntax: for (k, v in x)
            llvm::AllocaInst *kAlloc = CreateEntryBlockAlloca(func, node.keyVar.lexeme, keyVal->getType());
            builder.CreateStore(keyVal, kAlloc);
            namedValues[node.keyVar.lexeme] = kAlloc;
            llvm::AllocaInst *vAlloc = CreateEntryBlockAlloca(func, node.valVar.lexeme, valVal->getType());
            builder.CreateStore(valVal, vAlloc);
            namedValues[node.valVar.lexeme] = vAlloc;
        }
        else
        {
            // Syntax: for (x in y)
            // Table -> x is Key. Array/String -> x is Value.
            llvm::Value *singleVarVal = isTable ? keyVal : valVal;
            llvm::AllocaInst *varAlloc = CreateEntryBlockAlloca(func, node.keyVar.lexeme, singleVarVal->getType());
            builder.CreateStore(singleVarVal, varAlloc);
            namedValues[node.keyVar.lexeme] = varAlloc;
        }

        // Generate User Body
        node.body->accept(*this);

        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateBr(incBB);

        // -- Increment Block --
        builder.SetInsertPoint(incBB);
        llvm::Value *nextIdx = builder.CreateAdd(currIdx, builder.getInt32(1));
        builder.CreateStore(nextIdx, idxVar);
        builder.CreateBr(condBB);

        // -- After Block --
        builder.SetInsertPoint(afterBB);
        loopStack.pop_back();
    }

    // --- Object Oriented ---

    void CodeGenerator::visit(ClassDefinitionNode &node)
    {
        std::string className = node.name.lexeme;
        std::vector<llvm::Type *> elementTypes;
        std::map<std::string, int> indices;
        int currentIndex = 0;

        classFieldInitializers[className].clear();

        // --- 1. Header: VPointer (Index 0) ---
        // Every object starts with a pointer to its VTable
        elementTypes.push_back(builder.getPtrTy()); // void* vptr
        currentIndex++;

        // --- 2. Flatten Parent Fields ---
        // We copy fields from parents to ensure memory layout compatibility
        for (auto &parentVar : node.parentClassNames)
        {
            std::string parentName = parentVar->name.lexeme;
            if (classLayouts.find(parentName) == classLayouts.end())
                continue;

            std::vector<llvm::Type *> parentFields = classLayouts[parentName];

            // Start from index 1 (Skip parent's vptr, we manage our own)
            for (size_t i = 1; i < parentFields.size(); i++)
            {
                elementTypes.push_back(parentFields[i]);
            }

            // Map parent field names to child indices
            for (auto const &[fieldName, fieldIdx] : classFieldIndices[parentName])
            {
                indices[fieldName] = currentIndex + (fieldIdx - 1);
            }
            currentIndex += (parentFields.size() - 1);
        }

        std::string oldClassName = currentClassName;
        currentClassName = className;
        bool hasUserConstructor = false;

        // --- 3. Add Own Fields ---
        for (auto &member : node.members)
        {
            if (auto var = dynamic_cast<VariableDeclarationNode *>(member.get()))
            {
                std::shared_ptr<Type> resolved = var->type->resolvedType;
                if (!resolved)
                    resolved = std::make_shared<IntType>();
                elementTypes.push_back(getLLVMType(resolved, context, module.get()));
                indices[var->name.lexeme] = currentIndex++;
                if (var->initializer)
                {
                    classFieldInitializers[className].push_back(var);
                }
            }
        }

        // --- 4. Create LLVM Struct ---
        llvm::StructType *st = llvm::StructType::create(context, className);
        st->setBody(elementTypes);

        // Save Globals
        classFieldIndices[className] = indices;
        classLayouts[className] = elementTypes;
        isRefClassMap[node.name.lexeme] = node.isRef;

        // --- 5. Generate VTable ---
        const auto &vtableEntries = SemanticAnalyzer::getVTable(className);
        std::vector<llvm::Constant *> vtableFuncs;

        for (const auto &entry : vtableEntries)
        {
            llvm::Function *impl = module->getFunction(entry.implName);

            if (!impl)
            {
                // If function body isn't generated yet, create a Prototype
                // We use the signature stored in the VTableEntry
                llvm::FunctionType *ft = getLLVMFunctionProto(entry.signature);
                impl = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, entry.implName, module.get());
            }

            // Cast function to generic i8* (void*) for storage in the array
            vtableFuncs.push_back(llvm::ConstantExpr::getBitCast(impl, builder.getPtrTy()));
        }

        // Create the VTable Global Variable
        if (!vtableFuncs.empty())
        {
            llvm::ArrayType *vtableType = llvm::ArrayType::get(builder.getPtrTy(), vtableFuncs.size());
            llvm::Constant *vtableInit = llvm::ConstantArray::get(vtableType, vtableFuncs);

            std::string vtableName = "VTable_" + className;
            new llvm::GlobalVariable(*module, vtableType, true,
                                     llvm::GlobalValue::ExternalLinkage, vtableInit, vtableName);
        }

        // --- 6. Generate Method Bodies ---
        for (auto &member : node.members)
        {
            if (dynamic_cast<VariableDeclarationNode *>(member.get()))
                continue;
            if (member)
                member->accept(*this);
        }
        if (!hasUserConstructor)
        {
            std::string parentName = "";
            if (!node.parentClassNames.empty())
            {
                parentName = node.parentClassNames[0]->name.lexeme;
            }
            // [CHANGED] Pass parentName to the generator
            generateDefaultConstructor(className, parentName, node.members);
        }
        currentClassName = oldClassName;
    }

    void CodeGenerator::visit(StructDefinitionNode &node)
    {
        std::string structName = node.name.lexeme;

        llvm::StructType *structType = llvm::StructType::create(context, structName);
        std::vector<llvm::Type *> body;
        std::map<std::string, int> indices;
        int idx = 0;
        int currentBitOffset = 0;
        llvm::Type *currentContainerType = nullptr;

        for (auto &member : node.members)
        {
            if (auto varDecl = dynamic_cast<VariableDeclarationNode *>(member.get()))
            {
                llvm::Type *fieldType = getLLVMType(varDecl->type->resolvedType, context, module.get());

                if (varDecl->bitWidth > 0)
                {
                    // If it's a new bitfield group or the container type changed
                    if (currentContainerType == nullptr || currentBitOffset + varDecl->bitWidth > (currentContainerType->getIntegerBitWidth()))
                    {
                        body.push_back(fieldType);
                        currentContainerType = fieldType;
                        currentBitOffset = 0;
                        // We increment idx only when we actually push a new field to body
                        if (currentContainerType != nullptr && currentBitOffset > 0)
                            idx++;
                    }

                    BitfieldMeta meta;
                    meta.bitOffset = currentBitOffset;
                    meta.bitWidth = varDecl->bitWidth;
                    structBitfields[structName][varDecl->name.lexeme] = meta;

                    // Map the member name to the CURRENT container index
                    indices[varDecl->name.lexeme] = body.size() - 1;

                    currentBitOffset += varDecl->bitWidth;
                }
                else
                {
                    // Normal field
                    body.push_back(fieldType);
                    indices[varDecl->name.lexeme] = body.size() - 1;
                    currentBitOffset = 0;
                    currentContainerType = nullptr;
                }
            }
        }

        if (node.isUnion && !body.empty())
        {
            llvm::Type *largestType = body[0];
            uint64_t maxSize = module->getDataLayout().getTypeAllocSize(largestType);
            for (auto *t : body)
            {
                if (module->getDataLayout().getTypeAllocSize(t) > maxSize)
                    largestType = t;
            }
            structType->setBody({largestType}, node.isPacked);
        }
        else
        {
            structType->setBody(body, node.isPacked);
        }

        structLLVMTypes[structName] = structType;
        structFieldIndices[structName] = indices;
        structIsUnion[structName] = node.isUnion;
    }

    // 2. ADD visit(AsmExpressionNode)
    void CodeGenerator::visit(AsmExpressionNode &node)
    {
        // A. Resolve Return Type
        llvm::Type *retType = builder.getVoidTy();
        if (node.returnType)
        {
            if (node.returnType->resolvedType)
                retType = getLLVMType(node.returnType->resolvedType, context, module.get());
            else
            {
                if (node.returnType->baseTypeToken.type == TokenType::KW_INT)
                    retType = builder.getInt32Ty();
                else if (node.returnType->baseTypeToken.type == TokenType::KW_DOUBLE)
                    retType = builder.getDoubleTy();
            }
        }

        // B. Prepare Arguments
        std::vector<llvm::Type *> argTypes;
        std::vector<llvm::Value *> argValues; // Use this for the actual arguments

        for (auto &arg : node.arguments)
        {
            arg->accept(*this);
            llvm::Value *val = lastValue;

            if (std::dynamic_pointer_cast<ShortType>(arg->resolvedType))
                val = builder.CreateTrunc(val, builder.getInt16Ty());
            else if (std::dynamic_pointer_cast<CharType>(arg->resolvedType))
                val = builder.CreateTrunc(val, builder.getInt8Ty());

            argValues.push_back(val);
            argTypes.push_back(val->getType());
        }

        // 1. Get the FunctionType (ft)
        llvm::FunctionType *ft = llvm::FunctionType::get(retType, argTypes, false);

        // 2. Get the InlineAsm object
        llvm::InlineAsm *ia = llvm::InlineAsm::get(ft, node.assemblyTemplate, node.constraints, node.hasSideEffects);

        if (retType->isVoidTy())
        {
            builder.CreateCall(ft, ia, argValues);
        }
        else
        {
            lastValue = builder.CreateCall(ft, ia, argValues, "asm_res");
        }
    }

    void CodeGenerator::visit(NewExpression &node)
    {
        if (isFreestanding)
        {
            std::cerr << "[Error] 'print' statement is disabled in freestanding mode. Use a VGA driver or 'outb'.\n";
            return;
        }

        // Use calloc instead of malloc for zero-initialization
        llvm::Function *callocFunc = module->getFunction("calloc");
        llvm::StructType *st = llvm::StructType::getTypeByName(context, node.className.lexeme);
        if (!st)
        {
            std::cerr << "Codegen Error: Class '" << node.className.lexeme << "' not found.\n";
            lastValue = llvm::ConstantPointerNull::get(llvm::PointerType::get(context, 0));
            return;
        }
        const llvm::DataLayout &dl = module->getDataLayout();
        uint64_t sizeBytes = dl.getTypeAllocSize(st);

        // calloc(1, size)
        llvm::Value *ptr = builder.CreateCall(llvm::FunctionCallee(callocFunc), {builder.getInt64(1), builder.getInt64(sizeBytes)}, "alloc_obj");
        llvm::Value *objPtr = builder.CreateBitCast(ptr, llvm::PointerType::get(context, 0));

        std::string vtableName = "VTable_" + node.className.lexeme;
        llvm::GlobalVariable *vtableGlobal = module->getNamedGlobal(vtableName);

        if (vtableGlobal)
        {
            llvm::Value *vptrSlot = builder.CreateStructGEP(st, objPtr, 0);
            llvm::Value *vtableStart = builder.CreateConstInBoundsGEP2_32(vtableGlobal->getValueType(), vtableGlobal, 0, 0);
            builder.CreateStore(vtableStart, vptrSlot);
        }

        std::string ctorName = node.className.lexeme + "_constructor";
        llvm::Function *ctor = module->getFunction(ctorName);
        if (ctor)
        {
            std::vector<llvm::Value *> args;
            args.push_back(objPtr);
            for (auto &arg : node.arguments)
            {
                arg->accept(*this);
                args.push_back(lastValue);
            }
            builder.CreateCall(llvm::FunctionCallee(ctor), args);
        }
        lastValue = objPtr;
    }

    void CodeGenerator::visit(MemberAccessNode &node)
    {
        // 1. Scoped Enum Optimization (Compile-time Constant)
        if (auto varNode = dynamic_cast<VariableNode *>(node.object.get()))
        {
            std::string typeName = varNode->name.lexeme;
            std::string memberName = node.name.lexeme;
            if (scopedEnumConstants.count(typeName) && scopedEnumConstants[typeName].count(memberName))
            {
                lastValue = builder.getInt32(scopedEnumConstants[typeName][memberName]);
                return;
            }
        }

        // 2. Evaluate the Object to get the base pointer
        bool oldWantAddress = wantAddress;
        wantAddress = true;
        node.object->accept(*this);
        llvm::Value *objPtr = lastValue;
        wantAddress = oldWantAddress;

        if (!objPtr)
            return;

        // 3. Resolve Struct/Class Metadata
        std::string structName;
        bool isPointer = false;

        if (auto st = std::dynamic_pointer_cast<StructType>(node.object->resolvedType))
        {
            structName = st->structName;
        }
        else if (auto ptr = std::dynamic_pointer_cast<PointerType>(node.object->resolvedType))
        {
            if (auto st = std::dynamic_pointer_cast<StructType>(ptr->pointeeType))
            {
                structName = st->structName;
                isPointer = true;
            }
        }

        // 4. Handle Hardware Structs / Bitfields / Unions
        if (!structName.empty() && structLLVMTypes.count(structName))
        {
            llvm::StructType *stType = structLLVMTypes[structName];
            int fieldIdx = structFieldIndices[structName][node.name.lexeme];
            llvm::Value *fieldPtr = nullptr;

            if (structIsUnion[structName])
            {
                fieldPtr = builder.CreateBitCast(objPtr, builder.getPtrTy(), "union_ptr");
            }
            else
            {
                if (fieldIdx >= (int)stType->getStructNumElements())
                    fieldIdx = 0;
                fieldPtr = builder.CreateStructGEP(stType, objPtr, fieldIdx, "member_ptr");
            }

            // Define memberType so it is available for the logic below
            llvm::Type *memberType = nullptr;

            if (structIsUnion[structName])
            {
                auto highLevelType = SemanticAnalyzer::structFieldTypes[structName][node.name.lexeme];
                memberType = getLLVMType(highLevelType, context, module.get());
            }
            else
            {
                // For regular structs, the LLVM type layout matches the fields.
                memberType = stType->getStructElementType(fieldIdx);
            }

            // Bitfield Extraction
            auto &meta = structBitfields[structName][node.name.lexeme];
            if (meta.bitWidth > 0)
            {
                // If the caller (BinaryOpNode) wants the ADDRESS, return the container address directly.
                if (wantAddress)
                {
                    lastValue = fieldPtr;
                    return;
                }

                llvm::Value *container = builder.CreateLoad(memberType, fieldPtr, "bf_container");
                llvm::Value *shifted = builder.CreateLShr(container, meta.bitOffset);
                uint64_t maskValue = (1ULL << meta.bitWidth) - 1;
                lastValue = builder.CreateAnd(shifted, llvm::ConstantInt::get(memberType, maskValue));
                return;
            }

            // Standard Access
            lastValue = wantAddress ? fieldPtr : builder.CreateLoad(memberType, fieldPtr, "member_val");
            return;
        }

        // Safety check
        if (!objPtr)
            return;

        if (std::dynamic_pointer_cast<ClassInstanceType>(node.object->resolvedType) ||
            std::dynamic_pointer_cast<TableType>(node.object->resolvedType) ||
            std::dynamic_pointer_cast<StringType>(node.object->resolvedType) ||
            std::dynamic_pointer_cast<AnyType>(node.object->resolvedType) ||
            std::dynamic_pointer_cast<ArrayType>(node.object->resolvedType))
        {
            // --- FIX START ---
            // Only load if the expression yields the ADDRESS of the reference (Variable, Field, Array Slot).
            // Do NOT load if it yields the VALUE of the reference (this, new, call).
            bool isLValue = dynamic_cast<VariableNode *>(node.object.get()) ||
                            dynamic_cast<IndexNode *>(node.object.get()) ||
                            dynamic_cast<MemberAccessNode *>(node.object.get());

            if (isLValue)
            {
                llvm::Type *objType = getLLVMType(node.object->resolvedType, context, module.get());
                objPtr = builder.CreateLoad(objType, objPtr, "loaded_obj_ptr");
            }
            // --- FIX END ---
        }

        // --- OPTIONAL CHAINING SETUP (?. operator) ---
        llvm::BasicBlock *continueBB = nullptr;
        llvm::BasicBlock *nullBB = nullptr;
        llvm::BasicBlock *accessBB = nullptr;
        llvm::PHINode *phi = nullptr;

        if (node.isOptional)
        {
            llvm::Function *func = builder.GetInsertBlock()->getParent();
            accessBB = llvm::BasicBlock::Create(context, "opt_mem_access", func);
            nullBB = llvm::BasicBlock::Create(context, "opt_mem_null", func);
            continueBB = llvm::BasicBlock::Create(context, "opt_mem_cont", func);

            llvm::Value *isNull = builder.CreateICmpEQ(objPtr, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(objPtr->getType())));
            builder.CreateCondBr(isNull, nullBB, accessBB);

            builder.SetInsertPoint(accessBB);
        }
        // --- SAFETY CHECK (Standard . operator) ---
        // [FIX] Only generate the Null Check if we are SAFE (!insideUnsafe)
        else if (!insideUnsafe)
        {
            llvm::Function *func = builder.GetInsertBlock()->getParent();
            llvm::BasicBlock *errorBB = llvm::BasicBlock::Create(context, "mem_error", func);
            llvm::BasicBlock *safeBB = llvm::BasicBlock::Create(context, "mem_safe", func);

            llvm::Value *isNull = builder.CreateICmpEQ(objPtr, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(objPtr->getType())));
            builder.CreateCondBr(isNull, errorBB, safeBB);

            // Error Path (Throw Exception)
            builder.SetInsertPoint(errorBB);
            llvm::Function *boxStringFunc = module->getFunction("moksha_box_string");
            llvm::Value *msg = builder.CreateGlobalString("NullReferenceException: Attempted access on null object");
            llvm::Value *boxedMsg = builder.CreateCall(boxStringFunc, {msg});

            builder.CreateStore(boxedMsg, globalExceptionVal);
            builder.CreateStore(builder.getInt32(1), globalExceptionFlag);

            if (!exceptionStack.empty())
                builder.CreateBr(exceptionStack.top());
            else
            {
                if (func->getReturnType()->isVoidTy())
                    builder.CreateRetVoid();
                else
                    builder.CreateRet(llvm::Constant::getNullValue(func->getReturnType()));
            }

            // Safe Path
            builder.SetInsertPoint(safeBB);
        }

        // --- 3. MEMBER ACCESS LOGIC ---
        llvm::Value *result = nullptr;
        std::string memberName = node.name.lexeme;

        if (memberName == "length")
        {
            if (std::dynamic_pointer_cast<StringType>(node.object->resolvedType))
                result = builder.CreateCall(module->getFunction("moksha_strlen"), {objPtr});
            else
                result = builder.CreateCall(module->getFunction("moksha_get_length"), {objPtr});

            if (node.isOptional)
                result = builder.CreateCall(module->getFunction("moksha_box_int"), {result});
        }
        else if (std::dynamic_pointer_cast<TableType>(node.object->resolvedType))
        {
            llvm::Value *keyStr = builder.CreateGlobalString(memberName);
            llvm::Value *boxedKey = builder.CreateCall(module->getFunction("moksha_box_string"), {keyStr});
            result = builder.CreateCall(module->getFunction("moksha_table_get"), {objPtr, boxedKey}, "table_dot_get");
        }
        else
        {
            std::string className = "";
            if (auto varNode = dynamic_cast<VariableNode *>(node.object.get()))
            {
                if (variableTypes.count(varNode->name.lexeme))
                    className = variableTypes[varNode->name.lexeme];
            }
            else if (dynamic_cast<ThisExpression *>(node.object.get()))
            {
                className = currentClassName;
            }
            else if (auto newExpr = dynamic_cast<NewExpression *>(node.object.get()))
            {
                className = newExpr->className.lexeme;
            }
            else if (auto clsType = std::dynamic_pointer_cast<ClassInstanceType>(node.object->resolvedType))
            {
                className = clsType->className;
            }

            if (!className.empty() && classFieldIndices.count(className))
            {
                llvm::StructType *st = llvm::StructType::getTypeByName(context, className);
                llvm::PointerType *structPtrType = llvm::PointerType::get(st->getContext(), 0);

                if (objPtr->getType() != structPtrType)
                    objPtr = builder.CreateBitCast(objPtr, structPtrType);

                int idx = classFieldIndices[className][memberName];
                llvm::Value *fieldPtr = builder.CreateStructGEP(st, objPtr, idx);

                if (wantAddress)
                {
                    result = fieldPtr;
                }
                else
                {
                    llvm::Type *actualFieldType = st->getElementType(idx);
                    result = builder.CreateLoad(actualFieldType, fieldPtr);

                    if (node.isOptional)
                    {
                        if (result->getType()->isIntegerTy(32))
                            result = builder.CreateCall(module->getFunction("moksha_box_int"), {result});
                        else if (result->getType()->isDoubleTy())
                            result = builder.CreateCall(module->getFunction("moksha_box_double"), {result});
                        else if (result->getType()->isIntegerTy(1))
                        {
                            llvm::Value *b = builder.CreateZExt(result, builder.getInt32Ty());
                            result = builder.CreateCall(module->getFunction("moksha_box_bool"), {b});
                        }
                    }
                }
            }
            else
            {
                result = llvm::ConstantPointerNull::get(builder.getPtrTy());
            }
        }

        // --- 4. OPTIONAL MERGE ---
        if (node.isOptional)
        {
            if (result->getType() != builder.getPtrTy())
                result = builder.CreateBitCast(result, builder.getPtrTy());

            builder.CreateBr(continueBB);
            builder.SetInsertPoint(nullBB);
            llvm::Value *nullVal = llvm::ConstantPointerNull::get(builder.getPtrTy());
            builder.CreateBr(continueBB);
            builder.SetInsertPoint(continueBB);
            phi = builder.CreatePHI(builder.getPtrTy(), 2, "opt_mem_res");
            phi->addIncoming(nullVal, nullBB);
            phi->addIncoming(result, accessBB);
            result = phi;
        }

        lastValue = result;
    }

    void CodeGenerator::visit(ThisExpression &node)
    {
        if (currentThis)
        {
            // 'currentThis' is the raw pointer argument passed to the function.
            lastValue = currentThis;
        }
        else
        {
            // Only happens if 'this' is used outside a class (Semantic Analyzer should catch this, but safe fallback)
            std::cerr << "Codegen Error: 'this' used outside of method context." << std::endl;
            lastValue = llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(context));
        }
    }

    void CodeGenerator::visit(MacroDefinitionNode &node)
    {
        // 1. Save state
        llvm::BasicBlock *previousBlock = builder.GetInsertBlock();
        auto oldNamedValues = namedValues;

        // 2. Define Function Signature (Macros are void or Any, args are Any)
        std::vector<llvm::Type *> argTypes;
        for (size_t i = 0; i < node.parameters.size(); i++)
            argTypes.push_back(builder.getPtrTy()); // Treat args as Any

        // We use Void return for simple statement macros
        llvm::FunctionType *ft = llvm::FunctionType::get(builder.getVoidTy(), argTypes, false);
        llvm::Function *macroFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, node.name.lexeme, module.get());

        // 3. Mark as ALWAYS INLINE (Compiles into call site)
        macroFunc->addFnAttr(llvm::Attribute::AlwaysInline);

        // 4. Generate Body
        llvm::BasicBlock *bb = llvm::BasicBlock::Create(context, "entry", macroFunc);
        builder.SetInsertPoint(bb);
        namedValues.clear();

        unsigned argIdx = 0;
        for (auto &paramToken : node.parameters)
        {
            llvm::Argument *arg = macroFunc->getArg(argIdx++);
            arg->setName(paramToken.lexeme);
            llvm::AllocaInst *alloca = CreateEntryBlockAlloca(macroFunc, paramToken.lexeme, builder.getPtrTy());
            builder.CreateStore(arg, alloca);
            namedValues[paramToken.lexeme] = alloca;
        }

        if (node.body)
            node.body->accept(*this);

        if (!builder.GetInsertBlock()->getTerminator())
            builder.CreateRetVoid();

        // 5. Restore State
        namedValues = oldNamedValues;
        if (previousBlock)
            builder.SetInsertPoint(previousBlock);
    }

    void CodeGenerator::visit(SizeOfNode &node)
    {
        llvm::Type *typeToMeasure = nullptr;

        if (node.targetType)
        {
            typeToMeasure = getLLVMType(node.targetType->resolvedType, context, module.get());
        }
        else if (node.expression)
        {
            if (node.expression->resolvedType)
            {
                typeToMeasure = getLLVMType(node.expression->resolvedType, context, module.get());
            }
            else
            {
                node.expression->accept(*this);
                typeToMeasure = lastValue->getType();
            }
        }

        uint64_t size = module->getDataLayout().getTypeAllocSize(typeToMeasure);
        lastValue = llvm::ConstantInt::get(llvm::Type::getInt64Ty(context), size);
    }

    void CodeGenerator::visit(StaticIfNode &node)
    {
        if (node.thenBranch)
            node.thenBranch->accept(*this);
        else if (node.elseBranch)
            node.elseBranch->accept(*this);
    }

    // --- Metadata ---
    void CodeGenerator::visit(TypeNode &node) {}
    void CodeGenerator::visit(TypedefNode &node) {}
    void CodeGenerator::visit(ParameterNode &node) {}
    void CodeGenerator::visit(ImportNode &node) {}
    void CodeGenerator::visit(CaseNode &node) {}
    void CodeGenerator::visit(UnsafeBlockNode &node)
    {
        bool oldState = insideUnsafe;
        insideUnsafe = true; // Enter Danger Zone

        for (auto &stmt : node.body->statements)
        {
            stmt->accept(*this);
        }

        insideUnsafe = oldState; // Restore Safety
    }
}