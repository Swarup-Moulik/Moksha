#include "moksha/MIR/LowerHIRToMIR.h"
#include "moksha/HIR/HIRExpr.h"
#include "moksha/HIR/HIRFunction.h"
#include "moksha/HIR/HIRModule.h"
#include "moksha/HIR/HIRStmt.h"
#include "moksha/HIR/HIRType.h"
#include "moksha/HIR/HIRVisitor.h"
#include "moksha/MIR/MIRArgument.h"
#include "moksha/MIR/MIRBlock.h"
#include "moksha/MIR/MIRBuilder.h"
#include "moksha/MIR/MIRFunction.h"
#include "moksha/MIR/MIRGlobal.h"
#include "moksha/MIR/MIRInst.h"
#include "moksha/MIR/MIRModule.h"
#include "moksha/Support/Diagnostics.h"
#include "llvm/Support/Casting.h"
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace moksha {
namespace mir {

namespace {

// Helper to check for terminators
static MIRInst *getTerminator(MIRBlock *block) {
  if (!block || block->getInstructions().empty())
    return nullptr;
  MIRInst *last = block->getInstructions().back().get();
  if (last->getOpcode() == Opcode::Br || last->getOpcode() == Opcode::CondBr ||
      last->getOpcode() == Opcode::Return ||
      last->getOpcode() == Opcode::Switch ||
      last->getOpcode() == Opcode::Invoke ||
      last->getOpcode() == Opcode::Throw ||
      last->getOpcode() == Opcode::Resume ||
      last->getOpcode() == Opcode::Unreachable) {
    return last;
  }
  return nullptr;
}

class HIRToMIRConverter : public hir::ConstHIRVisitor {
public:
  HIRToMIRConverter(const hir::HIRModule *hirModule, DiagnosticEngine &diags)
      : hirModule(hirModule), diags(diags) {
    mirModule = std::make_unique<MIRModule>(hirModule->getName().str());
    builder = std::make_unique<MIRBuilder>(mirModule.get());
  }

  MIRFunction *initFunc = nullptr;
  MIRBlock *initBlock = nullptr;

  std::stack<size_t> breakScopeDepths;
  std::stack<size_t> continueScopeDepths;
  std::stack<size_t> tryScopeDepths;
  unsigned lambdaCounter = 0;
  const hir::HIRType *expectedLambdaReturnType = nullptr;
  const hir::HIRType *currentASTFuncRetTy = nullptr;
  std::unordered_map<std::string, uint64_t> enumVariantValues;
  bool inEscapeContext = false;
  bool isLValueContext = false;

  bool isOptionalLHS(const hir::HIRExpr *e) {
    if (!e)
      return false;
    if (auto *mem = llvm::dyn_cast_or_null<hir::HIRMemberExpr>(e)) {
      const hir::HIRType *objTy = mem->getObject()->getType();
      if (objTy && objTy->getKind() == hir::TypeKind::Nullable)
        return true;
      return isOptionalLHS(mem->getObject());
    }
    if (auto *idx = llvm::dyn_cast_or_null<hir::HIRIndexExpr>(e)) {
      if (idx->isOptionalAccess())
        return true;
      return isOptionalLHS(idx->getBase());
    }
    return false;
  }

  // MONOMORPHIZATION QUEUE STATE
  struct MonomorphizationTask {
    const hir::HIRClass *genericClass = nullptr;
    const hir::HIRFunction *genericFunc = nullptr;
    std::vector<const hir::HIRType *> typeArgs;
  };
  std::queue<MonomorphizationTask> monoQueue;
  std::unordered_set<std::string> instantiatedGenerics;
  std::unordered_map<std::string, const hir::HIRType *> currentTypeEnv;

  const hir::HIRType *getABICoercedType(const hir::HIRType *ty, bool isExtern) {
    if (!isExtern || !ty)
      return ty;

    bool isLargeStruct = false;
    auto kind = ty->getKind();
    if (kind == hir::TypeKind::Any || kind == hir::TypeKind::Slice ||
        kind == hir::TypeKind::Closure || kind == hir::TypeKind::Map ||
        kind == hir::TypeKind::Decimal) {
      isLargeStruct = true;
    } else if (auto *st = llvm::dyn_cast_or_null<hir::StructType>(ty)) {
      auto getByteSize = [&](const hir::HIRType *t) -> size_t {
        auto impl = [&](auto &self, const hir::HIRType *t_) -> size_t {
          if (!t_)
            return 8;
          if (t_->getKind() == hir::TypeKind::Int)
            return std::max<size_t>(
                1, static_cast<const hir::HIRIntType *>(t_)->getWidth() / 8);
          if (t_->getKind() == hir::TypeKind::Float)
            return std::max<size_t>(
                1, static_cast<const hir::HIRFloatType *>(t_)->getWidth() / 8);
          if (t_->getKind() == hir::TypeKind::Array) {
            auto *arr = static_cast<const hir::ArrayType *>(t_);
            return self(self, arr->getElementType()) * arr->getSize();
          }
          if (t_->getKind() == hir::TypeKind::Struct) {
            size_t size = 0;
            for (auto *f :
                 static_cast<const hir::StructType *>(t_)->getFields())
              size += self(self, f);
            return size;
          }
          if (t_->getKind() == hir::TypeKind::Any ||
              t_->getKind() == hir::TypeKind::Slice ||
              t_->getKind() == hir::TypeKind::Closure ||
              t_->getKind() == hir::TypeKind::Map ||
              t_->getKind() == hir::TypeKind::Decimal)
            return 16;
          return 8;
        };
        return impl(impl, t);
      };

      size_t structSize = getByteSize(st);

      if (structSize > 8) {
        return ty;
      } else if (structSize > 0 && structSize <= 8) {
        return const_cast<hir::HIRModule *>(hirModule)->getIntType(64, false);
      }
    }

    if (isLargeStruct) {
      return const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          ty, hir::Ownership::None);
    }
    return ty;
  }

  MIRGlobal *getExceptionPayloadGlobal() {
    MIRGlobal *exGlobal = mirModule->getGlobal("__moksha_ex_payload");
    if (!exGlobal) {
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
          hir::Ownership::None);

      exGlobal =
          builder->createGlobal(mirModule.get(), "__moksha_ex_payload",
                                voidPtrTy, nullptr, false, Linkage::Weak);
      exGlobal->setThreadLocal(true);
    }
    return exGlobal;
  }

  // Substitutes generic 'T' with concrete types during lowering
  const hir::HIRType *resolveType(const hir::HIRType *t) {
    if (!t)
      return nullptr;

    t = stripMemoryModifiers(t);

    std::string tName = t->toString();
    if (currentTypeEnv.count(tName)) {
      const hir::HIRType *resolvedEnv =
          stripMemoryModifiers(currentTypeEnv[tName]);
      ensureStringifierForAny(resolvedEnv);
      return resolvedEnv;
    }

    // Unwrap pointers safely
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(t)) {
      const hir::HIRType *resolved = resolveType(ptrTy->getPointee());
      auto *res = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          resolved, hir::Ownership::None);
      ensureStringifierForAny(res);
      return res;
    }
    if (auto *refTy = llvm::dyn_cast_or_null<hir::ReferenceType>(t)) {
      const hir::HIRType *resolved = resolveType(refTy->getInner());
      auto *res = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          resolved, hir::Ownership::None);
      ensureStringifierForAny(res);
      return res;
    }

    ensureStringifierForAny(t);
    return t;
  }

  const hir::HIRType *stripMemoryModifiers(const hir::HIRType *ty) const {
    if (!ty)
      return nullptr;
    const hir::HIRType *coreTy = ty;
    while (coreTy) {
      if (auto *mutTy = llvm::dyn_cast_or_null<hir::HIRMutType>(coreTy))
        coreTy = mutTy->getInner();
      else if (auto *viewTy = llvm::dyn_cast_or_null<hir::HIRViewType>(coreTy))
        coreTy = viewTy->getInner();
      else if (auto *lockTy = llvm::dyn_cast_or_null<hir::HIRLockType>(coreTy))
        coreTy = lockTy->getInner();
      else if (auto *constTy =
                   llvm::dyn_cast_or_null<hir::HIRConstType>(coreTy))
        coreTy = constTy->getInner();
      else if (auto *volTy =
                   llvm::dyn_cast_or_null<hir::HIRVolatileType>(coreTy))
        coreTy = volTy->getInner();
      else
        break;
    }

    // Deep stripping to catch inner pointers like `lock int*`
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(coreTy)) {
      const hir::HIRType *inner = stripMemoryModifiers(ptrTy->getPointee());
      if (inner != ptrTy->getPointee()) {
        return const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            inner, ptrTy->getOwnership());
      }
    } else if (auto *refTy =
                   llvm::dyn_cast_or_null<hir::ReferenceType>(coreTy)) {
      const hir::HIRType *inner = stripMemoryModifiers(refTy->getInner());
      if (inner != refTy->getInner()) {
        return const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            inner, refTy->getOwnership());
      }
    } else if (auto *arrTy = llvm::dyn_cast_or_null<hir::ArrayType>(coreTy)) {
      const hir::HIRType *inner = stripMemoryModifiers(arrTy->getElementType());
      if (inner != arrTy->getElementType()) {
        return const_cast<hir::HIRModule *>(hirModule)->getArrayType(
            inner, arrTy->getSize());
      }
    }

    return coreTy;
  }

  MIRValue *evaluateAsLValue(const hir::HIRExpr *expr) {
    if (!expr)
      return nullptr;

    bool oldLValueContext = isLValueContext;
    isLValueContext = true;

    size_t instCountBefore = 0;
    if (builder->getInsertBlock()) {
      instCountBefore = builder->getInsertBlock()->getInstructions().size();
    }

    visit(expr);
    isLValueContext = oldLValueContext;

    MIRValue *val = lastExprValue;

    // Fallback cleanup in case an expression ignored isLValueContext
    if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(val)) {
      MIRValue *ptr = loadInst->getPointer();
      auto &insts = builder->getInsertBlock()->getInstructionsMut();
      if (!insts.empty() && insts.back().get() == loadInst &&
          insts.size() > instCountBefore) {
        insts.pop_back();
      }
      return ptr;
    } else if (auto *loadWeak = llvm::dyn_cast_or_null<LoadWeakInst>(val)) {
      MIRValue *ptr = loadWeak->getPointer();
      auto &insts = builder->getInsertBlock()->getInstructionsMut();
      if (!insts.empty() && insts.back().get() == loadWeak &&
          insts.size() > instCountBefore) {
        insts.pop_back();
      }
      return ptr;
    } else if (auto *castInst = llvm::dyn_cast_or_null<CastInst>(val)) {
      if (castInst->getOpcode() == Opcode::AnyCast)
        return castInst;
    }

    return val;
  }

  std::string mangleName(const std::string &base,
                         const std::vector<const hir::HIRType *> &types) {
    std::string res = base;
    for (const auto *ty : types) {
      std::string tStr = ty ? ty->toString() : "void";
      // Sanitize for LLVM compatibility
      for (char &c : tStr) {
        if (!isalnum(c))
          c = '_';
      }
      res += "_" + tStr;
    }
    return res;
  }

  void ensureBuiltinMIR(const std::string &name) {
    if (mirModule->getFunction(name))
      return;

    // Search the HIR Module's function list for this builtin.
    for (const auto *hirFunc : hirModule->getFunctions()) {
      if (hirFunc->getName() == name && hirFunc->isExtern()) {
        std::string abi = hirFunc->getABI();
        if (!abi.empty() && abi != "stdcall" && abi != "fastcall" &&
            abi != "vectorcall" && abi != "sysv64" && abi != "win64" &&
            abi != "cdecl" && abi != "C") {
          if (mirModule->getFunction(abi))
            return;
        }
        createFunctionDecl(hirFunc);
        return;
      }
    }
  }

  // The Monomorphization Engine
  void processMonomorphizationQueue() {
    while (!monoQueue.empty()) {
      auto task = monoQueue.front();
      monoQueue.pop();

      currentTypeEnv.clear();

      // Process Free-Floating Generic Functions
      if (task.genericFunc) {
        // Map exact arguments (e.g., "T[]" -> "i32[]")
        for (size_t i = 0; i < task.genericFunc->getParams().size() &&
                           i < task.typeArgs.size();
             ++i) {
          std::string pName = task.genericFunc->getParams()[i].type->toString();
          currentTypeEnv[pName] = task.typeArgs[i];
        }

        // Extract 'T' from the lambda signature so the inner body can use it!
        if (currentTypeEnv.find("T") == currentTypeEnv.end()) {
          for (const auto *argTy : task.typeArgs) {
            if (auto *fnTy = llvm::dyn_cast_or_null<hir::FunctionType>(argTy)) {
              if (!fnTy->getParamTypes().empty())
                currentTypeEnv["T"] = fnTy->getParamTypes()[0];
            } else if (auto *closTy =
                           llvm::dyn_cast_or_null<hir::HIRClosureType>(argTy)) {
              if (!closTy->getParamTypes().empty())
                currentTypeEnv["T"] = closTy->getParamTypes()[0];
            }
          }
        }

        std::vector<const hir::HIRType *> pTys;
        for (const auto &p : task.genericFunc->getParams()) {
          pTys.push_back(resolveType(p.type));
        }

        std::string mangledName = mangleName(task.genericFunc->getName(), pTys);
        std::string retStr =
            task.genericFunc->getReturnType()
                ? resolveType(task.genericFunc->getReturnType())->toString()
                : "void";
        if (!retStr.empty() && retStr.back() == '?')
          retStr.pop_back();
        std::replace(retStr.begin(), retStr.end(), '*', 'p');
        for (char &c : retStr) {
          if (!isalnum(c))
            c = '_';
        }
        mangledName += "_ret_" + retStr;

        if (!mirModule->getFunction(mangledName)) {
          createFunctionDecl(task.genericFunc, mangledName, nullptr);
        }

        lowerFunction(*task.genericFunc, mangledName, nullptr);
        currentTypeEnv.clear();
        continue;
      }

      // Map generic parameters to concrete types based on constructor params
      for (const auto &method : task.genericClass->getMethods()) {
        if (method->getName() == "constructor") {
          for (size_t i = 0;
               i < method->getParams().size() && i < task.typeArgs.size();
               ++i) {
            std::string pName = method->getParams()[i].type->toString();
            if (pName.length() == 1 || pName == "T") {
              currentTypeEnv[pName] = task.typeArgs[i];
            }
          }
        }
      }

      // Fallback for standard single-parameter <T>
      if (currentTypeEnv.empty() && task.typeArgs.size() == 1) {
        currentTypeEnv["T"] = task.typeArgs[0];
      }

      const hir::HIRType *actualTy = task.genericClass->getType();
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(actualTy)) {
        actualTy = ptrTy->getPointee();
      } else if (auto *refTy =
                     llvm::dyn_cast_or_null<hir::ReferenceType>(actualTy)) {
        actualTy = refTy->getInner();
      }

      const hir::HIRType *thisTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              task.genericClass->getType(), hir::Ownership::Borrowed);

      for (const auto &method : task.genericClass->getMethods()) {
        if (!method->isExtern()) {
          std::vector<const hir::HIRType *> pTys;
          for (const auto &p : method->getParams()) {
            pTys.push_back(resolveType(p.type));
          }

          std::string uniquePrefix = thisTy->toString();
          if (!uniquePrefix.empty() && uniquePrefix[0] == '*')
            uniquePrefix = uniquePrefix.substr(1);
          std::replace(uniquePrefix.begin(), uniquePrefix.end(), '<', '_');
          std::replace(uniquePrefix.begin(), uniquePrefix.end(), '>', '_');
          while (!uniquePrefix.empty() && uniquePrefix.back() == '_')
            uniquePrefix.pop_back();

          std::string mangledName;
          if (method->getName() == "destructor") {
            mangledName = task.genericClass->getName() + ".destructor_ret_void";
          } else {
            mangledName = mangleName(
                task.genericClass->getName() + "." + method->getName(), pTys);
            std::string retStr =
                method->getReturnType()
                    ? resolveType(method->getReturnType())->toString()
                    : "void";
            if (!retStr.empty() && retStr.back() == '?')
              retStr.pop_back();
            std::replace(retStr.begin(), retStr.end(), '*', 'p');
            for (char &c : retStr) {
              if (!isalnum(c))
                c = '_';
            }
            mangledName += "_ret_" + retStr;
          }

          if (!mirModule->getFunction(mangledName)) {
            createFunctionDecl(method.get(), mangledName, thisTy);
          }

          lowerFunction(*method.get(), mangledName, thisTy);
        }
      }
      currentTypeEnv.clear();
    }
  }

  std::unique_ptr<MIRModule> run() {
    auto initF = std::make_unique<MIRFunction>(
        const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
        "__moksha_module_init", Linkage::External);
    initFunc = initF.get();
    mirModule->addFunction(std::move(initF));

    auto ib = std::make_unique<MIRBlock>("entry", initFunc);
    initBlock = ib.get();
    initFunc->addBlock(std::move(ib));

    builder->setInsertPoint(initBlock);
    currFunc = initFunc;

    // Process Functions
    for (const auto *func : hirModule->getFunctions()) {
      std::string funcName = func->getName();
      if (!mirModule->getFunction(funcName)) {
        createFunctionDecl(func, funcName);
      }
    }

    // Process Class Method Declarations
    for (const auto *cls : hirModule->getClasses()) {
      const hir::HIRType *actualTy = cls->getType();
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(actualTy)) {
        actualTy = ptrTy->getPointee();
      } else if (auto *refTy =
                     llvm::dyn_cast_or_null<hir::ReferenceType>(actualTy)) {
        actualTy = refTy->getInner();
      }

      const hir::HIRType *thisTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              actualTy, hir::Ownership::Borrowed);
      if (auto *st = const_cast<hir::StructType *>(
              llvm::dyn_cast_or_null<hir::StructType>(actualTy))) {
        st->setParentTypes(cls->getParentTypes());
      }
      for (const auto &method : cls->getMethods()) {
        std::vector<const hir::HIRType *> pTys;
        for (const auto &p : method->getParams())
          pTys.push_back(p.type);

        std::string mangledName;
        if (method->isExtern()) {
          std::string baseClass = cls->getName();
          size_t under = baseClass.find('_');
          if (under != std::string::npos) {
            baseClass = baseClass.substr(0, under);
          }
          mangledName = baseClass + "_" + method->getName();
          if (baseClass == "Channel") {
            mangledName = "moksha_builtin_" + mangledName;
          }
        } else {
          mangledName =
              mangleName(cls->getName() + "." + method->getName(), pTys);
          std::string retStr = method->getReturnType()
                                   ? method->getReturnType()->toString()
                                   : "void";
          if (!retStr.empty() && retStr.back() == '?')
            retStr.pop_back();
          std::replace(retStr.begin(), retStr.end(), '*', 'p');
          for (char &c : retStr) {
            if (!isalnum(c))
              c = '_';
          }
          mangledName += "_ret_" + retStr;
        }
        const hir::HIRType *passThisTy =
            method->isStaticFunc() ? nullptr : thisTy;
        if (!mirModule->getFunction(mangledName)) {
          createFunctionDecl(method.get(), mangledName, passThisTy);
        }
      }
      generateVTable(cls);
    }

    // Process Globals
    builder->setInsertPoint(initBlock);
    currFunc = initFunc;
    for (const auto &stmt : hirModule->getGlobals()) {
      createGlobalDecl(stmt);
    }

    MIRBlock *initExitBlock = builder->getInsertBlock();

    // Lower Bodies
    for (const auto *func : hirModule->getFunctions()) {
      if (!func->isExtern()) {
        lowerFunction(*func);
      }
    }

    // Lower Class Method Bodies
    for (const auto *cls : hirModule->getClasses()) {
      const hir::HIRType *actualTy = cls->getType();
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(actualTy)) {
        actualTy = ptrTy->getPointee();
      } else if (auto *refTy =
                     llvm::dyn_cast_or_null<hir::ReferenceType>(actualTy)) {
        actualTy = refTy->getInner();
      }

      const hir::HIRType *thisTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              cls->getType(), hir::Ownership::Borrowed);
      for (const auto &method : cls->getMethods()) {
        if (!method->isExtern()) {
          bool isGenericTemplate = false;
          for (const auto &p : method->getParams()) {
            if (p.type->toString() == "T" || p.type->toString().length() == 1)
              isGenericTemplate = true;
          }
          if (isGenericTemplate)
            continue;

          std::vector<const hir::HIRType *> pTys;
          for (const auto &p : method->getParams())
            pTys.push_back(p.type);

          std::string mangledName =
              mangleName(cls->getName() + "." + method->getName(), pTys);
          std::string retStr = method->getReturnType()
                                   ? method->getReturnType()->toString()
                                   : "void";
          if (!retStr.empty() && retStr.back() == '?')
            retStr.pop_back();
          std::replace(retStr.begin(), retStr.end(), '*', 'p');
          for (char &c : retStr) {
            if (!isalnum(c))
              c = '_';
          }
          mangledName += "_ret_" + retStr;
          const hir::HIRType *passThisTy =
              method->isStaticFunc() ? nullptr : thisTy;
          lowerFunction(*method.get(), mangledName, passThisTy);
        }
      }
    }

    // PROCESS QUEUE BEFORE EXIT
    processMonomorphizationQueue();

    // Terminate the Init Function
    builder->setInsertPoint(initExitBlock);
    builder->insert(std::make_unique<ReturnInst>(nullptr, SourceLocation{}));
    initFunc->numberUnnamedValues();
    builder->clearInsertPoint();

    // Module Destroy Function
    auto destroyF = std::make_unique<MIRFunction>(
        const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
        "__moksha_module_destroy", Linkage::External);
    MIRFunction *destroyFunc = destroyF.get();
    mirModule->addFunction(std::move(destroyF));

    auto db = std::make_unique<MIRBlock>("entry", destroyFunc);
    builder->setInsertPoint(db.get());
    destroyFunc->addBlock(std::move(db));
    currFunc = destroyFunc;

    for (const auto &global : hirModule->getGlobals()) {
      if (auto *varDecl = llvm::dyn_cast_or_null<hir::HIRVarDeclStmt>(global)) {
        std::string baseName;
        const hir::HIRType *actualType = varDecl->getType();
        bool isPointer = false;
        if (auto *ptrTy =
                llvm::dyn_cast_or_null<hir::PointerType>(actualType)) {
          actualType = ptrTy->getPointee();
          isPointer = true;
        }

        if (auto *stTy = llvm::dyn_cast_or_null<hir::StructType>(actualType)) {
          baseName = stTy->getName().str();
          std::replace(baseName.begin(), baseName.end(), '<', '_');
          std::replace(baseName.begin(), baseName.end(), '>', '_');
          while (!baseName.empty() && baseName.back() == '_')
            baseName.pop_back();
          if (baseName.find("struct.") == 0)
            baseName = baseName.substr(7);
          if (baseName.find("class.") == 0)
            baseName = baseName.substr(6);
        }

        if (!baseName.empty()) {
          std::string dtorName = baseName + ".destructor_ret_void";
          if (MIRFunction *dtorFunc = mirModule->getFunction(dtorName)) {
            MIRGlobal *gVar = mirModule->getGlobal(varDecl->getName());
            if (gVar && !isPointer) {
              MIRValue *argVal = gVar;
              const hir::HIRType *expectedTy =
                  dtorFunc->getRawArguments().empty()
                      ? nullptr
                      : dtorFunc->getRawArguments()[0]->getType();
              if (expectedTy && argVal->getType() != expectedTy) {
                argVal = builder->createBitCast(argVal, expectedTy, "gvar.cast",
                                                varDecl->getLoc());
              }
              argVal->setBorrowKind(mir::BorrowKind::View);
              builder->insert(std::make_unique<CallInst>(
                  dtorFunc, std::vector<MIRValue *>{argVal},
                  const_cast<hir::HIRModule *>(hirModule)->getVoidType(), "",
                  false, varDecl->getLoc()));
            }
          }
        }
      }
    }

    builder->insert(std::make_unique<ReturnInst>(nullptr, SourceLocation{}));
    destroyFunc->numberUnnamedValues();
    builder->clearInsertPoint();

    // Global CFG Linker
    for (auto &fPtr : mirModule->getFunctions()) {
      if (fPtr->isDeclaration())
        continue;

      for (auto &blockPtr : fPtr->getBlocks()) {
        blockPtr->getSuccessors().clear();
        blockPtr->getPredecessors().clear();
      }

      for (auto &blockPtr : fPtr->getBlocks()) {
        MIRBlock *b = blockPtr.get();
        if (b->getInstructions().empty())
          continue;

        MIRInst *term = b->getInstructions().back().get();
        if (auto *br = llvm::dyn_cast_or_null<BranchInst>(term)) {
          b->addSuccessor(br->getTarget());
          br->getTarget()->addPredecessor(b);
        } else if (auto *condBr =
                       llvm::dyn_cast_or_null<CondBranchInst>(term)) {
          b->addSuccessor(condBr->getTrueBlock());
          condBr->getTrueBlock()->addPredecessor(b);
          b->addSuccessor(condBr->getFalseBlock());
          condBr->getFalseBlock()->addPredecessor(b);
        } else if (auto *invoke = llvm::dyn_cast_or_null<InvokeInst>(term)) {
          b->addSuccessor(invoke->getNormalDest());
          invoke->getNormalDest()->addPredecessor(b);
          if (invoke->getUnwindDest()) {
            b->addSuccessor(invoke->getUnwindDest());
            invoke->getUnwindDest()->addPredecessor(b);
          }
        } else if (auto *throwInst = llvm::dyn_cast_or_null<ThrowInst>(term)) {
          if (throwInst->getUnwindDest()) {
            b->addSuccessor(throwInst->getUnwindDest());
            throwInst->getUnwindDest()->addPredecessor(b);
          }
        } else if (auto *switchInst =
                       llvm::dyn_cast_or_null<SwitchInst>(term)) {
          b->addSuccessor(switchInst->getDefaultBlock());
          switchInst->getDefaultBlock()->addPredecessor(b);
          for (auto &casePair : switchInst->getCases()) {
            b->addSuccessor(casePair.second);
            casePair.second->addPredecessor(b);
          }
        }
      }
    }

    return std::move(mirModule);
  }

  // Lexical Scope Tracker
  struct LexicalScope {
    std::vector<MIRValue *> refCountedVars;
    std::vector<MIRValue *> ownedVars;
    std::vector<const hir::HIRStmt *> deferredStmts;
  };
  std::vector<LexicalScope> scopeStack;

  void emitScopeCleanup(size_t scopeIdx, SourceLocation loc,
                        bool isUnwind = false) {
    std::vector<const hir::HIRStmt *> defersToProcess =
        scopeStack[scopeIdx].deferredStmts;
    std::vector<MIRValue *> ownedToProcess = scopeStack[scopeIdx].ownedVars;
    std::vector<MIRValue *> sharedToProcess =
        scopeStack[scopeIdx].refCountedVars;

    scopeStack[scopeIdx].deferredStmts.clear();
    scopeStack[scopeIdx].ownedVars.clear();
    scopeStack[scopeIdx].refCountedVars.clear();

    // 1. Run Deferred Statements & Lock Cleanups (LIFO)
    for (auto it = defersToProcess.rbegin(); it != defersToProcess.rend();
         ++it) {
      const hir::HIRStmt *deferred = *it;

      if (getTerminator(builder->getInsertBlock()))
        continue;

      // Ensure locks are released during stack unwinding or early returns
      if (auto *lockStmt = llvm::dyn_cast_or_null<hir::LockStmt>(deferred)) {
        visit(lockStmt->getMutex());
        MIRValue *mutexPtr = lastExprValue;

        if (mutexPtr && lockStmt->getMutex()->getValueCategory() ==
                            hir::ValueCategory::LValue) {
          if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(
                  mutexPtr->getType())) {
            if (ptrTy->getPointee()->getKind() == hir::TypeKind::Pointer ||
                ptrTy->getPointee()->getKind() == hir::TypeKind::Reference) {
              mutexPtr =
                  builder->createLoad(mutexPtr, "mutex.load.unwind", loc);
            }
          }
        }

        if (mutexPtr) {
          // Branch for Async Unlock
          if (lockStmt->isAsyncLock()) {
            std::string unlockMethodName = "AsyncMutex_unlock_ret_void";
            if (MIRFunction *unlockFunc =
                    mirModule->getFunction(unlockMethodName)) {
              auto *voidTy =
                  const_cast<hir::HIRModule *>(hirModule)->getVoidType();

              MIRValue *unlockCast = mutexPtr;
              if (!unlockFunc->getRawArguments().empty()) {
                const hir::HIRType *expectedTy =
                    unlockFunc->getRawArguments()[0]->getType();
                if (mutexPtr->getType() != expectedTy) {
                  unlockCast = builder->createBitCast(
                      mutexPtr, expectedTy, "mutex.unlock.cast.unwind", loc);
                }
              }

              builder->createCall(unlockFunc,
                                  std::vector<MIRValue *>{unlockCast}, voidTy,
                                  "", false, loc);
            }
          } else {
            // Standard OS Unlock
            auto *voidTy =
                const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            auto *voidPtrTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    voidTy, hir::Ownership::None);

            MIRFunction *unlockFunc = mirModule->getFunction("__moksha_unlock");
            if (!unlockFunc) {
              auto fn = std::make_unique<MIRFunction>(voidTy, "__moksha_unlock",
                                                      Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              unlockFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }

            MIRValue *castMutex = mutexPtr;
            if (!unlockFunc->getRawArguments().empty()) {
              const hir::HIRType *expectedTy =
                  unlockFunc->getRawArguments()[0]->getType();
              if (mutexPtr->getType() != expectedTy) {
                castMutex = builder->createBitCast(
                    mutexPtr, expectedTy, "mutex.unlock.cast.unwind", loc);
              }
            }

            builder->createCall(unlockFunc, std::vector<MIRValue *>{castMutex},
                                voidTy, "", false, loc);
          }
        }
      } else {
        // Standard deferred statement
        visit(deferred);
      }
    }

    // Helper to process both Owned and Shared drops
    auto processDrops = [&](std::vector<MIRValue *> &vars, bool isARC) {
      for (auto it = vars.rbegin(); it != vars.rend(); ++it) {
        if (getTerminator(builder->getInsertBlock()))
          continue;

        MIRValue *allocaPtr = *it;

        SourceLocation dropLoc;
        if (auto *inst = llvm::dyn_cast_or_null<MIRInst>(allocaPtr)) {
          dropLoc = inst->getLoc();
        }

        const hir::HIRType *ptrTy = allocaPtr->getType();
        const hir::HIRType *valTy = nullptr;
        if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(ptrTy)) {
          valTy = pTy->getPointee();
        }

        if (valTy) {
          std::string typeName = valTy->toString();

          // Clean up pointer/smart pointer prefixes for destructor lookup
          if (typeName.find("shared ") == 0)
            typeName = typeName.substr(7);
          if (typeName.find("owned ") == 0)
            typeName = typeName.substr(6);
          if (typeName.find("weak ") == 0)
            typeName = typeName.substr(5);

          size_t arcPos = typeName.find("Arc<");
          size_t boxPos = typeName.find("Box<");
          size_t startPos =
              (arcPos != std::string::npos)
                  ? arcPos
                  : ((boxPos != std::string::npos) ? boxPos
                                                   : std::string::npos);

          if (startPos != std::string::npos) {
            typeName = typeName.substr(startPos + 4);
            size_t endPos = typeName.rfind(">");
            if (endPos != std::string::npos)
              typeName = typeName.substr(0, endPos);
          }

          while (!typeName.empty() &&
                 (typeName[0] == '&' || typeName[0] == '*' ||
                  typeName[0] == ' ')) {
            typeName = typeName.substr(1);
          }

          if (typeName.find("struct ") == 0)
            typeName = typeName.substr(7);
          if (typeName.find("class ") == 0)
            typeName = typeName.substr(6);

          std::string dropName = typeName + ".destructor_ret_void";
          MIRFunction *dropFunc = mirModule->getFunction(dropName);

          // Properly detect all ARC-managed types
          bool isClosureType =
              (valTy->getKind() == hir::TypeKind::Closure ||
               valTy->toString().find("closure") != std::string::npos);
          bool isAnyType = (valTy->getKind() == hir::TypeKind::Any);

          bool typeIsARC =
              isARC || isClosureType || isAnyType ||
              valTy->getKind() == hir::TypeKind::String ||
              valTy->getKind() == hir::TypeKind::Map ||
              valTy->getKind() == hir::TypeKind::Slice ||
              valTy->getKind() == hir::TypeKind::Promise ||
              valTy->getKind() == hir::TypeKind::Nullable ||
              valTy->toString().find("Box<") != std::string::npos ||
              valTy->toString().find("Arc<") != std::string::npos;

          bool needsFree =
              (!typeIsARC && valTy->getKind() == hir::TypeKind::Pointer);

          if (dropFunc || typeIsARC || needsFree) {
            auto *loaded = builder->insert(
                std::make_unique<LoadInst>(allocaPtr, "cleanup_val", dropLoc));

            if (dropFunc && !typeIsARC) {
              MIRValue *argVal = allocaPtr;
              if (!dropFunc->getRawArguments().empty()) {
                const hir::HIRType *expectedTy =
                    dropFunc->getRawArguments()[0]->getType();
                if (argVal->getType() != expectedTy) {
                  argVal = builder->createBitCast(argVal, expectedTy,
                                                  "drop.cast", dropLoc);
                }
              }
              builder->insert(std::make_unique<CallInst>(
                  dropFunc, std::vector<MIRValue *>{argVal},
                  dropFunc->getType(), "", false, dropLoc));
            }

            if (typeIsARC) {
              builder->insert(std::make_unique<ARCInst>(Opcode::Release, loaded,
                                                        dropFunc, dropLoc));
            } else if (needsFree) {
              std::string freeName = "__moksha_free";
              MIRFunction *freeFunc = mirModule->getFunction(freeName);
              if (!freeFunc) {
                auto *voidTy =
                    const_cast<hir::HIRModule *>(hirModule)->getVoidType();
                auto *voidPtrTy =
                    const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                        voidTy, hir::Ownership::None);
                auto fn = std::make_unique<MIRFunction>(voidTy, freeName,
                                                        Linkage::External);
                fn->addArgument(
                    std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
                freeFunc = fn.get();
                mirModule->addFunction(std::move(fn));
              }

              MIRValue *castToVoid = builder->createBitCast(
                  loaded, freeFunc->getRawArguments()[0]->getType(),
                  "free.cast", dropLoc);
              builder->insert(std::make_unique<CallInst>(
                  freeFunc, std::vector<MIRValue *>{castToVoid},
                  freeFunc->getType(), "", false, dropLoc));
            }
          }
        }
      }
    };

    // 2. Drop and Free Unique/Owned Variables (LIFO)
    processDrops(ownedToProcess, false);

    // 3. Release ARC Variables (LIFO)
    processDrops(sharedToProcess, true);

    if (isUnwind) {
      scopeStack[scopeIdx].deferredStmts = std::move(defersToProcess);
      scopeStack[scopeIdx].ownedVars = std::move(ownedToProcess);
      scopeStack[scopeIdx].refCountedVars = std::move(sharedToProcess);
    }
  }

  void emitAllDrops(SourceLocation loc) {
    for (size_t i = scopeStack.size(); i > 0; --i) {
      emitScopeCleanup(i - 1, loc);
    }
  }

private:
  const hir::HIRModule *hirModule;
  std::unique_ptr<MIRModule> mirModule;
  DiagnosticEngine &diags;
  std::unique_ptr<MIRBuilder> builder;

  MIRFunction *currFunc = nullptr;
  MIRValue *lastExprValue = nullptr;
  std::unordered_map<std::string, MIRValue *> symbolMap;

  MIRBlock *currentUnwindDest = nullptr;
  MIRBlock *currentUnwindBody = nullptr;
  MIRValue *currentAsyncPromise = nullptr;
  std::stack<MIRBlock *> loopCondBlocks;
  std::stack<MIRBlock *> loopMergeBlocks;

  std::unordered_set<MIRValue *> volatileVars;

  void ensureStringifierForAny(const hir::HIRType *castOpTy) {
    if (!castOpTy)
      return;
    const hir::HIRType *checkTy = castOpTy;
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(checkTy)) {
      checkTy = ptrTy->getPointee();
    }
    if (checkTy->getKind() == hir::TypeKind::Array ||
        checkTy->getKind() == hir::TypeKind::Slice) {
      getOrCreateArrayStringifier(castOpTy);
    } else if (checkTy->getKind() == hir::TypeKind::Map) {
      getOrCreateMapStringifier(castOpTy);
    }
  }

  MIRValue *boxValue(MIRValue *val, const hir::HIRType *srcTy,
                     const hir::HIRType *destTy, SourceLocation loc) {
    if (!val || !srcTy || !destTy)
      return val;

    if (srcTy->getKind() == hir::TypeKind::Any &&
        destTy->getKind() == hir::TypeKind::Any)
      return val;

    const hir::HIRType *strippedSrc = stripMemoryModifiers(srcTy);
    if (auto *nullTy =
            llvm::dyn_cast_or_null<hir::HIRNullableType>(strippedSrc)) {
      MIRBlock *checkBlock = builder->getInsertBlock();
      MIRBlock *validBlock = newBlock("box.opt.valid");
      MIRBlock *mergeBlock = newBlock("box.opt.merge");

      auto *nullConst =
          mirModule->getOrInsertConstant<ConstantNull>(val->getType());
      auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
      MIRValue *isNotNull =
          builder->createICmp(CompareInst::Predicate::NE, val, nullConst,
                              boolTy, "box.opt.notnull", loc);

      builder->createCondBr(isNotNull, validBlock, mergeBlock);

      builder->setInsertPoint(validBlock);
      const hir::HIRType *innerTy = nullTy->getInner();
      MIRValue *unboxedVal = val;

      if (innerTy->getKind() == hir::TypeKind::Int ||
          innerTy->getKind() == hir::TypeKind::Float ||
          innerTy->getKind() == hir::TypeKind::Decimal ||
          innerTy->getKind() == hir::TypeKind::Bool) {
        auto *ptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            innerTy, hir::Ownership::None);
        MIRValue *castPtr =
            builder->createBitCast(val, ptrTy, "box.unwrap.cast", loc);
        unboxedVal = builder->insert(
            std::make_unique<LoadInst>(castPtr, "box.unwrap.load", loc));
      } else {
        auto *rawPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                innerTy, hir::Ownership::None);
        unboxedVal =
            builder->createBitCast(val, rawPtrTy, "box.unwrap.cast", loc);
      }

      MIRValue *validBoxed = boxValue(unboxedVal, innerTy, destTy, loc);
      MIRBlock *validEnd = builder->getInsertBlock();
      builder->createBr(mergeBlock);

      builder->setInsertPoint(mergeBlock);
      auto *phi = builder->createPhi(destTy, "box.opt.phi", loc);
      auto *nullDest = mirModule->getOrInsertConstant<ConstantNull>(destTy);

      phi->addIncoming(nullDest, checkBlock);
      phi->addIncoming(validBoxed, validEnd);

      return phi;
    }

    bool destIsManaged = false;
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(destTy)) {
      if (ptrTy->getOwnership() == hir::Ownership::Shared ||
          ptrTy->getOwnership() == hir::Ownership::Owned) {
        destIsManaged = true;
      }
    }

    bool strippedASTCast = false;
    bool isFreshAllocation = false;

    if (destIsManaged) {
      MIRValue *trace = val;
      while (trace) {
        if (auto *cast = llvm::dyn_cast_or_null<CastInst>(trace)) {
          trace = cast->getValue();
          if (trace->getType()->getKind() == hir::TypeKind::Pointer) {
            strippedASTCast = true;
          }
        } else if (auto *call = llvm::dyn_cast_or_null<CallInst>(trace)) {
          if (call->getCallee() &&
              call->getCallee()->getName() == "__moksha_alloc") {
            isFreshAllocation = true;
          }
          break;
        } else if (auto *invoke = llvm::dyn_cast_or_null<InvokeInst>(trace)) {
          if (invoke->getCallee() &&
              invoke->getCallee()->getName() == "__moksha_alloc") {
            isFreshAllocation = true;
          }
          break;
        } else {
          break;
        }
      }

      if (trace && trace->getType()->getKind() == hir::TypeKind::Pointer) {
        val = trace;
        srcTy = val->getType();
      }
    }

    bool destIsAny = destTy->getKind() == hir::TypeKind::Any;
    bool destIsShared = false;
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(destTy)) {
      if (ptrTy->getOwnership() == hir::Ownership::Shared ||
          ptrTy->getOwnership() == hir::Ownership::Owned) {
        destIsShared = true;
      }
    }
    bool destIsNullable = destTy->getKind() == hir::TypeKind::Nullable;

    if (!destIsAny && !destIsShared && !destIsNullable)
      return val;

    bool isCString = false;
    if (auto *arrTy = llvm::dyn_cast_or_null<hir::ArrayType>(srcTy)) {
      if (auto *intTy = llvm::dyn_cast_or_null<hir::HIRIntType>(
              arrTy->getElementType())) {
        if (intTy->getWidth() == 8)
          isCString = true;
      }
    } else if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(srcTy)) {
      if (auto *intTy =
              llvm::dyn_cast_or_null<hir::HIRIntType>(ptrTy->getPointee())) {
        if (intTy->getWidth() == 8)
          isCString = true;
      } else if (auto *arrTy = llvm::dyn_cast_or_null<hir::ArrayType>(
                     ptrTy->getPointee())) {
        if (auto *intTy = llvm::dyn_cast_or_null<hir::HIRIntType>(
                arrTy->getElementType())) {
          if (intTy->getWidth() == 8)
            isCString = true;
        }
      }
    }

    if (isCString) {
      val = coerceToString(val, loc);
      srcTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();
    }

    bool isManagedPtr = false;
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(srcTy)) {
      if (ptrTy->getOwnership() == hir::Ownership::Shared ||
          ptrTy->getOwnership() == hir::Ownership::Owned) {
        isManagedPtr = true;
      } else {
        MIRValue *trace = val;
        while (trace) {
          if (auto *load = llvm::dyn_cast_or_null<LoadInst>(trace)) {
            trace = load->getPointer();
          } else if (auto *alloca = llvm::dyn_cast_or_null<AllocaInst>(trace)) {
            for (auto &scope : scopeStack) {
              for (auto *arcVar : scope.refCountedVars) {
                if (arcVar == alloca) {
                  isManagedPtr = true;
                  break;
                }
              }
              if (isManagedPtr)
                break;
            }
            break;
          } else if (auto *cast = llvm::dyn_cast_or_null<CastInst>(trace)) {
            trace = cast->getValue();
          } else if (auto *call = llvm::dyn_cast_or_null<CallInst>(trace)) {
            if (call->getCallee() &&
                call->getCallee()->getName() == "__moksha_alloc") {
              isManagedPtr = true;
            }
            break;
          } else {
            break;
          }
        }
      }
    }

    if (srcTy->getKind() == hir::TypeKind::Closure ||
        srcTy->getKind() == hir::TypeKind::Any ||
        srcTy->getKind() == hir::TypeKind::Promise ||
        srcTy->getKind() == hir::TypeKind::Null ||
        srcTy->getKind() == hir::TypeKind::String ||
        srcTy->getKind() == hir::TypeKind::Map ||
        srcTy->getKind() == hir::TypeKind::Slice ||
        srcTy->getKind() == hir::TypeKind::Pointer ||
        srcTy->getKind() == hir::TypeKind::Reference || isManagedPtr) {

      if (srcTy->getKind() == destTy->getKind() && !destIsAny) {
        if (strippedASTCast && !isFreshAllocation) {
          builder->insert(
              std::make_unique<ARCInst>(Opcode::Retain, val, nullptr, loc));
        }
        if (val->getType() != destTy) {
          val = builder->createBitCast(val, destTy, "box.alias.cast", loc);
        }
        return val;
      }

      if (destIsAny) {
        ensureStringifierForAny(val->getType());
        return builder->insert(std::make_unique<CastInst>(
            Opcode::AnyCast, val, destTy, "box.anycast", loc));
      }

      MIRValue *castVal = builder->createBitCast(val, destTy, "box.cast", loc);

      if (strippedASTCast && !isFreshAllocation) {
        builder->insert(
            std::make_unique<ARCInst>(Opcode::Retain, castVal, nullptr, loc));
      }
      return castVal;
    }

    auto *i32Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
        hir::Ownership::None);

    auto *i64Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
    std::string allocName = "__moksha_alloc";
    ensureBuiltinMIR(allocName);
    MIRFunction *allocFunc = mirModule->getFunction(allocName);
    if (!allocFunc) {
      auto fn = std::make_unique<MIRFunction>(voidPtrTy, allocName,
                                              Linkage::External);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 0));
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 1));
      allocFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    const hir::HIRType *coreTy = stripMemoryModifiers(srcTy);
    if (auto *nullTy = llvm::dyn_cast_or_null<hir::HIRNullableType>(coreTy)) {
      coreTy = nullTy->getInner();
    }

    auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
        const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            coreTy, hir::Ownership::None));
    auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
    auto *sizeGep =
        builder->createGEP(nullPtr, {one}, coreTy, "sizeof.gep", loc);
    MIRValue *sizeVal = builder->insert(std::make_unique<CastInst>(
        Opcode::PtrToInt, sizeGep, i64Ty, "sizeof.i64", loc));

    uint32_t typeId = 19; // Default pointer
    if (coreTy->getKind() == hir::TypeKind::Bool)
      typeId = 0;
    else if (coreTy->getKind() == hir::TypeKind::Int) {
      auto *intTy = static_cast<const hir::HIRIntType *>(coreTy);
      if (intTy->isSize())
        typeId = intTy->isSigned() ? 9 : 10;
      else if (intTy->getWidth() == 8)
        typeId = intTy->isSigned() ? 1 : 2;
      else if (intTy->getWidth() == 16)
        typeId = intTy->isSigned() ? 3 : 4;
      else if (intTy->getWidth() == 32)
        typeId = intTy->isSigned() ? 5 : 6;
      else if (intTy->getWidth() == 64)
        typeId = intTy->isSigned() ? 7 : 8;
    } else if (coreTy->getKind() == hir::TypeKind::Float) {
      auto *fltTy = static_cast<const hir::HIRFloatType *>(coreTy);
      if (fltTy->getWidth() == 8)
        typeId = 11;
      else if (fltTy->getWidth() == 16)
        typeId = 12;
      else if (fltTy->getWidth() == 32)
        typeId = 13;
      else if (fltTy->getWidth() == 64)
        typeId = 14;
    } else if (coreTy->getKind() == hir::TypeKind::Decimal) {
      typeId = 15;
    } else if (coreTy->getKind() == hir::TypeKind::String) {
      typeId = 16;
    } else if (coreTy->getKind() == hir::TypeKind::Map ||
               (coreTy->getKind() == hir::TypeKind::Pointer &&
                static_cast<const hir::PointerType *>(coreTy)
                        ->getPointee()
                        ->getKind() == hir::TypeKind::Map)) {
      typeId = 17;
    } else if (coreTy->getKind() == hir::TypeKind::Array ||
               coreTy->getKind() == hir::TypeKind::Slice) {
      typeId = 18;
    } else if (coreTy->getKind() == hir::TypeKind::Promise) {
      typeId = 20;
    } else {
      typeId = 19;
    }

    MIRValue *typeIdVal =
        mirModule->getOrInsertConstant<ConstantInt>(typeId, i32Ty);
    MIRValue *rawBoxPtr = builder->createCall(
        allocFunc, {sizeVal, typeIdVal}, voidPtrTy, "box.alloc", false, loc);

    if (coreTy->getKind() == hir::TypeKind::Array ||
        coreTy->getKind() == hir::TypeKind::Struct) {
      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      std::string memcpyName = "__moksha_array_copy";
      ensureBuiltinMIR(memcpyName);
      MIRFunction *memcpyFunc = mirModule->getFunction(memcpyName);
      if (!memcpyFunc) {
        auto fn = std::make_unique<MIRFunction>(voidTy, memcpyName,
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
        auto *i64Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
        memcpyFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *srcPointer = val;
      if (val->getType()->getKind() != hir::TypeKind::Pointer &&
          val->getType()->getKind() != hir::TypeKind::Nullable) {
        MIRValue *spill =
            builder->createAlloca(val->getType(), "box.src.spill", loc);
        builder->insert(std::make_unique<StoreInst>(val, spill, loc));
        srcPointer = spill;
      }

      MIRValue *srcVoidPtr =
          builder->createBitCast(srcPointer, voidPtrTy, "src.void.cast", loc);
      builder->insert(std::make_unique<CallInst>(
          memcpyFunc, std::vector<MIRValue *>{rawBoxPtr, srcVoidPtr, sizeVal},
          voidTy, "", false, loc));

      if (destIsAny) {
        auto *typedPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                coreTy, hir::Ownership::None);
        auto *typedBoxPtr =
            builder->createBitCast(rawBoxPtr, typedPtrTy, "box.typed", loc);

        ensureStringifierForAny(typedBoxPtr->getType());
        return builder->insert(std::make_unique<CastInst>(
            Opcode::AnyCast, typedBoxPtr, destTy, "box.anycast", loc));
      }

      return builder->createBitCast(rawBoxPtr, destTy, "box.final", loc);
    }

    auto *typedPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        val->getType(), hir::Ownership::None);
    MIRValue *typedBoxPtr =
        builder->createBitCast(rawBoxPtr, typedPtrTy, "box.typed", loc);

    if (val->getType()->getKind() != hir::TypeKind::Void) {
      builder->insert(std::make_unique<StoreInst>(val, typedBoxPtr, loc));
    }

    if (destIsAny) {
      ensureStringifierForAny(typedBoxPtr->getType());
      return builder->insert(std::make_unique<CastInst>(
          Opcode::AnyCast, typedBoxPtr, destTy, "box.anycast", loc));
    }
    return builder->createBitCast(typedBoxPtr, destTy, "box.final", loc);
  }

  MIRValue *unboxValue(MIRValue *val, const hir::HIRType *srcTy,
                       const hir::HIRType *destTy, SourceLocation loc) {
    if (!val || !srcTy || !destTy)
      return val;
    if (srcTy->getKind() != hir::TypeKind::Any)
      return val;

    if (destTy->getKind() == hir::TypeKind::Any)
      return val;

    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
        hir::Ownership::None);
    MIRValue *dataPtr = builder->insert(
        std::make_unique<ExtractValueInst>(val, 0, voidPtrTy, "any.data", loc));

    if (destTy->getKind() == hir::TypeKind::Pointer ||
        destTy->getKind() == hir::TypeKind::Reference ||
        destTy->getKind() == hir::TypeKind::String ||
        destTy->getKind() == hir::TypeKind::Map ||
        destTy->getKind() == hir::TypeKind::Closure ||
        destTy->getKind() == hir::TypeKind::Any ||
        destTy->getKind() == hir::TypeKind::Promise ||
        destTy->getKind() == hir::TypeKind::Null ||
        destTy->getKind() == hir::TypeKind::Array ||
        destTy->getKind() == hir::TypeKind::Slice) {
      return builder->createBitCast(dataPtr, destTy, "unbox.cast", loc);
    }

    auto *typedPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        destTy, hir::Ownership::None);
    MIRValue *typedPtr =
        builder->createBitCast(dataPtr, typedPtrTy, "unbox.ptr", loc);
    return builder->insert(
        std::make_unique<LoadInst>(typedPtr, "unbox.val", loc));
  }

  bool isVolatilePointer(MIRValue *ptr) {
    if (!ptr)
      return false;

    if (auto *g = llvm::dyn_cast_or_null<MIRGlobal>(ptr))
      return g->isVolatile();
    if (volatileVars.count(ptr))
      return true;
    if (auto *cast = llvm::dyn_cast_or_null<CastInst>(ptr)) {
      if (llvm::dyn_cast_or_null<ConstantInt>(cast->getValue()))
        return true;
      return isVolatilePointer(cast->getValue());
    }
    if (auto *load = llvm::dyn_cast_or_null<LoadInst>(ptr))
      return isVolatilePointer(load->getPointer());
    if (auto *gep = llvm::dyn_cast_or_null<GetElementPtrInst>(ptr))
      return isVolatilePointer(gep->getPointer());

    return false;
  }

  bool isIdentifierUsed(const hir::HIRStmt *stmt, const std::string &name) {
    if (!stmt)
      return false;

    std::string buffer;
    llvm::raw_string_ostream ss(buffer);
    stmt->dump(ss);
    ss.flush();

    // Perform a whole-word search to avoid partial matches
    size_t pos = buffer.find(name);
    while (pos != std::string::npos) {
      bool startOk =
          (pos == 0 || (!isalnum(buffer[pos - 1]) && buffer[pos - 1] != '_'));
      bool endOk = (pos + name.length() == buffer.length() ||
                    (!isalnum(buffer[pos + name.length()]) &&
                     buffer[pos + name.length()] != '_'));

      if (startOk && endOk)
        return true;
      pos = buffer.find(name, pos + 1);
    }
    return false;
  }

  static void applyBorrowKind(mir::MIRValue *mirVal,
                              const hir::HIRType *hirType) {
    if (!mirVal || !hirType)
      return;

    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(hirType)) {
      if (ptrTy->isMut())
        mirVal->setBorrowKind(mir::BorrowKind::Mut);
      else if (ptrTy->isView())
        mirVal->setBorrowKind(mir::BorrowKind::View);
      else if (ptrTy->isLock())
        mirVal->setBorrowKind(mir::BorrowKind::Lock);
    } else if (auto *refTy =
                   llvm::dyn_cast_or_null<hir::ReferenceType>(hirType)) {
      if (refTy->isMut())
        mirVal->setBorrowKind(mir::BorrowKind::Mut);
      else if (refTy->isView())
        mirVal->setBorrowKind(mir::BorrowKind::View);
      else if (refTy->isLock())
        mirVal->setBorrowKind(mir::BorrowKind::Lock);
    }
  }

  // Helper to detect weak references in memory
  bool isWeakMemory(const hir::HIRType *ty) const {
    if (!ty)
      return false;
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(ty)) {
      ty = ptrTy->getPointee();
    }
    if (!ty)
      return false;
    if (ty->getKind() == hir::TypeKind::Weak)
      return true;
    return ty->toString().find("weak ") != std::string::npos;
  }

  void generateVTable(const hir::HIRClass *cls) {
    if (!cls->hasVTable())
      return;

    std::string vtableName = cls->getName() + ".vtable";
    if (mirModule->getGlobal(vtableName))
      return;

    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
        hir::Ownership::None);

    // 1. Collect virtual methods and determine VTable size
    int maxIdx = -1;
    for (const auto &m : cls->getMethods()) {
      if (m->isVirtualFunc() || m->isOverrideFunc()) {
        maxIdx = std::max(maxIdx, m->getVTableIndex());
      }
    }

    if (maxIdx == -1)
      return;

    // 2. Populate the VTable Array with BitCasted Function Pointers
    std::vector<MIRValue *> vtableEntries(
        maxIdx + 1, mirModule->getOrInsertConstant<ConstantNull>(voidPtrTy));

    auto *thisTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        cls->getType(), hir::Ownership::Borrowed);

    for (const auto &m : cls->getMethods()) {
      if (m->isVirtualFunc() || m->isOverrideFunc()) {
        std::vector<const hir::HIRType *> pTys;
        for (const auto &p : m->getParams())
          pTys.push_back(p.type);

        std::string mangledName =
            mangleName(cls->getName() + "." + m->getName(), pTys);

        // Append the return type suffix so it matches the real function
        std::string retStr =
            m->getReturnType() ? m->getReturnType()->toString() : "void";
        if (!retStr.empty() && retStr.back() == '?')
          retStr.pop_back();
        std::replace(retStr.begin(), retStr.end(), '*', 'p');
        mangledName += "_ret_" + retStr;

        MIRFunction *mirF = mirModule->getFunction(mangledName);
        if (!mirF) {
          createFunctionDecl(m.get(), mangledName, thisTy);
          mirF = mirModule->getFunction(mangledName);
        }
        auto *bitcast =
            mirModule->getOrInsertConstant<ConstantBitCast>(mirF, voidPtrTy);
        vtableEntries[m->getVTableIndex()] = bitcast;
      }
    }

    // 3. Construct the Constant Array and Struct
    auto *arrayTy = const_cast<hir::HIRModule *>(hirModule)->getArrayType(
        voidPtrTy, vtableEntries.size());
    auto *vtableArray = mirModule->getOrInsertConstant<ConstantArray>(
        arrayTy, std::move(vtableEntries));
    auto *rttiNull = mirModule->getOrInsertConstant<ConstantNull>(voidPtrTy);

    auto *vtableStructTy =
        const_cast<hir::HIRModule *>(hirModule)->getStructType(
            vtableName + "_type", {voidPtrTy, arrayTy});

    auto *vtableStruct = mirModule->getOrInsertConstant<ConstantStruct>(
        vtableStructTy, std::vector<MIRValue *>{rttiNull, vtableArray});

    // 4. Emit the Global Variable
    builder->createGlobal(mirModule.get(), vtableName, vtableStructTy,
                          vtableStruct, true, Linkage::External);
  }

  MIRValue *coerceToBool(MIRValue *val, SourceLocation loc) {
    if (!val || !val->getType())
      return val;

    const hir::HIRType *ty = val->getType();

    // 0. Base Case: Exactly a bool
    if (ty->getKind() == hir::TypeKind::Bool)
      return val;

    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();

    // Unwrap Modifier Types
    std::string tyStr = ty->toString();
    if (tyStr.find("bool") != std::string::npos &&
        tyStr.find("*") == std::string::npos &&
        tyStr.find("&") == std::string::npos &&
        tyStr.find("?") == std::string::npos) {
      return builder->createBitCast(val, boolTy, "bool.cast", loc);
    }

    // 1. Pointer / Nullable -> icmp ne null
    if (ty->getKind() == hir::TypeKind::Pointer ||
        ty->getKind() == hir::TypeKind::Nullable) {
      auto *nullConst = mirModule->getOrInsertConstant<ConstantNull>(ty);
      return builder->createICmp(CompareInst::Predicate::NE, val, nullConst,
                                 boolTy, "tobool", loc);
    }

    // 2. Integer -> icmp ne 0
    if (ty->getKind() == hir::TypeKind::Int) {
      auto *zeroConst = mirModule->getOrInsertConstant<ConstantInt>(0, ty);
      return builder->createICmp(CompareInst::Predicate::NE, val, zeroConst,
                                 boolTy, "tobool", loc);
    }

    // 3. Float / Decimal -> fcmp une 0.0
    if (ty->getKind() == hir::TypeKind::Float ||
        ty->getKind() == hir::TypeKind::Decimal) {
      auto *zeroConst = mirModule->getOrInsertConstant<ConstantFloat>(0.0, ty);
      return builder->createFCmp(FCmpInst::Predicate::UNE, val, zeroConst,
                                 boolTy, "tobool", loc);
    }

    return val;
  }

  MIRFunction *getOrCreateArrayStringifier(const hir::HIRType *valTy) {
    std::string tyStr = valTy->toString();

    tyStr.erase(std::remove(tyStr.begin(), tyStr.end(), '?'), tyStr.end());

    std::string funcName = "__moksha_array_to_string_" + tyStr;
    for (char &c : funcName)
      if (!isalnum(c))
        c = '_';

    if (MIRFunction *f = mirModule->getFunction(funcName))
      return f;

    auto *strTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();
    auto *argTy = valTy;

    auto fn = std::make_unique<MIRFunction>(strTy, funcName, Linkage::Internal);
    fn->addArgument(std::make_unique<MIRArgument>(fn.get(), argTy, 0));
    MIRFunction *func = fn.get();
    mirModule->addFunction(std::move(fn));

    MIRBlock *savedBlock = builder->getInsertBlock();
    MIRFunction *savedFunc = currFunc;
    auto savedScope = scopeStack;
    auto savedMap = symbolMap;

    currFunc = func;
    scopeStack.clear();
    scopeStack.push_back({});
    symbolMap.clear();

    MIRBlock *entry = newBlock("entry");
    builder->setInsertPoint(entry);
    SourceLocation loc{};

    MIRValue *colVal = func->getRawArguments()[0];
    MIRBlock *nullRetBlock = newBlock("arr.null.ret");
    MIRBlock *validBlock = newBlock("arr.valid");

    auto *nullConst =
        mirModule->getOrInsertConstant<ConstantNull>(colVal->getType());
    auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
    MIRValue *isNull = builder->createICmp(CompareInst::Predicate::EQ, colVal,
                                           nullConst, boolTy, "is.null", loc);
    builder->createCondBr(isNull, nullRetBlock, validBlock);

    // If Null -> return "null"
    builder->setInsertPoint(nullRetBlock);
    ensureBuiltinMIR("__moksha_cstr_to_string");
    MIRFunction *cstrFuncNull =
        mirModule->getFunction("__moksha_cstr_to_string");
    auto *i8TyNull =
        const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
    auto *i8PtrTyNull = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        i8TyNull, hir::Ownership::None);
    if (!cstrFuncNull) {
      auto fn2 = std::make_unique<MIRFunction>(strTy, "__moksha_cstr_to_string",
                                               Linkage::External);
      fn2->addArgument(
          std::make_unique<MIRArgument>(fn2.get(), i8PtrTyNull, 0));
      cstrFuncNull = fn2.get();
      mirModule->addFunction(std::move(fn2));
    }
    MIRValue *nullStrRes = builder->createCall(
        cstrFuncNull,
        {mirModule->getOrInsertConstant<ConstantString>("null", i8PtrTyNull)},
        strTy, "null.str", false, loc);
    builder->insert(std::make_unique<ReturnInst>(nullStrRes, loc));

    // If Valid -> Continue normal iteration
    builder->setInsertPoint(validBlock);

    auto *i32Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    const hir::HIRType *actualColTy = valTy;
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(valTy)) {
      actualColTy = ptrTy->getPointee();
      if (actualColTy->getKind() == hir::TypeKind::Slice) {
        colVal = builder->createLoad(colVal, "slice.load", loc);
      }
    }

    // 1. Get Length & Data Ptr
    MIRValue *lenVal = nullptr;
    MIRValue *dataPtr = nullptr;
    const hir::HIRType *elemTy = nullptr;

    if (auto *sliceTy = llvm::dyn_cast_or_null<hir::SliceType>(actualColTy)) {
      elemTy = sliceTy->getElementType();
      auto *elemPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          elemTy, hir::Ownership::None);
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
          hir::Ownership::None);

      ensureBuiltinMIR("moksha_rt_array_data");
      ensureBuiltinMIR("moksha_rt_array_length");
      MIRFunction *dataFunc = mirModule->getFunction("moksha_rt_array_data");
      MIRFunction *lenFunc = mirModule->getFunction("moksha_rt_array_length");
      if (!dataFunc) {
        auto fn = std::make_unique<MIRFunction>(
            voidPtrTy, "moksha_rt_array_data", Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        dataFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      if (!lenFunc) {
        auto fn = std::make_unique<MIRFunction>(i32Ty, "moksha_rt_array_length",
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        lenFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *voidCol =
          builder->createBitCast(colVal, voidPtrTy, "slice.void.cast", loc);
      MIRValue *rawData = builder->createCall(dataFunc, {voidCol}, voidPtrTy,
                                              "slice.data.raw", false, loc);
      dataPtr = builder->createBitCast(rawData, elemPtrTy, "slice.ptr", loc);
      lenVal = builder->createCall(lenFunc, {voidCol}, i32Ty, "slice.len",
                                   false, loc);
    } else if (auto *arrTy =
                   llvm::dyn_cast_or_null<hir::ArrayType>(actualColTy)) {
      elemTy = arrTy->getElementType();
      lenVal =
          mirModule->getOrInsertConstant<ConstantInt>(arrTy->getSize(), i32Ty);

      auto *elemPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          elemTy, hir::Ownership::None);
      dataPtr = builder->createBitCast(colVal, elemPtrTy, "arr.decay", loc);
    }

    // 2. Setup Loop Variables
    MIRValue *idxAlloca = builder->createAlloca(i32Ty, "idx", loc);
    builder->insert(std::make_unique<StoreInst>(
        mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty), idxAlloca, loc));

    MIRValue *resAlloca = builder->createAlloca(strTy, "res", loc);

    ensureBuiltinMIR("__moksha_cstr_to_string");
    MIRFunction *cstrFunc = mirModule->getFunction("__moksha_cstr_to_string");
    auto *i8Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
    auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        i8Ty, hir::Ownership::None);
    if (!cstrFunc) {
      auto fn2 = std::make_unique<MIRFunction>(strTy, "__moksha_cstr_to_string",
                                               Linkage::External);
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), i8PtrTy, 0));
      cstrFunc = fn2.get();
      mirModule->addFunction(std::move(fn2));
    }

    MIRValue *openBrack = builder->createCall(
        cstrFunc,
        {mirModule->getOrInsertConstant<ConstantString>("[", i8PtrTy)}, strTy,
        "open", false, loc);
    builder->insert(std::make_unique<StoreInst>(openBrack, resAlloca, loc));

    MIRBlock *condBlock = newBlock("cond");
    MIRBlock *bodyBlock = newBlock("body");
    MIRBlock *endBlock = newBlock("end");

    builder->createBr(condBlock);

    // Cond Block
    builder->setInsertPoint(condBlock);
    MIRValue *idxVal = builder->createLoad(idxAlloca, "idx.val", loc);
    MIRValue *cmp = builder->createICmp(
        CompareInst::Predicate::LT, idxVal, lenVal,
        const_cast<hir::HIRModule *>(hirModule)->getBoolType(), "cmp", loc);
    builder->createCondBr(cmp, bodyBlock, endBlock);

    // Body Block
    builder->setInsertPoint(bodyBlock);

    MIRBlock *commaBlock = newBlock("comma");
    MIRBlock *elemBlock = newBlock("elem");
    MIRValue *isGtZero = builder->createICmp(
        CompareInst::Predicate::GT, idxVal,
        mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty),
        const_cast<hir::HIRModule *>(hirModule)->getBoolType(), "gt0", loc);
    builder->createCondBr(isGtZero, commaBlock, elemBlock);

    builder->setInsertPoint(commaBlock);
    MIRValue *commaStr = builder->createCall(
        cstrFunc,
        {mirModule->getOrInsertConstant<ConstantString>(", ", i8PtrTy)}, strTy,
        "comma", false, loc);
    MIRValue *curRes1 = builder->createLoad(resAlloca, "res.val1", loc);

    ensureBuiltinMIR("__moksha_string_concat");
    MIRFunction *concatFunc = mirModule->getFunction("__moksha_string_concat");
    if (!concatFunc) {
      auto fn2 = std::make_unique<MIRFunction>(strTy, "__moksha_string_concat",
                                               Linkage::External);
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), strTy, 0));
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), strTy, 1));
      concatFunc = fn2.get();
      mirModule->addFunction(std::move(fn2));
    }

    MIRValue *concat1 = builder->createCall(concatFunc, {curRes1, commaStr},
                                            strTy, "concat1", false, loc);
    builder->insert(std::make_unique<StoreInst>(concat1, resAlloca, loc));
    builder->createBr(elemBlock);

    builder->setInsertPoint(elemBlock);

    MIRValue *elemPtr = nullptr;
    elemPtr = builder->createGEP(dataPtr, {idxVal}, elemTy, "elem.ptr", loc);

    MIRValue *elemLoad = builder->createLoad(elemPtr, "elem.load", loc);
    MIRValue *elemStr = coerceToString(elemLoad, loc);
    MIRValue *curRes2 = builder->createLoad(resAlloca, "res.val2", loc);
    MIRValue *concat2 = builder->createCall(concatFunc, {curRes2, elemStr},
                                            strTy, "concat2", false, loc);
    builder->insert(std::make_unique<StoreInst>(concat2, resAlloca, loc));

    MIRValue *nextIdx = builder->createAdd(
        idxVal, mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty),
        "idx.next", loc);
    builder->insert(std::make_unique<StoreInst>(nextIdx, idxAlloca, loc));
    builder->createBr(condBlock);

    // End Block
    builder->setInsertPoint(endBlock);
    MIRValue *closeBrack = builder->createCall(
        cstrFunc,
        {mirModule->getOrInsertConstant<ConstantString>("]", i8PtrTy)}, strTy,
        "close", false, loc);
    MIRValue *curRes3 = builder->createLoad(resAlloca, "res.val3", loc);
    MIRValue *finalStr = builder->createCall(concatFunc, {curRes3, closeBrack},
                                             strTy, "concat3", false, loc);

    builder->insert(std::make_unique<ReturnInst>(finalStr, loc));

    // Restore
    builder->setInsertPoint(savedBlock);
    currFunc = savedFunc;
    scopeStack = savedScope;
    symbolMap = savedMap;

    return func;
  }

  MIRFunction *getOrCreateMapStringifier(const hir::HIRType *valTy) {
    std::string tyStr = valTy->toString();

    tyStr.erase(std::remove(tyStr.begin(), tyStr.end(), '?'), tyStr.end());

    std::string funcName = "__moksha_map_to_string_" + tyStr;
    for (char &c : funcName)
      if (!isalnum(c))
        c = '_';

    if (MIRFunction *f = mirModule->getFunction(funcName))
      return f;

    auto *strTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();
    auto *argTy = valTy;

    auto fn = std::make_unique<MIRFunction>(strTy, funcName, Linkage::Internal);
    fn->addArgument(std::make_unique<MIRArgument>(fn.get(), argTy, 0));
    MIRFunction *func = fn.get();
    mirModule->addFunction(std::move(fn));

    MIRBlock *savedBlock = builder->getInsertBlock();
    MIRFunction *savedFunc = currFunc;
    auto savedScope = scopeStack;
    auto savedMap = symbolMap;

    currFunc = func;
    scopeStack.clear();
    scopeStack.push_back({});
    symbolMap.clear();

    MIRBlock *entry = newBlock("entry");
    builder->setInsertPoint(entry);
    SourceLocation loc{};

    MIRValue *mapVal = func->getRawArguments()[0];
    MIRBlock *nullRetBlock = newBlock("map.null.ret");
    MIRBlock *validBlock = newBlock("map.valid");

    auto *nullConst =
        mirModule->getOrInsertConstant<ConstantNull>(mapVal->getType());
    auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
    MIRValue *isNull = builder->createICmp(CompareInst::Predicate::EQ, mapVal,
                                           nullConst, boolTy, "is.null", loc);
    builder->createCondBr(isNull, nullRetBlock, validBlock);

    builder->setInsertPoint(nullRetBlock);
    ensureBuiltinMIR("__moksha_cstr_to_string");
    MIRFunction *cstrFuncNull =
        mirModule->getFunction("__moksha_cstr_to_string");
    auto *i8TyNull =
        const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
    auto *i8PtrTyNull = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        i8TyNull, hir::Ownership::None);
    if (!cstrFuncNull) {
      auto fn2 = std::make_unique<MIRFunction>(strTy, "__moksha_cstr_to_string",
                                               Linkage::External);
      fn2->addArgument(
          std::make_unique<MIRArgument>(fn2.get(), i8PtrTyNull, 0));
      cstrFuncNull = fn2.get();
      mirModule->addFunction(std::move(fn2));
    }
    MIRValue *nullStrRes = builder->createCall(
        cstrFuncNull,
        {mirModule->getOrInsertConstant<ConstantString>("null", i8PtrTyNull)},
        strTy, "null.str", false, loc);
    builder->insert(std::make_unique<ReturnInst>(nullStrRes, loc));

    builder->setInsertPoint(validBlock);

    auto *i32Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);
    auto *anyTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();

    // Unwrap pointers to maps passed by reference
    const hir::HIRType *actualColTy = valTy;
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(valTy)) {
      actualColTy = ptrTy->getPointee();
      if (actualColTy->getKind() == hir::TypeKind::Map) {
        mapVal = builder->createLoad(mapVal, "map.load", loc);
      }
    }

    MIRValue *mapPtr =
        builder->createBitCast(mapVal, voidPtrTy, "map.ptr", loc);

    ensureBuiltinMIR("moksha_rt_map_len");
    MIRFunction *mapLenFunc = mirModule->getFunction("moksha_rt_map_len");
    if (!mapLenFunc) {
      auto fn2 = std::make_unique<MIRFunction>(i32Ty, "moksha_rt_map_len",
                                               Linkage::External);
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), voidPtrTy, 0));
      mapLenFunc = fn2.get();
      mirModule->addFunction(std::move(fn2));
    }
    MIRValue *lenVal =
        builder->createCall(mapLenFunc, {mapPtr}, i32Ty, "map.len", false, loc);

    MIRValue *idxAlloca = builder->createAlloca(i32Ty, "idx", loc);
    builder->insert(std::make_unique<StoreInst>(
        mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty), idxAlloca, loc));

    MIRValue *resAlloca = builder->createAlloca(strTy, "res", loc);

    ensureBuiltinMIR("__moksha_cstr_to_string");
    MIRFunction *cstrFunc = mirModule->getFunction("__moksha_cstr_to_string");
    auto *i8Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
    auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        i8Ty, hir::Ownership::None);
    if (!cstrFunc) {
      auto fn2 = std::make_unique<MIRFunction>(strTy, "__moksha_cstr_to_string",
                                               Linkage::External);
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), i8PtrTy, 0));
      cstrFunc = fn2.get();
      mirModule->addFunction(std::move(fn2));
    }

    MIRValue *openBrack = builder->createCall(
        cstrFunc,
        {mirModule->getOrInsertConstant<ConstantString>("{ ", i8PtrTy)}, strTy,
        "open", false, loc);
    builder->insert(std::make_unique<StoreInst>(openBrack, resAlloca, loc));

    MIRBlock *condBlock = newBlock("cond");
    MIRBlock *bodyBlock = newBlock("body");
    MIRBlock *endBlock = newBlock("end");

    builder->createBr(condBlock);

    // Cond Block
    builder->setInsertPoint(condBlock);
    MIRValue *idxVal = builder->createLoad(idxAlloca, "idx.val", loc);
    MIRValue *cmp = builder->createICmp(
        CompareInst::Predicate::LT, idxVal, lenVal,
        const_cast<hir::HIRModule *>(hirModule)->getBoolType(), "cmp", loc);
    builder->createCondBr(cmp, bodyBlock, endBlock);

    // Body Block
    builder->setInsertPoint(bodyBlock);

    MIRBlock *commaBlock = newBlock("comma");
    MIRBlock *elemBlock = newBlock("elem");
    MIRValue *isGtZero = builder->createICmp(
        CompareInst::Predicate::GT, idxVal,
        mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty),
        const_cast<hir::HIRModule *>(hirModule)->getBoolType(), "gt0", loc);
    builder->createCondBr(isGtZero, commaBlock, elemBlock);

    ensureBuiltinMIR("__moksha_string_concat");
    MIRFunction *concatFunc = mirModule->getFunction("__moksha_string_concat");
    if (!concatFunc) {
      auto fn2 = std::make_unique<MIRFunction>(strTy, "__moksha_string_concat",
                                               Linkage::External);
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), strTy, 0));
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), strTy, 1));
      concatFunc = fn2.get();
      mirModule->addFunction(std::move(fn2));
    }

    builder->setInsertPoint(commaBlock);
    MIRValue *commaStr = builder->createCall(
        cstrFunc,
        {mirModule->getOrInsertConstant<ConstantString>(", ", i8PtrTy)}, strTy,
        "comma", false, loc);
    MIRValue *curRes1 = builder->createLoad(resAlloca, "res.val1", loc);
    MIRValue *concat1 = builder->createCall(concatFunc, {curRes1, commaStr},
                                            strTy, "concat1", false, loc);
    builder->insert(std::make_unique<StoreInst>(concat1, resAlloca, loc));
    builder->createBr(elemBlock);

    builder->setInsertPoint(elemBlock);

    const hir::HIRType *abiAnyTy = getABICoercedType(anyTy, true);

    ensureBuiltinMIR("moksha_rt_map_get_key_at");
    MIRFunction *getKeyFunc =
        mirModule->getFunction("moksha_rt_map_get_key_at");
    if (!getKeyFunc) {
      auto fn2 = std::make_unique<MIRFunction>(
          abiAnyTy, "moksha_rt_map_get_key_at", Linkage::External);
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), voidPtrTy, 0));
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), i32Ty, 1));
      getKeyFunc = fn2.get();
      mirModule->addFunction(std::move(fn2));
    }
    const hir::HIRType *keyRetTy = getKeyFunc->getType();
    MIRValue *anyKey = builder->createCall(getKeyFunc, {mapPtr, idxVal},
                                           keyRetTy, "any.key.ptr", false, loc);
    if (anyKey->getType() != abiAnyTy &&
        anyKey->getType()->getKind() == hir::TypeKind::Pointer) {
      anyKey = builder->createBitCast(anyKey, abiAnyTy, "any.key.cast", loc);
    }
    if (abiAnyTy != anyTy) {
      anyKey = builder->createLoad(anyKey, "any.key", loc);
    }

    ensureBuiltinMIR("moksha_rt_map_get_val_at");
    MIRFunction *getValFunc =
        mirModule->getFunction("moksha_rt_map_get_val_at");
    if (!getValFunc) {
      auto fn2 = std::make_unique<MIRFunction>(
          abiAnyTy, "moksha_rt_map_get_val_at", Linkage::External);
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), voidPtrTy, 0));
      fn2->addArgument(std::make_unique<MIRArgument>(fn2.get(), i32Ty, 1));
      getValFunc = fn2.get();
      mirModule->addFunction(std::move(fn2));
    }
    const hir::HIRType *valRetTy = getValFunc->getType();
    MIRValue *anyVal = builder->createCall(getValFunc, {mapPtr, idxVal},
                                           valRetTy, "any.val.ptr", false, loc);
    if (anyVal->getType() != abiAnyTy &&
        anyVal->getType()->getKind() == hir::TypeKind::Pointer) {
      anyVal = builder->createBitCast(anyVal, abiAnyTy, "any.val.cast", loc);
    }
    if (abiAnyTy != anyTy) {
      anyVal = builder->createLoad(anyVal, "any.val", loc);
    }

    auto *mapTyInfo = static_cast<const hir::HIRMapType *>(actualColTy);
    MIRValue *keyVal = unboxValue(anyKey, anyTy, mapTyInfo->getKeyType(), loc);
    MIRValue *valVal =
        unboxValue(anyVal, anyTy, mapTyInfo->getValueType(), loc);

    MIRValue *keyStr = coerceToString(keyVal, loc);
    MIRValue *curRes2 = builder->createLoad(resAlloca, "res.val2", loc);
    MIRValue *concat2 = builder->createCall(concatFunc, {curRes2, keyStr},
                                            strTy, "concat2", false, loc);
    builder->insert(std::make_unique<StoreInst>(concat2, resAlloca, loc));

    MIRValue *colonStr = builder->createCall(
        cstrFunc,
        {mirModule->getOrInsertConstant<ConstantString>(": ", i8PtrTy)}, strTy,
        "colon", false, loc);
    MIRValue *curRes3 = builder->createLoad(resAlloca, "res.val3", loc);
    MIRValue *concat3 = builder->createCall(concatFunc, {curRes3, colonStr},
                                            strTy, "concat3", false, loc);
    builder->insert(std::make_unique<StoreInst>(concat3, resAlloca, loc));

    MIRValue *valStr = coerceToString(valVal, loc);
    MIRValue *curRes4 = builder->createLoad(resAlloca, "res.val4", loc);
    MIRValue *concat4 = builder->createCall(concatFunc, {curRes4, valStr},
                                            strTy, "concat4", false, loc);
    builder->insert(std::make_unique<StoreInst>(concat4, resAlloca, loc));

    MIRValue *nextIdx = builder->createAdd(
        idxVal, mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty),
        "idx.next", loc);
    builder->insert(std::make_unique<StoreInst>(nextIdx, idxAlloca, loc));
    builder->createBr(condBlock);

    // End Block
    builder->setInsertPoint(endBlock);
    MIRValue *closeBrack = builder->createCall(
        cstrFunc,
        {mirModule->getOrInsertConstant<ConstantString>(" }", i8PtrTy)}, strTy,
        "close", false, loc);
    MIRValue *curRes5 = builder->createLoad(resAlloca, "res.val5", loc);
    MIRValue *finalStr = builder->createCall(concatFunc, {curRes5, closeBrack},
                                             strTy, "concat5", false, loc);

    builder->insert(std::make_unique<ReturnInst>(finalStr, loc));

    // Restore
    builder->setInsertPoint(savedBlock);
    currFunc = savedFunc;
    scopeStack = savedScope;
    symbolMap = savedMap;

    return func;
  }

  MIRValue *coerceToString(MIRValue *val, SourceLocation loc) {
    if (val->getType()->getKind() == hir::TypeKind::String)
      return val;

    auto *stringTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();
    std::string typeName;
    const hir::HIRType *valTy = stripMemoryModifiers(val->getType());

    if (auto *nullTy = llvm::dyn_cast_or_null<hir::HIRNullableType>(valTy)) {
      MIRBlock *nullBlock = newBlock("opt.null");
      MIRBlock *validBlock = newBlock("opt.valid");
      MIRBlock *mergeBlock = newBlock("opt.merge");

      auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
      auto *nullConst =
          mirModule->getOrInsertConstant<ConstantNull>(val->getType());
      MIRValue *isNull = builder->createICmp(CompareInst::Predicate::EQ, val,
                                             nullConst, boolTy, "is.null", loc);
      builder->createCondBr(isNull, nullBlock, validBlock);

      // Null Path
      builder->setInsertPoint(nullBlock);
      ensureBuiltinMIR("__moksha_cstr_to_string");
      MIRFunction *cstrFunc = mirModule->getFunction("__moksha_cstr_to_string");
      auto *i8Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
      auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          i8Ty, hir::Ownership::None);
      if (!cstrFunc) {
        auto fn = std::make_unique<MIRFunction>(
            stringTy, "__moksha_cstr_to_string", Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i8PtrTy, 0));
        cstrFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      MIRValue *nullStr = builder->createCall(
          cstrFunc,
          {mirModule->getOrInsertConstant<ConstantString>("null", i8PtrTy)},
          stringTy, "null.str", false, loc);
      MIRBlock *nullEnd = builder->getInsertBlock();
      builder->createBr(mergeBlock);

      // Valid Path
      builder->setInsertPoint(validBlock);
      MIRValue *loadedVal = val;
      const hir::HIRType *innerTy = nullTy->getInner();

      // Load primitive values from the Optional pointer
      if (innerTy->getKind() == hir::TypeKind::Int ||
          innerTy->getKind() == hir::TypeKind::Float ||
          innerTy->getKind() == hir::TypeKind::Decimal ||
          innerTy->getKind() == hir::TypeKind::Bool) {
        auto *innerPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                innerTy, hir::Ownership::None);
        MIRValue *castPtr =
            builder->createBitCast(val, innerPtrTy, "opt.val.cast", loc);
        loadedVal = builder->insert(
            std::make_unique<LoadInst>(castPtr, "opt.val.load", loc));
      } else {
        auto *innerPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                innerTy, hir::Ownership::None);
        loadedVal =
            builder->createBitCast(val, innerPtrTy, "opt.ptr.cast", loc);
      }

      // Recursively format the unwrapped value
      MIRValue *validStr = coerceToString(loadedVal, loc);
      MIRBlock *validEnd = builder->getInsertBlock();
      builder->createBr(mergeBlock);

      // Merge Path
      builder->setInsertPoint(mergeBlock);
      auto phi = std::make_unique<PhiInst>(stringTy, "opt.str.phi", loc);
      phi->addIncoming(nullStr, nullEnd);
      phi->addIncoming(validStr, validEnd);
      return builder->insert(std::move(phi));
    }

    if (auto *intTy = llvm::dyn_cast_or_null<hir::HIRIntType>(valTy)) {
      if (intTy->isSize()) {
        typeName = intTy->isSigned() ? "isize" : "usize";
      } else {
        switch (intTy->getWidth()) {
        case 8:
          typeName = intTy->isSigned() ? "char" : "uchar";
          break;
        case 16:
          typeName = intTy->isSigned() ? "short" : "ushort";
          break;
        case 32:
          typeName = intTy->isSigned() ? "int" : "uint";
          break;
        case 64:
          typeName = intTy->isSigned() ? "long" : "ulong";
          break;
        default:
          typeName = "int";
          break;
        }
      }
    } else if (auto *floatTy =
                   llvm::dyn_cast_or_null<hir::HIRFloatType>(valTy)) {
      switch (floatTy->getWidth()) {
      case 8:
        typeName = "quarter";
        break;
      case 16:
        typeName = "half";
        break;
      case 32:
        typeName = "float";
        break;
      case 64:
        typeName = "double";
        break;
      default:
        typeName = "float";
        break;
      }
    } else if (valTy->getKind() == hir::TypeKind::Bool) {
      typeName = "bool";
    } else if (valTy->getKind() == hir::TypeKind::Decimal) {
      typeName = "decimal";
    } else if (valTy->getKind() == hir::TypeKind::Any) {
      typeName = "any";
    } else {
      bool isCString = false;
      if (auto *arrTy = llvm::dyn_cast_or_null<hir::ArrayType>(valTy)) {
        if (auto *intTy = llvm::dyn_cast_or_null<hir::HIRIntType>(
                arrTy->getElementType())) {
          if (intTy->getWidth() == 8)
            isCString = true;
        }
      } else if (auto *ptrTy =
                     llvm::dyn_cast_or_null<hir::PointerType>(valTy)) {
        if (auto *intTy =
                llvm::dyn_cast_or_null<hir::HIRIntType>(ptrTy->getPointee())) {
          if (intTy->getWidth() == 8)
            isCString = true;
        } else if (auto *arrTy = llvm::dyn_cast_or_null<hir::ArrayType>(
                       ptrTy->getPointee())) {
          if (auto *intTy = llvm::dyn_cast_or_null<hir::HIRIntType>(
                  arrTy->getElementType())) {
            if (intTy->getWidth() == 8)
              isCString = true;
          }
        }
      }

      const hir::HIRType *checkTy = valTy;
      if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(valTy)) {
        checkTy = pTy->getPointee();
      }

      if (isCString) {
        typeName = "cstr";
      } else if (checkTy->getKind() == hir::TypeKind::Array ||
                 checkTy->getKind() == hir::TypeKind::Slice) {
        MIRFunction *strFunc = getOrCreateArrayStringifier(valTy);
        return builder->createCall(
            strFunc, {val},
            const_cast<hir::HIRModule *>(hirModule)->getStringType(),
            "arr.to_str", false, loc);
      } else if (checkTy->getKind() == hir::TypeKind::Map) {
        MIRFunction *strFunc = getOrCreateMapStringifier(valTy);
        return builder->createCall(
            strFunc, {val},
            const_cast<hir::HIRModule *>(hirModule)->getStringType(),
            "map.to_str", false, loc);
      } else {
        typeName = "ptr";
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);
        val = builder->createBitCast(val, voidPtrTy, "ptr.cast", loc);
        valTy = voidPtrTy;
      }
    }

    std::string toStringName = "__moksha_" + typeName + "_to_string";
    MIRFunction *toStringFunc = mirModule->getFunction(toStringName);
    const hir::HIRType *abiValTy = getABICoercedType(valTy, true);

    if (!toStringFunc) {
      auto fn = std::make_unique<MIRFunction>(stringTy, toStringName,
                                              Linkage::External);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), abiValTy, 0));
      toStringFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    const hir::HIRType *expectedTy =
        toStringFunc->getRawArguments()[0]->getType();
    if (val->getType() != expectedTy) {
      if (expectedTy->getKind() == hir::TypeKind::Pointer &&
          val->getType()->getKind() != hir::TypeKind::Pointer) {
        auto *spill =
            builder->createAlloca(val->getType(), "abi.str.spill", loc);
        builder->insert(std::make_unique<StoreInst>(val, spill, loc));
        val = spill;
      }

      if (val->getType() != expectedTy) {
        val = builder->createBitCast(val, expectedTy, typeName + ".cast", loc);
      }
    }

    return builder->createCall(toStringFunc, {val}, stringTy,
                               typeName + ".to_str", false, loc);
  }

  // Helper function to evaluate string escapes down to raw bytes
  std::string unescapeString(const std::string &in) {
    std::string out;
    for (size_t i = 0; i < in.length(); ++i) {
      if (in[i] == '\\' && i + 1 < in.length()) {
        i++;
        switch (in[i]) {
        case 'n':
          out += '\n';
          break;
        case 't':
          out += '\t';
          break;
        case 'r':
          out += '\r';
          break;
        case '"':
          out += '"';
          break;
        case '\\':
          out += '\\';
          break;
        case 'x': { // Hex escape: \xNN
          if (i + 2 < in.length()) {
            std::string hex = in.substr(i + 1, 2);
            out += (char)std::strtol(hex.c_str(), nullptr, 16);
            i += 2;
          }
          break;
        }
        case 'u': { // Unicode escape: \u{NNNN}
          if (i + 1 < in.length() && in[i + 1] == '{') {
            size_t end = in.find('}', i + 2);
            if (end != std::string::npos) {
              std::string hex = in.substr(i + 2, end - i - 2);
              uint32_t cp = std::strtol(hex.c_str(), nullptr, 16);
              // Manually encode the code point to UTF-8
              if (cp <= 0x7F) {
                out += (char)cp;
              } else if (cp <= 0x7FF) {
                out += (char)(0xC0 | ((cp >> 6) & 0x1F));
                out += (char)(0x80 | (cp & 0x3F));
              } else if (cp <= 0xFFFF) {
                out += (char)(0xE0 | ((cp >> 12) & 0x0F));
                out += (char)(0x80 | ((cp >> 6) & 0x3F));
                out += (char)(0x80 | (cp & 0x3F));
              } else if (cp <= 0x10FFFF) {
                out += (char)(0xF0 | ((cp >> 18) & 0x07));
                out += (char)(0x80 | ((cp >> 12) & 0x3F));
                out += (char)(0x80 | ((cp >> 6) & 0x3F));
                out += (char)(0x80 | (cp & 0x3F));
              }
              i = end;
            }
          }
          break;
        }
        default:
          out += in[i];
          break;
        }
      } else {
        out += in[i];
      }
    }
    return out;
  }

  const hir::HIRType *getMIRType(const hir::HIRType *t) { return t; }

  MIRBlock *newBlock(const std::string &name) {
    std::string safeName = currFunc ? currFunc->getUniqueName(name) : name;
    auto block = std::make_unique<MIRBlock>(safeName, currFunc);
    MIRBlock *ptr = block.get();
    if (currFunc) {
      currFunc->addBlock(std::move(block));
    }
    return ptr;
  }

  void createFunctionDecl(const hir::HIRFunction *hirFunc,
                          std::string overrideName = "",
                          const hir::HIRType *thisType = nullptr) {
    std::string mirName =
        overrideName.empty() ? hirFunc->getName() : overrideName;

    if (hirFunc->isExtern() && !hirFunc->getABI().empty()) {
      std::string abi = hirFunc->getABI();
      if (abi != "stdcall" && abi != "fastcall" && abi != "cdecl" &&
          abi != "C" && abi != "vectorcall" && abi != "sysv64" &&
          abi != "win64") {
        mirName = abi;
      }
    }

    if (hirFunc->isExtern() &&
        (mirName == "yield" || mirName == "spawn" || mirName == "cancel" ||
         mirName == "select" || mirName == "timeout" || mirName == "sleep" ||
         mirName == "join")) {
      mirName = "moksha_builtin_" + mirName;
    }

    Linkage linkage = hirFunc->isWeak() ? Linkage::Weak : Linkage::External;

    bool isExternC = hirFunc->isExtern();
    const hir::HIRType *retTy = resolveType(hirFunc->getReturnType());
    retTy = getABICoercedType(retTy, isExternC);

    auto mirFunc = std::make_unique<MIRFunction>(retTy, mirName, linkage);
    std::string abi = hirFunc->getABI();
    if (abi == "stdcall")
      mirFunc->setCallingConv(CallingConv::StdCall);
    else if (abi == "fastcall")
      mirFunc->setCallingConv(CallingConv::FastCall);
    else if (abi == "vectorcall")
      mirFunc->setCallingConv(CallingConv::VectorCall);
    else if (abi == "sysv64")
      mirFunc->setCallingConv(CallingConv::SysV64);
    else if (abi == "win64")
      mirFunc->setCallingConv(CallingConv::Win64);

    if (hirFunc->isInterruptFunc()) {
      mirFunc->setCallingConv(CallingConv::Interrupt);
    }

    mirFunc->setVariadic(hirFunc->isVariadicFunc());
    mirFunc->setNaked(hirFunc->isNakedFunc());
    mirFunc->setNoReturn(hirFunc->isNoReturnFunc());
    mirFunc->setSection(hirFunc->getSection());
    mirFunc->setInline(hirFunc->isInlineFunc());
    mirFunc->setNoInline(hirFunc->isNoInlineFunc());
    mirFunc->setPure(hirFunc->isPureFunc());
    mirFunc->setCold(hirFunc->isColdFunc());
    mirFunc->setUsed(hirFunc->isUsedFunc());
    mirFunc->setInterrupt(hirFunc->isInterruptFunc());

    unsigned idx = 0;
    if (thisType) {
      auto arg = std::make_unique<MIRArgument>(mirFunc.get(),
                                               resolveType(thisType), idx++);
      arg->setName("this");
      applyBorrowKind(arg.get(), thisType);
      mirFunc->addArgument(std::move(arg));
    }

    for (const auto &p : hirFunc->getParams()) {
      const hir::HIRType *paramTy = resolveType(p.type);
      paramTy = getABICoercedType(paramTy, isExternC);

      auto arg = std::make_unique<MIRArgument>(mirFunc.get(), paramTy, idx++);
      arg->setName(p.getName());
      applyBorrowKind(arg.get(), p.getType());
      mirFunc->addArgument(std::move(arg));
    }
    mirModule->addFunction(std::move(mirFunc));
  }

  void createGlobalDecl(const hir::HIRStmt *stmt) {
    if (auto *varDecl = llvm::dyn_cast_or_null<hir::HIRVarDeclStmt>(stmt)) {
      MIRValue *initVal = nullptr;
      if (varDecl->getInit()) {
        expectedLambdaReturnType = varDecl->getType();
        visit(varDecl->getInit());
        expectedLambdaReturnType = nullptr;
        initVal = lastExprValue;
      }

      MIRValue *valToStore = initVal;
      const hir::HIRType *actualType = resolveType(varDecl->getType());
      const hir::HIRType *rawType = varDecl->getType();

      bool isManagedTarget = false;
      if (rawType && rawType->getKind() == hir::TypeKind::Pointer) {
        auto own =
            static_cast<const hir::PointerType *>(rawType)->getOwnership();
        if (own == hir::Ownership::Shared || own == hir::Ownership::Owned) {
          isManagedTarget = true;
        }
      }

      if (initVal && initVal->getType() &&
          (initVal->getType() != actualType || isManagedTarget)) {

        // Unwrap type strings for robust numeric coercion
        std::string tyStr = actualType->toString();
        bool isIntTy = actualType->getKind() == hir::TypeKind::Int ||
                       tyStr.find("i8") != std::string::npos ||
                       tyStr.find("i16") != std::string::npos ||
                       tyStr.find("i32") != std::string::npos ||
                       tyStr.find("i64") != std::string::npos ||
                       tyStr.find("isize") != std::string::npos ||
                       tyStr.find("usize") != std::string::npos;

        bool isFloatTy = actualType->getKind() == hir::TypeKind::Float ||
                         actualType->getKind() == hir::TypeKind::Decimal ||
                         tyStr.find("f32") != std::string::npos ||
                         tyStr.find("f64") != std::string::npos;

        // Automatically promote literals to match the expected type
        if (auto *cInt = llvm::dyn_cast_or_null<ConstantInt>(initVal);
            cInt && isIntTy) {
          valToStore = mirModule->getOrInsertConstant<ConstantInt>(
              cInt->getValue(), actualType);
        } else if (auto *cFloat =
                       llvm::dyn_cast_or_null<ConstantFloat>(initVal);
                   cFloat && isFloatTy) {
          valToStore = mirModule->getOrInsertConstant<ConstantFloat>(
              cFloat->getValue(), actualType);
        } else if (llvm::dyn_cast_or_null<ConstantNull>(initVal)) {
          // Coerce the Null pointer to the expected Variable Type
          valToStore = mirModule->getOrInsertConstant<ConstantNull>(actualType);
        } else if (actualType->getKind() == hir::TypeKind::Any ||
                   isManagedTarget) {
          valToStore =
              boxValue(initVal, initVal->getType(), rawType, varDecl->getLoc());
          if (valToStore->getType() != actualType) {
            valToStore = builder->createBitCast(
                valToStore, actualType, "init.managed.cast", varDecl->getLoc());
          }
        } else {
          // Fallback to emit BitCast for Dynamic Values
          bool isDestStruct = actualType->getKind() == hir::TypeKind::Struct;
          bool isSrcPtr =
              initVal->getType()->getKind() == hir::TypeKind::Pointer ||
              initVal->getType()->getKind() == hir::TypeKind::Reference;

          if (isSrcPtr && actualType->getKind() != hir::TypeKind::Pointer &&
              actualType->getKind() != hir::TypeKind::Reference &&
              actualType->getKind() != hir::TypeKind::String &&
              actualType->getKind() != hir::TypeKind::Slice &&
              actualType->getKind() != hir::TypeKind::Map) {
            auto *rawRefTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    actualType, isDestStruct ? hir::Ownership::Borrowed
                                             : hir::Ownership::None);
            auto *castPtr = builder->createBitCast(
                initVal, rawRefTy, "init.raw_ptr", varDecl->getLoc());
            valToStore = builder->insert(std::make_unique<LoadInst>(
                castPtr, "init.load", varDecl->getLoc()));
          } else {
            if (auto *mirConst = llvm::dyn_cast_or_null<MIRConstant>(initVal)) {
              valToStore = mirModule->getOrInsertConstant<ConstantBitCast>(
                  mirConst, actualType);
            } else {
              valToStore = builder->createBitCast(
                  initVal, actualType, "init.cast", varDecl->getLoc());
              applyBorrowKind(valToStore, actualType);
            }
          }
        }
      }

      Linkage linkage = varDecl->isStaticVar() ? Linkage::Internal
                        : varDecl->isWeakVar() ? Linkage::Weak
                                               : Linkage::External;

      bool isConstant = false;
      if (actualType && actualType->isImmutable()) {
        isConstant = true;
      }

      auto *global =
          builder->createGlobal(mirModule.get(), varDecl->getName(), rawType,
                                nullptr, isConstant, linkage);

      global->setThreadLocal(varDecl->isThreadLocalVar());
      global->setVolatile(varDecl->isVolatileVar());

      if (varDecl->getAlignment() > 0) {
        global->setAlignment(varDecl->getAlignment());
      }

      global->setUsed(varDecl->isUsedVar());
      global->setSection(varDecl->getSection());

      if (initVal) {
        if (auto *initConst = llvm::dyn_cast_or_null<MIRConstant>(valToStore)) {
          global->setInitializer(initConst);
        } else {
          // Set the global to null/zero initially
          global->setInitializer(
              mirModule->getOrInsertConstant<ConstantNull>(actualType));

          if (actualType->getKind() == hir::TypeKind::Array ||
              actualType->getKind() == hir::TypeKind::Struct) {
            MIRValue *srcPointer = valToStore;
            if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(valToStore)) {
              srcPointer = loadInst->getPointer();
              auto &insts = builder->getInsertBlock()->getInstructionsMut();
              insts.erase(
                  std::remove_if(insts.begin(), insts.end(),
                                 [&](const std::unique_ptr<MIRInst> &i) {
                                   return i.get() == loadInst;
                                 }),
                  insts.end());
            }

            auto *voidTy =
                const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            auto *voidPtrTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    voidTy, hir::Ownership::None);
            auto *i32Ty =
                const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);

            MIRValue *destVoidPtr = builder->createBitCast(
                global, voidPtrTy, "global.dest.void", varDecl->getLoc());
            MIRValue *srcVoidPtr = builder->createBitCast(
                srcPointer, voidPtrTy, "global.src.void", varDecl->getLoc());

            // Calculate Total Bytes (Sizeof trick)
            auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    actualType, hir::Ownership::None));
            auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
            auto *sizeGep =
                builder->createGEP(nullPtr, {one}, actualType,
                                   "global.sizeof.gep", varDecl->getLoc());
            auto *i64Ty =
                const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
            MIRValue *bytesToCopy = builder->insert(std::make_unique<CastInst>(
                Opcode::PtrToInt, sizeGep, i64Ty, "global.sizeof.i64",
                varDecl->getLoc()));

            // Inject memcpy
            std::string memcpyName = "__moksha_array_copy";
            ensureBuiltinMIR(memcpyName);
            MIRFunction *memcpyFunc = mirModule->getFunction(memcpyName);
            if (!memcpyFunc) {
              auto fn = std::make_unique<MIRFunction>(voidTy, memcpyName,
                                                      Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
              auto *i64Ty =
                  const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
              memcpyFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }
            builder->insert(std::make_unique<CallInst>(
                memcpyFunc,
                std::vector<MIRValue *>{destVoidPtr, srcVoidPtr, bytesToCopy},
                voidTy, "", false, varDecl->getLoc()));

          } else {
            if (valToStore->getType() != rawType) {
              valToStore = builder->createBitCast(
                  valToStore, rawType, "global.store.cast", varDecl->getLoc());
            }
            builder->insert(std::make_unique<StoreInst>(valToStore, global,
                                                        varDecl->getLoc()));
          }
        }
      } else {
        global->setExtern(varDecl->isExternVar());
        if (!varDecl->isExternVar()) {
          global->setInitializer(
              mirModule->getOrInsertConstant<ConstantNull>(actualType));
        }
      }
    }
  }

  MIRValue *coerceValue(MIRValue *val, const hir::HIRType *targetTy,
                        SourceLocation loc) {
    if (!val || !targetTy || val->getType() == targetTy)
      return val;

    auto srcKind = val->getType()->getKind();
    auto dstKind = targetTy->getKind();

    if (dstKind == hir::TypeKind::Nullable) {
      auto *nullTy = llvm::cast<hir::HIRNullableType>(targetTy);
      const hir::HIRType *innerTy = nullTy->getInner();

      if (val->getType()->toString() == innerTy->toString()) {
        if (innerTy->getKind() == hir::TypeKind::Int ||
            innerTy->getKind() == hir::TypeKind::Float ||
            innerTy->getKind() == hir::TypeKind::Decimal ||
            innerTy->getKind() == hir::TypeKind::Bool ||
            innerTy->getKind() == hir::TypeKind::Slice ||
            innerTy->getKind() == hir::TypeKind::Array) {
          return boxValue(val, val->getType(), targetTy, loc);
        } else {
          auto *ptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              innerTy, hir::Ownership::None);
          MIRValue *castPtr =
              builder->createBitCast(val, ptrTy, "opt.ptr.cast", loc);
          return builder->createBitCast(castPtr, targetTy, "opt.cast", loc);
        }
      } else if (llvm::isa<ConstantNull>(val)) {
        return mirModule->getOrInsertConstant<ConstantNull>(targetTy);
      }
    }

    if (srcKind == hir::TypeKind::Nullable) {
      auto *nullTy = llvm::cast<hir::HIRNullableType>(val->getType());
      const hir::HIRType *innerTy = nullTy->getInner();

      MIRValue *unboxed = val;
      if (innerTy->getKind() == hir::TypeKind::Int ||
          innerTy->getKind() == hir::TypeKind::Float ||
          innerTy->getKind() == hir::TypeKind::Decimal ||
          innerTy->getKind() == hir::TypeKind::Bool) {
        auto *ptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            innerTy, hir::Ownership::None);
        MIRValue *castPtr =
            builder->createBitCast(val, ptrTy, "coerce.unwrap.cast", loc);
        unboxed = builder->insert(
            std::make_unique<LoadInst>(castPtr, "coerce.unwrap.load", loc));
      } else {
        auto *ptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            innerTy, hir::Ownership::None);
        unboxed = builder->createBitCast(val, ptrTy, "coerce.unwrap.cast", loc);
      }
      return coerceValue(unboxed, targetTy, loc);
    }

    if (srcKind == hir::TypeKind::Int && dstKind == hir::TypeKind::Struct) {
      auto *spill = builder->createAlloca(targetTy, "coerce.spill", loc);
      auto *ptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          val->getType(), hir::Ownership::None);
      auto *castPtr = builder->createBitCast(spill, ptrTy, "coerce.cast", loc);
      builder->insert(std::make_unique<StoreInst>(val, castPtr, loc));
      return builder->insert(
          std::make_unique<LoadInst>(spill, "coerce.load", loc));
    }
    if (srcKind == hir::TypeKind::Struct && dstKind == hir::TypeKind::Int) {
      auto *spill = builder->createAlloca(val->getType(), "coerce.spill", loc);
      builder->insert(std::make_unique<StoreInst>(val, spill, loc));
      auto *ptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          targetTy, hir::Ownership::None);
      auto *castPtr = builder->createBitCast(spill, ptrTy, "coerce.cast", loc);
      return builder->insert(
          std::make_unique<LoadInst>(castPtr, "coerce.load", loc));
    }

    // Float to Decimal Conversion
    if (srcKind == hir::TypeKind::Float && dstKind == hir::TypeKind::Decimal) {
      auto *decTy = static_cast<const hir::HIRDecimalType *>(targetTy);
      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      auto *f64Ty = const_cast<hir::HIRModule *>(hirModule)->getFloatType(64);
      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          voidTy, hir::Ownership::None);

      MIRValue *f64Val = val;
      if (static_cast<const hir::HIRFloatType *>(val->getType())->getWidth() !=
          64) {
        f64Val = builder->createBitCast(val, f64Ty, "float.ext", loc);
      }
      MIRValue *outPtr = builder->createAlloca(targetTy, "dec.out", loc);
      MIRValue *scaleVal =
          mirModule->getOrInsertConstant<ConstantInt>(decTy->getScale(), i32Ty);
      std::string funcName = "__moksha_f64_to_decimal";
      ensureBuiltinMIR(funcName);
      MIRFunction *f64ToDec = mirModule->getFunction(funcName);
      if (!f64ToDec) {
        auto fn =
            std::make_unique<MIRFunction>(voidTy, funcName, Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), f64Ty, 1));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 2));
        f64ToDec = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *castOutPtr =
          builder->createBitCast(outPtr, voidPtrTy, "out.cast", loc);
      builder->createCall(f64ToDec, {castOutPtr, f64Val, scaleVal}, voidTy, "",
                          false, loc);

      return builder->insert(
          std::make_unique<LoadInst>(outPtr, "dec.load", loc));
    }

    // Decimal to Float Conversion
    if (srcKind == hir::TypeKind::Decimal && dstKind == hir::TypeKind::Float) {
      auto *f64Ty = const_cast<hir::HIRModule *>(hirModule)->getFloatType(64);
      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          voidTy, hir::Ownership::None);
      MIRValue *spill = builder->createAlloca(val->getType(), "dec.spill", loc);
      builder->insert(std::make_unique<StoreInst>(val, spill, loc));
      std::string funcName = "__moksha_decimal_to_f64";
      ensureBuiltinMIR(funcName);
      MIRFunction *decToF64 = mirModule->getFunction(funcName);
      if (!decToF64) {
        auto fn =
            std::make_unique<MIRFunction>(f64Ty, funcName, Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        decToF64 = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      MIRValue *castSpill =
          builder->createBitCast(spill, voidPtrTy, "spill.cast", loc);
      MIRValue *f64Val = builder->createCall(decToF64, {castSpill}, f64Ty,
                                             "f64.val", false, loc);
      auto *targetFltTy = static_cast<const hir::HIRFloatType *>(targetTy);
      if (targetFltTy->getWidth() != 64) {
        return builder->createBitCast(f64Val, targetTy, "float.trunc", loc);
      }
      return f64Val;
    }

    if (srcKind == hir::TypeKind::Function &&
        dstKind == hir::TypeKind::Pointer) {
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(targetTy)) {
        if (ptrTy->getPointee()->getKind() == hir::TypeKind::Function) {
          return val;
        }
      }
    }

    // Decimal to Decimal Conversion (Struct Bitcast Bypass)
    if (srcKind == hir::TypeKind::Decimal &&
        dstKind == hir::TypeKind::Decimal) {
      auto *targetPtrTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              targetTy, hir::Ownership::None);

      MIRValue *spill = builder->createAlloca(val->getType(), "dec.spill", loc);
      builder->insert(std::make_unique<StoreInst>(val, spill, loc));

      MIRValue *castPtr =
          builder->createBitCast(spill, targetPtrTy, "dec.ptr.cast", loc);
      return builder->insert(
          std::make_unique<LoadInst>(castPtr, "dec.reload", loc));
    }

    // INT TO FLOAT
    if (srcKind == hir::TypeKind::Int && dstKind == hir::TypeKind::Float) {
      return builder->insert(std::make_unique<CastInst>(
          Opcode::IntToFloat, val, targetTy, "coerce.sitofp", loc));
    }
    // FLOAT TO INT
    if (srcKind == hir::TypeKind::Float && dstKind == hir::TypeKind::Int) {
      return builder->insert(std::make_unique<CastInst>(
          Opcode::FloatToInt, val, targetTy, "coerce.fptosi", loc));
    }

    return builder->createBitCast(val, targetTy, "coerce.bitcast", loc);
  }

  // Visitor Implementations
  void visitFunction(const hir::HIRFunction &func) override {
    lowerFunction(func);
  }

  void visitClass(const hir::HIRClass &cls) override {
    for (const auto &method : cls.getMethods()) {
      if (method) {
        visitFunction(*method);
      }
    }
  }

  void lowerFunction(const hir::HIRFunction &func,
                     std::string overrideName = "",
                     const hir::HIRType *thisType = nullptr) {
    scopeStack.clear();
    scopeStack.push_back({});
    currentASTFuncRetTy = func.getReturnType();

    while (!breakScopeDepths.empty())
      breakScopeDepths.pop();
    while (!continueScopeDepths.empty())
      continueScopeDepths.pop();

    std::string mirName = overrideName.empty() ? func.getName() : overrideName;
    currFunc = mirModule->getFunction(mirName);

    symbolMap.clear();
    currentUnwindDest = nullptr;
    while (!loopCondBlocks.empty())
      loopCondBlocks.pop();
    while (!loopMergeBlocks.empty())
      loopMergeBlocks.pop();

    MIRBlock *entryBlock = newBlock("entry");
    builder->setInsertPoint(entryBlock);

    const auto &mirArgs = currFunc->getArguments();
    const auto &hirParams = func.getParams();

    size_t argIdx = 0;

    if (thisType && argIdx < mirArgs.size()) {
      MIRArgument *thisArg = mirArgs[argIdx].get();
      auto *alloca =
          builder->createAlloca(thisArg->getType(), "this.addr", func.getLoc());
      builder->insert(
          std::make_unique<StoreInst>(thisArg, alloca, func.getLoc()));
      symbolMap["this"] = alloca;
      argIdx++;
    }

    for (size_t i = 0; i < hirParams.size(); ++i) {
      if (argIdx >= mirArgs.size())
        break;

      MIRArgument *arg = mirArgs[argIdx].get();
      const hir::HIRType *actualType = resolveType(arg->getType());
      auto *alloca = builder->createAlloca(
          actualType, hirParams[i].getName() + ".addr", hirParams[i].getLoc());

      builder->insert(
          std::make_unique<StoreInst>(arg, alloca, hirParams[i].getLoc()));
      symbolMap[hirParams[i].getName()] = alloca;

      const hir::HIRType *rawParamTy = arg->getType();
      if (rawParamTy) {
        std::string tyStr = rawParamTy->toString();
        if (auto *ptrTy =
                llvm::dyn_cast_or_null<hir::PointerType>(rawParamTy)) {
          if (ptrTy->getOwnership() == hir::Ownership::Shared)
            scopeStack.back().refCountedVars.push_back(alloca);
          else if (ptrTy->getOwnership() != hir::Ownership::Borrowed &&
                   ptrTy->getOwnership() != hir::Ownership::None) {
            scopeStack.back().ownedVars.push_back(alloca);
          }
        } else if (!llvm::dyn_cast_or_null<hir::ReferenceType>(rawParamTy)) {
          if (tyStr.find("Arc<") != std::string::npos ||
              tyStr.find("shared ") != std::string::npos) {
            scopeStack.back().refCountedVars.push_back(alloca);
          } else if (tyStr.find("Box<") != std::string::npos ||
                     rawParamTy->getKind() == hir::TypeKind::Struct ||
                     rawParamTy->getKind() == hir::TypeKind::Any ||
                     rawParamTy->getKind() == hir::TypeKind::Closure ||
                     rawParamTy->getKind() == hir::TypeKind::String ||
                     rawParamTy->getKind() == hir::TypeKind::Array ||
                     rawParamTy->getKind() == hir::TypeKind::Map ||
                     rawParamTy->getKind() == hir::TypeKind::Slice ||
                     rawParamTy->getKind() == hir::TypeKind::Nullable ||
                     rawParamTy->getKind() == hir::TypeKind::Promise) {
            scopeStack.back().ownedVars.push_back(alloca);
          }
        }
      }
      argIdx++;
    }

    if (mirName.find(".constructor") != std::string::npos &&
        thisType != nullptr) {
      std::string className = mirName.substr(0, mirName.find(".constructor"));
      const hir::HIRClass *targetCls = nullptr;
      for (const auto *cls : hirModule->getClasses()) {
        if (cls->getName() == className) {
          targetCls = cls;
          break;
        }
      }

      if (targetCls) {
        for (const auto *parentTy : targetCls->getParentTypes()) {
          std::string pName = parentTy->toString();
          while (!pName.empty() && !isalnum(pName[0]))
            pName = pName.substr(1);

          std::string parentCtorName = pName + ".constructor_ret_void";
          if (MIRFunction *parentCtor =
                  mirModule->getFunction(parentCtorName)) {
            MIRValue *thisAddr = symbolMap["this"];
            MIRValue *loadedThis =
                builder->createLoad(thisAddr, "this.val", func.getLoc());

            const hir::HIRType *expectedThisTy =
                parentCtor->getRawArguments()[0]->getType();
            MIRValue *castedThis = builder->createBitCast(
                loadedThis, expectedThisTy, "base.cast", func.getLoc());

            builder->createCall(
                parentCtor, {castedThis},
                const_cast<hir::HIRModule *>(hirModule)->getVoidType(), "",
                false, func.getLoc());
          }
        }
      }
    }

    if (func.getBody()) {
      if (func.getName() == "main") {
        MIRBlock *tryBodyBlock = newBlock("main.try.body");
        MIRBlock *lpadBlock = newBlock("main.lpad");

        builder->createBr(tryBodyBlock);
        builder->setInsertPoint(tryBodyBlock);

        MIRBlock *oldUnwind = currentUnwindDest;
        currentUnwindDest = lpadBlock;
        tryScopeDepths.push(scopeStack.size());

        visit(func.getBody());

        tryScopeDepths.pop();
        currentUnwindDest = oldUnwind;

        if (!getTerminator(builder->getInsertBlock())) {
          emitScopeCleanup(scopeStack.size() - 1, func.getLoc());
          if (func.isAsyncFunc()) {
            std::string funcName = "moksha_rt_make_resolved_promise";
            ensureBuiltinMIR(funcName);
            MIRFunction *makePromFunc = mirModule->getFunction(funcName);
            auto *voidTy =
                const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            auto *voidPtrTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    voidTy, hir::Ownership::None);

            if (!makePromFunc) {
              auto fn = std::make_unique<MIRFunction>(voidPtrTy, funcName,
                                                      Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              makePromFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }

            MIRValue *nullArg =
                mirModule->getOrInsertConstant<ConstantNull>(voidPtrTy);
            MIRValue *rawProm =
                builder->createCall(makePromFunc, {nullArg}, voidPtrTy,
                                    "resolved.prom.def", false, func.getLoc());

            MIRValue *finalRet = builder->createBitCast(
                rawProm, currFunc->getType(), "prom.cast", func.getLoc());
            builder->insert(
                std::make_unique<ReturnInst>(finalRet, func.getLoc()));
          } else {
            builder->insert(
                std::make_unique<ReturnInst>(nullptr, func.getLoc()));
          }
        }

        // Catch-All Block
        builder->setInsertPoint(lpadBlock);
        auto *i8Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
        auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            i8Ty, hir::Ownership::None);
        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
        auto *lpadType = const_cast<hir::HIRModule *>(hirModule)->getStructType(
            "eh_result", {i8PtrTy, i32Ty});
        auto *lpadInst =
            builder->createLandingPad(lpadType, "main.lpad", func.getLoc());

        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);
        lpadInst->addCatchType(voidPtrTy);

        emitScopeCleanup(scopeStack.size() - 1, func.getLoc());

        std::string panicName = "moksha_rt_panic";
        ensureBuiltinMIR(panicName);
        MIRFunction *panicFunc = mirModule->getFunction(panicName);
        if (!panicFunc) {
          auto fn = std::make_unique<MIRFunction>(voidTy, panicName,
                                                  Linkage::External);
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i8PtrTy, 0));
          panicFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        std::string unesc = "Unhandled Exception reached main() boundary.";
        MIRValue *msg =
            mirModule->getOrInsertConstant<ConstantString>(unesc, i8PtrTy);

        builder->createCall(panicFunc, {msg}, voidTy, "", false, func.getLoc());
        builder->insert(std::make_unique<UnreachableInst>(func.getLoc()));
      } else if (func.isAsyncFunc()) {
        MIRBlock *tryBlock = newBlock("async.try");
        MIRBlock *catchLpadBlock = newBlock("async.catch.lpad");
        MIRBlock *catchBodyBlock = newBlock("async.catch.body");
        MIRBlock *contBlock = newBlock("async.cont");

        builder->createBr(tryBlock);
        builder->setInsertPoint(tryBlock);

        MIRBlock *oldUnwind = currentUnwindDest;
        MIRBlock *oldUnwindBody = currentUnwindBody;
        currentUnwindDest = catchLpadBlock;
        currentUnwindBody = catchBodyBlock;
        tryScopeDepths.push(scopeStack.size());

        visit(func.getBody());

        tryScopeDepths.pop();
        currentUnwindDest = oldUnwind;
        currentUnwindBody = oldUnwindBody;

        if (!getTerminator(builder->getInsertBlock())) {
          builder->createBr(contBlock);
        }

        builder->setInsertPoint(catchLpadBlock);

        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);

        auto *i8Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
        auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            i8Ty, hir::Ownership::None);
        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
        auto *lpadType = const_cast<hir::HIRModule *>(hirModule)->getStructType(
            "eh_result", {i8PtrTy, i32Ty});

        auto *lpadInst =
            builder->createLandingPad(lpadType, "async.lpad", func.getLoc());
        lpadInst->addCatchType(voidPtrTy);

        builder->createBr(catchBodyBlock);
        builder->setInsertPoint(catchBodyBlock);
        std::string consumeName = "moksha_rt_consume_exception";
        ensureBuiltinMIR(consumeName);
        MIRFunction *consumeFunc = mirModule->getFunction(consumeName);
        if (!consumeFunc) {
          auto fn = std::make_unique<MIRFunction>(voidPtrTy, consumeName,
                                                  Linkage::External);
          consumeFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }
        MIRValue *exPayload = builder->createCall(
            consumeFunc, {}, voidPtrTy, "ex.consumed", false, func.getLoc());

        std::string rejectName = "moksha_rt_make_rejected_promise";
        ensureBuiltinMIR(rejectName);
        MIRFunction *rejectFunc = mirModule->getFunction(rejectName);
        if (!rejectFunc) {
          auto fn = std::make_unique<MIRFunction>(voidPtrTy, rejectName,
                                                  Linkage::External);
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
          rejectFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        MIRValue *rejectedProm =
            builder->createCall(rejectFunc, {exPayload}, voidPtrTy,
                                "rejected.prom", false, func.getLoc());

        const hir::HIRType *promTy = currFunc->getType();
        MIRValue *finalRet = builder->createBitCast(rejectedProm, promTy,
                                                    "prom.cast", func.getLoc());

        emitScopeCleanup(scopeStack.size() - 1, func.getLoc());
        builder->insert(std::make_unique<ReturnInst>(finalRet, func.getLoc()));
        builder->setInsertPoint(contBlock);
      } else {
        visit(func.getBody());
      }
    }

    if (!getTerminator(builder->getInsertBlock())) {
      if (mirName.find(".destructor") != std::string::npos &&
          thisType != nullptr) {
        std::string className = mirName.substr(0, mirName.find(".destructor"));
        const hir::HIRClass *targetCls = nullptr;
        for (const auto *cls : hirModule->getClasses()) {
          if (cls->getName() == className) {
            targetCls = cls;
            break;
          }
        }

        if (targetCls) {
          for (const auto *parentTy : targetCls->getParentTypes()) {
            std::string pName = parentTy->toString();
            while (!pName.empty() && !isalnum(pName[0]))
              pName = pName.substr(1);

            std::string parentDtorName = pName + ".destructor_ret_void";
            if (MIRFunction *parentDtor =
                    mirModule->getFunction(parentDtorName)) {
              MIRValue *thisAddr = symbolMap["this"];
              MIRValue *loadedThis =
                  builder->createLoad(thisAddr, "this.val", func.getLoc());

              const hir::HIRType *expectedThisTy =
                  parentDtor->getRawArguments()[0]->getType();
              MIRValue *castedThis = builder->createBitCast(
                  loadedThis, expectedThisTy, "base.cast", func.getLoc());

              builder->createCall(
                  parentDtor, {castedThis},
                  const_cast<hir::HIRModule *>(hirModule)->getVoidType(), "",
                  false, func.getLoc());
            }
          }
        }
      }

      emitScopeCleanup(scopeStack.size() - 1, func.getLoc());

      // AUTOMATIC FIELD CLEANUP IN DESTRUCTORS
      if (mirName.find(".destructor") != std::string::npos &&
          thisType != nullptr) {
        std::string className = mirName.substr(0, mirName.find(".destructor"));
        const hir::HIRClass *targetCls = nullptr;
        for (const auto *cls : hirModule->getClasses()) {
          if (cls->getName() == className) {
            targetCls = cls;
            break;
          }
        }

        if (targetCls && symbolMap.count("this")) {
          const hir::HIRType *actualTy = targetCls->getType();
          if (auto *ptrTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(actualTy)) {
            actualTy = ptrTy->getPointee();
          }

          if (auto *stTy = llvm::dyn_cast_or_null<hir::StructType>(actualTy)) {
            MIRValue *thisAddr = symbolMap["this"];
            MIRValue *loadedThis =
                builder->createLoad(thisAddr, "this.val", func.getLoc());

            auto *structPtrTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    stTy, hir::Ownership::None);
            if (loadedThis->getType() != structPtrTy) {
              loadedThis = builder->createBitCast(
                  loadedThis, structPtrTy, "this.struct.cast", func.getLoc());
            }

            auto *i32Ty =
                const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
            auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);

            for (size_t i = 0; i < stTy->getFields().size(); ++i) {
              const hir::HIRType *fieldTy = stTy->getFields()[i];

              bool isManaged = false;
              if (auto *ptrTy =
                      llvm::dyn_cast_or_null<hir::PointerType>(fieldTy)) {
                if (ptrTy->getOwnership() != hir::Ownership::None &&
                    ptrTy->getOwnership() != hir::Ownership::Borrowed) {
                  isManaged = true;
                }
              } else if (auto *nullTy =
                             llvm::dyn_cast_or_null<hir::HIRNullableType>(
                                 fieldTy)) {
                const hir::HIRType *inner = nullTy->getInner();
                if (auto *innerPtr =
                        llvm::dyn_cast_or_null<hir::PointerType>(inner)) {
                  if (innerPtr->getOwnership() != hir::Ownership::None &&
                      innerPtr->getOwnership() != hir::Ownership::Borrowed) {
                    isManaged = true;
                  }
                } else if (inner->getKind() == hir::TypeKind::Struct) {
                  isManaged = true;
                }
              } else if (fieldTy->getKind() == hir::TypeKind::String ||
                         fieldTy->getKind() == hir::TypeKind::Array ||
                         fieldTy->getKind() == hir::TypeKind::Map ||
                         fieldTy->getKind() == hir::TypeKind::Closure ||
                         fieldTy->getKind() == hir::TypeKind::Any ||
                         fieldTy->getKind() == hir::TypeKind::Struct) {
                isManaged = true;
              } else {
                std::string tyStr = fieldTy->toString();
                if (tyStr.find("Arc<") != std::string::npos ||
                    tyStr.find("Box<") != std::string::npos ||
                    tyStr.find("?") != std::string::npos) {
                  isManaged = true;
                }
              }

              if (fieldTy->getKind() == hir::TypeKind::Weak ||
                  fieldTy->toString().find("weak") != std::string::npos) {
                isManaged = false;
              }

              if (isManaged) {
                auto *idx =
                    mirModule->getOrInsertConstant<ConstantInt>(i, i32Ty);
                MIRValue *fieldGep = builder->createGEP(
                    loadedThis, {zero, idx}, stTy, "cap.gep", func.getLoc());

                auto *expectedPtrTy =
                    const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                        fieldTy, hir::Ownership::None);
                if (fieldGep->getType() != expectedPtrTy) {
                  fieldGep = builder->createBitCast(
                      fieldGep, expectedPtrTy, "field.gep.cast", func.getLoc());
                }

                MIRValue *fieldVal =
                    builder->createLoad(fieldGep, "field.load", func.getLoc());
                std::string dropName = "";
                const hir::HIRType *baseFieldTy = fieldTy;
                while (baseFieldTy) {
                  if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(
                          baseFieldTy)) {
                    baseFieldTy = pTy->getPointee();
                  } else if (auto *rTy =
                                 llvm::dyn_cast_or_null<hir::ReferenceType>(
                                     baseFieldTy)) {
                    baseFieldTy = rTy->getInner();
                  } else if (auto *nTy =
                                 llvm::dyn_cast_or_null<hir::HIRNullableType>(
                                     baseFieldTy)) {
                    baseFieldTy = nTy->getInner();
                  } else {
                    break;
                  }
                }

                if (baseFieldTy &&
                    baseFieldTy->getKind() == hir::TypeKind::Struct) {
                  std::string fName = baseFieldTy->toString();
                  while (!fName.empty() &&
                         (fName[0] == '&' || fName[0] == '*' ||
                          fName[0] == ' ' || fName[0] == '?'))
                    fName = fName.substr(1);
                  if (fName.find("struct ") == 0)
                    fName = fName.substr(7);
                  if (fName.find("class ") == 0)
                    fName = fName.substr(6);
                  if (fName.find("Arc<") == 0)
                    fName = fName.substr(4, fName.length() - 5);
                  if (fName.find("Box<") == 0)
                    fName = fName.substr(4, fName.length() - 5);
                  if (!fName.empty() && fName.back() == '?')
                    fName.pop_back();

                  dropName = fName + ".destructor_ret_void";
                }

                MIRFunction *dropFunc = dropName.empty()
                                            ? nullptr
                                            : mirModule->getFunction(dropName);
                builder->insert(std::make_unique<ARCInst>(
                    Opcode::Release, fieldVal, dropFunc, func.getLoc()));
              }
            }
          }
        }
      }

      if (!getTerminator(builder->getInsertBlock())) {
        if (currFunc->isNoReturn()) {
          builder->insert(std::make_unique<UnreachableInst>(func.getLoc()));
        } else {
          const hir::HIRType *expectedTy = currFunc->getType();
          bool isVoidReturn =
              !expectedTy || expectedTy->getKind() == hir::TypeKind::Void;

          if (!isVoidReturn) {
            MIRValue *defVal = nullptr;
            if (expectedTy->getKind() == hir::TypeKind::Promise) {
              std::string funcName = "moksha_rt_make_resolved_promise";
              ensureBuiltinMIR(funcName);
              MIRFunction *makePromFunc = mirModule->getFunction(funcName);
              auto *voidTy =
                  const_cast<hir::HIRModule *>(hirModule)->getVoidType();
              auto *voidPtrTy =
                  const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                      voidTy, hir::Ownership::None);

              if (!makePromFunc) {
                auto fn = std::make_unique<MIRFunction>(voidPtrTy, funcName,
                                                        Linkage::External);
                fn->addArgument(
                    std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
                makePromFunc = fn.get();
                mirModule->addFunction(std::move(fn));
              }
              MIRValue *nullArg =
                  mirModule->getOrInsertConstant<ConstantNull>(voidPtrTy);
              MIRValue *rawProm = builder->createCall(
                  makePromFunc, {nullArg}, voidPtrTy, "resolved.prom.def",
                  false, func.getLoc());
              defVal = builder->createBitCast(rawProm, expectedTy, "prom.cast",
                                              func.getLoc());
            } else if (expectedTy->getKind() == hir::TypeKind::Int)
              defVal =
                  mirModule->getOrInsertConstant<ConstantInt>(0, expectedTy);
            else if (expectedTy->getKind() == hir::TypeKind::Float ||
                     expectedTy->getKind() == hir::TypeKind::Decimal)
              defVal = mirModule->getOrInsertConstant<ConstantFloat>(
                  0.0, expectedTy);
            else if (expectedTy->getKind() == hir::TypeKind::Bool)
              defVal = mirModule->getOrInsertConstant<ConstantBool>(false,
                                                                    expectedTy);
            else
              defVal = mirModule->getOrInsertConstant<ConstantNull>(expectedTy);

            builder->insert(
                std::make_unique<ReturnInst>(defVal, func.getLoc()));
          } else {
            builder->insert(
                std::make_unique<ReturnInst>(nullptr, func.getLoc()));
          }
        }
      }
    }

    scopeStack.pop_back();
    currFunc->numberUnnamedValues();
  }

  void visitBlockStmt(const hir::BlockStmt &stmt) override {
    auto oldSymbolMap = symbolMap;
    scopeStack.push_back({});

    for (const auto &s : stmt.getStatements()) {
      visit(s.get());
      if (builder->getInsertBlock() && getTerminator(builder->getInsertBlock()))
        break;
    }

    if (builder->getInsertBlock() &&
        !getTerminator(builder->getInsertBlock())) {
      emitScopeCleanup(scopeStack.size() - 1, stmt.getLoc());
    }

    scopeStack.pop_back();
    symbolMap = oldSymbolMap;
  }

  void visitReturnStmt(const hir::ReturnStmt &stmt) override {
    MIRValue *retVal = nullptr;

    if (stmt.getReturnValue()) {
      const hir::HIRType *oldExpected = expectedLambdaReturnType;
      if (currFunc && currFunc->getType() &&
          currFunc->getType()->getKind() != hir::TypeKind::Void) {
        expectedLambdaReturnType = currFunc->getType();
      }

      bool oldEscape = inEscapeContext;
      inEscapeContext = true;

      if (auto *ident = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(
              stmt.getReturnValue())) {
        std::string name = ident->getName();
        if (symbolMap.count(name)) {
          MIRValue *ptr = symbolMap[name];
          retVal = builder->insert(
              std::make_unique<LoadInst>(ptr, name + ".val", stmt.getLoc()));
          lastExprValue = retVal;
        } else if (MIRGlobal *g = mirModule->getGlobal(name)) {
          retVal = builder->insert(
              std::make_unique<LoadInst>(g, name + ".val", stmt.getLoc()));
          lastExprValue = retVal;
        } else {
          visit(stmt.getReturnValue());
          retVal = lastExprValue;
        }
      } else {
        visit(stmt.getReturnValue());
        retVal = lastExprValue;
      }

      inEscapeContext = oldEscape;
      expectedLambdaReturnType = oldExpected;

      if (retVal && currFunc) {
        const hir::HIRType *expectedTy = currFunc->getType();
        if (expectedTy && expectedTy->getKind() != hir::TypeKind::Void &&
            retVal->getType() != expectedTy) {

          if (expectedTy->getKind() == hir::TypeKind::Promise) {
            std::string funcName = "moksha_rt_make_resolved_promise";
            ensureBuiltinMIR(funcName);
            MIRFunction *makePromFunc = mirModule->getFunction(funcName);

            auto *voidTy =
                const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            auto *voidPtrTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    voidTy, hir::Ownership::None);

            if (!makePromFunc) {
              auto fn = std::make_unique<MIRFunction>(voidPtrTy, funcName,
                                                      Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              makePromFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }

            MIRValue *arg = retVal;
            if (arg->getType() != voidPtrTy) {
              arg = builder->createBitCast(arg, voidPtrTy, "prom.arg.cast",
                                           stmt.getLoc());
            }

            MIRValue *rawProm =
                builder->createCall(makePromFunc, {arg}, voidPtrTy,
                                    "resolved.prom", false, stmt.getLoc());
            retVal = builder->createBitCast(rawProm, expectedTy, "prom.cast",
                                            stmt.getLoc());
          }
          // Standard Coercion
          else if (llvm::dyn_cast_or_null<ConstantNull>(retVal)) {
            retVal = mirModule->getOrInsertConstant<ConstantNull>(expectedTy);
          } else if (expectedTy->getKind() == hir::TypeKind::Any ||
                     (expectedTy->getKind() == hir::TypeKind::Pointer &&
                      (static_cast<const hir::PointerType *>(expectedTy)
                               ->getOwnership() == hir::Ownership::Shared ||
                       static_cast<const hir::PointerType *>(expectedTy)
                               ->getOwnership() == hir::Ownership::Owned))) {
            retVal =
                boxValue(retVal, retVal->getType(), expectedTy, stmt.getLoc());
            if (retVal->getType() != expectedTy) {
              retVal = builder->createBitCast(
                  retVal, expectedTy, "ret.managed.cast", stmt.getLoc());
            }
          } else if (retVal->getType()->getKind() == hir::TypeKind::Any) {
            retVal = unboxValue(retVal, retVal->getType(), expectedTy,
                                stmt.getLoc());
          } else {
            retVal = coerceValue(retVal, expectedTy, stmt.getLoc());
          }
        }
      }
    }

    MIRValue *retOriginAlloca = nullptr;
    if (retVal) {
      MIRValue *trace = retVal;
      while (auto *cast = llvm::dyn_cast_or_null<CastInst>(trace)) {
        trace = cast->getValue();
      }

      if (auto *load = llvm::dyn_cast_or_null<LoadInst>(trace)) {
        retOriginAlloca = load->getPointer();
      }
    }

    for (size_t i = scopeStack.size(); i > 0; --i) {
      size_t scopeIdx = i - 1;
      if (retOriginAlloca) {
        auto &owned = scopeStack[scopeIdx].ownedVars;
        owned.erase(std::remove(owned.begin(), owned.end(), retOriginAlloca),
                    owned.end());

        auto &shared = scopeStack[scopeIdx].refCountedVars;
        shared.erase(std::remove(shared.begin(), shared.end(), retOriginAlloca),
                     shared.end());
      }

      emitScopeCleanup(scopeIdx, stmt.getLoc(), true);
    }

    if (!getTerminator(builder->getInsertBlock())) {
      builder->insert(std::make_unique<ReturnInst>(retVal, stmt.getLoc()));
    }
  }

  void visitIfStmt(const hir::IfStmt &stmt) override {
    visit(stmt.getCondition());
    MIRValue *cond = coerceToBool(lastExprValue, stmt.getLoc());
    MIRBlock *thenBlock = newBlock("if.then");
    MIRBlock *elseBlock = newBlock("if.else");
    MIRBlock *mergeBlock = newBlock("if.end");

    builder->createCondBr(cond, thenBlock, elseBlock);

    builder->setInsertPoint(thenBlock);
    visit(stmt.getThenBranch());
    if (!getTerminator(builder->getInsertBlock())) {
      builder->createBr(mergeBlock);
    }

    builder->setInsertPoint(elseBlock);
    if (stmt.getElseBranch())
      visit(stmt.getElseBranch());
    if (!getTerminator(builder->getInsertBlock())) {
      builder->createBr(mergeBlock);
    }

    builder->setInsertPoint(mergeBlock);
    lastExprValue = nullptr;
  }

  void visitWhileStmt(const hir::WhileStmt &stmt) override {
    MIRBlock *condBlock = newBlock("while.cond");
    MIRBlock *bodyBlock = newBlock("while.body");
    MIRBlock *mergeBlock = newBlock("while.end");

    builder->createBr(condBlock);

    builder->setInsertPoint(condBlock);
    visit(stmt.getCondition());
    MIRValue *cond = coerceToBool(lastExprValue, stmt.getLoc());
    builder->createCondBr(cond, bodyBlock, mergeBlock);

    builder->setInsertPoint(bodyBlock);
    loopCondBlocks.push(condBlock);
    loopMergeBlocks.push(mergeBlock);

    continueScopeDepths.push(scopeStack.size());
    breakScopeDepths.push(scopeStack.size());

    visit(stmt.getBody());

    continueScopeDepths.pop();
    breakScopeDepths.pop();

    loopCondBlocks.pop();
    loopMergeBlocks.pop();

    if (!getTerminator(builder->getInsertBlock())) {
      builder->createBr(condBlock);
    }

    builder->setInsertPoint(mergeBlock);
    lastExprValue = nullptr;
  }

  void visitDoWhileStmt(const hir::DoWhileStmt &stmt) override {
    MIRBlock *bodyBlock = newBlock("dowhile.body");
    MIRBlock *condBlock = newBlock("dowhile.cond");
    MIRBlock *mergeBlock = newBlock("dowhile.end");

    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();
    MIRValue *trueVal =
        mirModule->getOrInsertConstant<ConstantBool>(true, boolTy);
    builder->createCondBr(trueVal, bodyBlock, condBlock);

    builder->setInsertPoint(bodyBlock);

    loopCondBlocks.push(condBlock);
    loopMergeBlocks.push(mergeBlock);

    continueScopeDepths.push(scopeStack.size());
    breakScopeDepths.push(scopeStack.size());

    visit(stmt.getBody());

    continueScopeDepths.pop();
    breakScopeDepths.pop();

    loopCondBlocks.pop();
    loopMergeBlocks.pop();

    if (!getTerminator(builder->getInsertBlock()))
      builder->createBr(condBlock);

    builder->setInsertPoint(condBlock);
    visit(stmt.getCondition());

    MIRValue *cond = coerceToBool(lastExprValue, stmt.getLoc());
    builder->createCondBr(cond, bodyBlock, mergeBlock);

    builder->setInsertPoint(mergeBlock);
    lastExprValue = nullptr;
  }

  void visitForStmt(const hir::ForStmt &stmt) override {
    auto oldSymbolMap = symbolMap;
    if (stmt.getInit())
      visit(stmt.getInit());

    MIRBlock *condBlock = newBlock("for.cond");
    MIRBlock *bodyBlock = newBlock("for.body");
    MIRBlock *incBlock = newBlock("for.inc");
    MIRBlock *mergeBlock = newBlock("for.end");

    builder->createBr(condBlock);
    builder->setInsertPoint(condBlock);

    if (stmt.getCondition()) {
      visit(stmt.getCondition());
      MIRValue *condVal = coerceToBool(lastExprValue, stmt.getLoc());
      builder->createCondBr(condVal, bodyBlock, mergeBlock);
    } else {
      builder->createBr(bodyBlock);
    }

    builder->setInsertPoint(bodyBlock);
    loopCondBlocks.push(incBlock);
    loopMergeBlocks.push(mergeBlock);

    continueScopeDepths.push(scopeStack.size());
    breakScopeDepths.push(scopeStack.size());

    visit(stmt.getBody());

    continueScopeDepths.pop();
    breakScopeDepths.pop();

    loopCondBlocks.pop();
    loopMergeBlocks.pop();

    if (!getTerminator(builder->getInsertBlock()))
      builder->createBr(incBlock);

    builder->setInsertPoint(incBlock);
    if (stmt.getIncrement())
      visit(stmt.getIncrement());
    builder->createBr(condBlock);

    builder->setInsertPoint(mergeBlock);
    symbolMap = oldSymbolMap;
    lastExprValue = nullptr;
  }

  void visitForInStmt(const hir::ForInStmt &stmt) override {
    auto oldSymbolMap = symbolMap;
    MIRValue *lvalueBase = evaluateAsLValue(stmt.getCollection());
    MIRValue *collection = lvalueBase;

    if (collection && collection->getType() &&
        collection->getType()->getKind() == hir::TypeKind::Pointer) {
      if (auto *pTy =
              llvm::dyn_cast_or_null<hir::PointerType>(collection->getType())) {

        auto kind = pTy->getPointee()->getKind();
        if (kind == hir::TypeKind::Slice || kind == hir::TypeKind::String ||
            kind == hir::TypeKind::Map || kind == hir::TypeKind::Any) {

          collection = builder->insert(std::make_unique<LoadInst>(
              collection, "col.load", stmt.getLoc()));
        }
      }
    }

    bool needsRetain = false;
    const hir::HIRType *colTy =
        collection ? collection->getType() : stmt.getCollection()->getType();
    if (colTy) {
      auto kind = colTy->getKind();
      if (kind == hir::TypeKind::Slice || kind == hir::TypeKind::String ||
          kind == hir::TypeKind::Map || kind == hir::TypeKind::Any) {
        needsRetain = true;
      } else if (auto *ptrTy =
                     llvm::dyn_cast_or_null<hir::PointerType>(colTy)) {
        if (ptrTy->getOwnership() == hir::Ownership::Shared) {
          needsRetain = true;
        }
      }
    }

    MIRValue *pinnedCollection = collection;
    MIRValue *keepAliveAlloca = nullptr;
    if (needsRetain && collection) {
      keepAliveAlloca = builder->createAlloca(collection->getType(),
                                              "forin.keepalive", stmt.getLoc());
      builder->insert(std::make_unique<ARCInst>(Opcode::Retain, collection,
                                                nullptr, stmt.getLoc()));
      builder->insert(std::make_unique<StoreInst>(collection, keepAliveAlloca,
                                                  stmt.getLoc()));
    }

    MIRValue *valLoopVar = nullptr;
    MIRValue *idxLoopVar = nullptr;
    const hir::HIRType *targetVarTy = nullptr;
    const hir::HIRType *targetIdxTy = nullptr;

    if (auto *valDecl = stmt.getVariable()) {
      targetVarTy = valDecl->getType();
      if (colTy && colTy->getKind() == hir::TypeKind::String) {
        targetVarTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
      }
      valLoopVar = builder->createAlloca(targetVarTy, valDecl->getName(),
                                         valDecl->getLoc());
      symbolMap[valDecl->getName()] = valLoopVar;
    }

    if (auto *idxDecl = stmt.getIndexVariable()) {
      targetIdxTy = idxDecl->getType();
      idxLoopVar = builder->createAlloca(targetIdxTy, idxDecl->getName(),
                                         idxDecl->getLoc());
      symbolMap[idxDecl->getName()] = idxLoopVar;
    }

    auto *intType =
        const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    MIRValue *indexAlloca =
        builder->createAlloca(intType, "forin.idx", stmt.getLoc());
    MIRValue *zero = mirModule->getOrInsertConstant<ConstantInt>(0, intType);
    builder->insert(
        std::make_unique<StoreInst>(zero, indexAlloca, stmt.getLoc()));

    MIRBlock *condBlock = newBlock("forin.cond");
    MIRBlock *bodyBlock = newBlock("forin.body");
    MIRBlock *incBlock = newBlock("forin.inc");
    MIRBlock *endBlock = newBlock("forin.end");

    builder->createBr(condBlock);
    builder->setInsertPoint(condBlock);
    MIRValue *currentIndex = builder->insert(
        std::make_unique<LoadInst>(indexAlloca, "idx.load", stmt.getLoc()));

    MIRValue *lengthVal = nullptr;
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(colTy)) {
      colTy = ptrTy->getPointee();
    }

    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
        hir::Ownership::None);

    if (colTy && colTy->getKind() == hir::TypeKind::Slice) {
      ensureBuiltinMIR("moksha_rt_array_length");
      MIRFunction *lenFunc = mirModule->getFunction("moksha_rt_array_length");
      if (!lenFunc) {
        auto fn = std::make_unique<MIRFunction>(
            intType, "moksha_rt_array_length", Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        lenFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      MIRValue *voidCol = builder->createBitCast(pinnedCollection, voidPtrTy,
                                                 "col.cast", stmt.getLoc());
      lengthVal = builder->createCall(lenFunc, {voidCol}, intType, "slice.len",
                                      false, stmt.getLoc());
    } else if (auto *arrTy = llvm::dyn_cast_or_null<hir::ArrayType>(colTy)) {
      lengthVal = mirModule->getOrInsertConstant<ConstantInt>(arrTy->getSize(),
                                                              intType);
    } else if (colTy && colTy->getKind() == hir::TypeKind::String) {
      std::string lenName = "moksha_rt_string_len";
      ensureBuiltinMIR(lenName);
      MIRFunction *strLenFunc = mirModule->getFunction(lenName);
      if (!strLenFunc) {
        auto fn =
            std::make_unique<MIRFunction>(intType, lenName, Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(
            fn.get(), pinnedCollection->getType(), 0));
        strLenFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      lengthVal = builder->createCall(strLenFunc, {pinnedCollection}, intType,
                                      "str.len", false, stmt.getLoc());
    } else if (colTy && colTy->getKind() == hir::TypeKind::Map) {
      std::string mapLenName = "moksha_rt_map_len";
      ensureBuiltinMIR(mapLenName);
      MIRFunction *mapLenFunc = mirModule->getFunction(mapLenName);
      if (!mapLenFunc) {
        auto fn = std::make_unique<MIRFunction>(intType, mapLenName,
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        mapLenFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      MIRValue *castedMap = pinnedCollection;
      if (castedMap->getType() != voidPtrTy) {
        castedMap = builder->createBitCast(castedMap, voidPtrTy, "map.len.cast",
                                           stmt.getLoc());
      }
      lengthVal = builder->createCall(mapLenFunc, {castedMap}, intType,
                                      "map.len", false, stmt.getLoc());
    } else {
      lengthVal = mirModule->getOrInsertConstant<ConstantInt>(0, intType);
    }

    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();
    MIRValue *loopCond =
        builder->createICmp(CompareInst::Predicate::LT, currentIndex, lengthVal,
                            boolTy, "forin.cmp", stmt.getLoc());
    builder->createCondBr(loopCond, bodyBlock, endBlock);
    builder->setInsertPoint(bodyBlock);

    MIRValue *mapKey = nullptr;
    MIRValue *mapVal = nullptr;
    auto *anyTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();

    if (colTy && colTy->getKind() == hir::TypeKind::Map) {
      MIRValue *castedMap = pinnedCollection;
      if (castedMap->getType() != voidPtrTy) {
        castedMap = builder->createBitCast(castedMap, voidPtrTy,
                                           "map.iter.cast", stmt.getLoc());
      }
      const hir::HIRType *abiAnyTy = getABICoercedType(anyTy, true);
      std::string iterKeyName = "moksha_rt_map_get_key_at";
      ensureBuiltinMIR(iterKeyName);
      MIRFunction *iterKeyFunc = mirModule->getFunction(iterKeyName);
      if (!iterKeyFunc) {
        auto fn = std::make_unique<MIRFunction>(abiAnyTy, iterKeyName,
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), intType, 1));
        iterKeyFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      mapKey =
          builder->createCall(iterKeyFunc, {castedMap, currentIndex}, abiAnyTy,
                              "map.key.ptr", false, stmt.getLoc());
      if (abiAnyTy != anyTy) {
        mapKey = builder->createLoad(mapKey, "map.key", stmt.getLoc());
      }

      if (idxLoopVar) {
        std::string iterValName = "moksha_rt_map_get_val_at";
        ensureBuiltinMIR(iterValName);
        MIRFunction *iterValFunc = mirModule->getFunction(iterValName);
        if (!iterValFunc) {
          auto fn = std::make_unique<MIRFunction>(abiAnyTy, iterValName,
                                                  Linkage::External);
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), intType, 1));
          iterValFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }
        mapVal =
            builder->createCall(iterValFunc, {castedMap, currentIndex},
                                abiAnyTy, "map.val.ptr", false, stmt.getLoc());
        if (abiAnyTy != anyTy) {
          mapVal = builder->createLoad(mapVal, "map.val", stmt.getLoc());
        }
      }
    }

    if (collection && valLoopVar) {
      MIRValue *loadedElem = nullptr;
      MIRValue *elemAddr = nullptr;
      const hir::HIRType *trueElemTy = targetVarTy;

      if (colTy && colTy->getKind() == hir::TypeKind::String) {
        std::string charAtName = "moksha_rt_string_char_at";
        ensureBuiltinMIR(charAtName);
        MIRFunction *charAtFunc = mirModule->getFunction(charAtName);
        auto *i8Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
        if (!charAtFunc) {
          auto fn = std::make_unique<MIRFunction>(i8Ty, charAtName,
                                                  Linkage::External);
          fn->addArgument(std::make_unique<MIRArgument>(
              fn.get(), pinnedCollection->getType(), 0));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), intType, 1));
          charAtFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }
        loadedElem =
            builder->createCall(charAtFunc, {pinnedCollection, currentIndex},
                                i8Ty, "str.char", false, stmt.getLoc());

      } else if (colTy && colTy->getKind() == hir::TypeKind::Map) {
        if (idxLoopVar) {
          loadedElem = mapVal;
        } else {
          loadedElem = mapKey;
        }
      } else {
        MIRValue *dataPtr = pinnedCollection;
        if (colTy && colTy->getKind() == hir::TypeKind::Slice) {
          trueElemTy =
              static_cast<const hir::SliceType *>(colTy)->getElementType();
        } else if (colTy && colTy->getKind() == hir::TypeKind::Array) {
          trueElemTy =
              static_cast<const hir::ArrayType *>(colTy)->getElementType();
        }

        auto *elemPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                trueElemTy, hir::Ownership::None);

        if (colTy && colTy->getKind() == hir::TypeKind::Slice) {
          ensureBuiltinMIR("moksha_rt_array_data");
          MIRFunction *dataFunc =
              mirModule->getFunction("moksha_rt_array_data");
          if (!dataFunc) {
            auto *voidTy =
                const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            auto *vpTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    voidTy, hir::Ownership::None);
            auto fn = std::make_unique<MIRFunction>(
                vpTy, "moksha_rt_array_data", Linkage::External);
            fn->addArgument(std::make_unique<MIRArgument>(fn.get(), vpTy, 0));
            dataFunc = fn.get();
            mirModule->addFunction(std::move(fn));
          }
          auto *voidPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
                  hir::Ownership::None);
          MIRValue *voidCol = builder->createBitCast(
              pinnedCollection, voidPtrTy, "col.cast", stmt.getLoc());
          MIRValue *rawData =
              builder->createCall(dataFunc, {voidCol}, voidPtrTy,
                                  "slice.data.raw", false, stmt.getLoc());
          dataPtr = builder->createBitCast(rawData, elemPtrTy,
                                           "slice.ptr.extract", stmt.getLoc());
        } else if (colTy && colTy->getKind() == hir::TypeKind::Array) {
          if (dataPtr->getType() != elemPtrTy) {
            dataPtr = builder->createBitCast(dataPtr, elemPtrTy,
                                             "array.decay.cast", stmt.getLoc());
          }
        }

        MIRValue *gep = nullptr;
        gep = builder->createGEP(dataPtr, {currentIndex}, trueElemTy,
                                 "elem.ptr", stmt.getLoc());

        elemAddr = gep;
        loadedElem = builder->insert(
            std::make_unique<LoadInst>(gep, "elem.val", stmt.getLoc()));
      }

      if (trueElemTy != targetVarTy || loadedElem->getType() != targetVarTy) {
        if (targetVarTy->getKind() == hir::TypeKind::Any) {
          loadedElem =
              boxValue(loadedElem, trueElemTy, targetVarTy, stmt.getLoc());
        } else if (loadedElem->getType()->getKind() == hir::TypeKind::Any) {
          loadedElem = unboxValue(loadedElem, loadedElem->getType(),
                                  targetVarTy, stmt.getLoc());
        } else if (auto *ptrTy =
                       llvm::dyn_cast_or_null<hir::PointerType>(targetVarTy);
                   ptrTy && (ptrTy->getOwnership() == hir::Ownership::Shared ||
                             ptrTy->getOwnership() == hir::Ownership::Owned)) {
          if (loadedElem->getType()->getKind() != hir::TypeKind::Pointer) {
            if (elemAddr) {
              loadedElem = builder->createBitCast(
                  elemAddr, targetVarTy, "elem.alias.cast", stmt.getLoc());
            } else {
              loadedElem =
                  boxValue(loadedElem, trueElemTy, targetVarTy, stmt.getLoc());
            }
          } else {
            loadedElem = builder->createBitCast(loadedElem, targetVarTy,
                                                "elem.cast", stmt.getLoc());
          }
        } else {
          loadedElem = builder->createBitCast(loadedElem, targetVarTy,
                                              "elem.cast", stmt.getLoc());
        }
      }

      builder->insert(
          std::make_unique<StoreInst>(loadedElem, valLoopVar, stmt.getLoc()));
    }

    if (idxLoopVar) {
      MIRValue *targetIndex = currentIndex;
      if (colTy && colTy->getKind() == hir::TypeKind::Map) {
        targetIndex = mapKey;
      }
      if (targetIdxTy != targetIndex->getType()) {
        if (targetIdxTy->getKind() == hir::TypeKind::Any) {
          ensureStringifierForAny(targetIndex->getType());
          targetIndex = builder->insert(std::make_unique<CastInst>(
              Opcode::AnyCast, targetIndex, targetIdxTy, "idx.anycast",
              stmt.getLoc()));
        } else {
          targetIndex = builder->createBitCast(targetIndex, targetIdxTy,
                                               "idx.cast", stmt.getLoc());
        }
      }
      builder->insert(
          std::make_unique<StoreInst>(targetIndex, idxLoopVar, stmt.getLoc()));
    }

    if (stmt.getBody()) {
      loopCondBlocks.push(incBlock);
      loopMergeBlocks.push(endBlock);

      auto oldSymbolMapBody = symbolMap;
      scopeStack.push_back({});

      continueScopeDepths.push(scopeStack.size());
      breakScopeDepths.push(scopeStack.size());

      visit(stmt.getBody());

      continueScopeDepths.pop();
      breakScopeDepths.pop();

      emitScopeCleanup(scopeStack.size() - 1, stmt.getLoc());
      scopeStack.pop_back();
      symbolMap = oldSymbolMapBody;

      loopCondBlocks.pop();
      loopMergeBlocks.pop();
    }

    if (!getTerminator(builder->getInsertBlock())) {
      builder->createBr(incBlock);
    }

    builder->setInsertPoint(incBlock);

    MIRValue *loadedIdx = builder->insert(
        std::make_unique<LoadInst>(indexAlloca, "idx.inc.load", stmt.getLoc()));
    MIRValue *oneVal = mirModule->getOrInsertConstant<ConstantInt>(1, intType);
    MIRValue *nextIdx =
        builder->createAdd(loadedIdx, oneVal, "idx.add", stmt.getLoc());
    builder->insert(
        std::make_unique<StoreInst>(nextIdx, indexAlloca, stmt.getLoc()));

    builder->createBr(condBlock);
    builder->setInsertPoint(endBlock);
    symbolMap = oldSymbolMap;

    if (needsRetain && keepAliveAlloca) {
      MIRValue *loadedKeepAlive = builder->insert(std::make_unique<LoadInst>(
          keepAliveAlloca, "keepalive.load", stmt.getLoc()));

      builder->insert(std::make_unique<ARCInst>(
          Opcode::Release, loadedKeepAlive, nullptr, stmt.getLoc()));
    }
    lastExprValue = nullptr;
  }

  void visitSwitchStmt(const hir::SwitchStmt &stmt) override {
    visit(stmt.getCondition());
    MIRValue *condVal = lastExprValue;

    if (!condVal)
      return;

    if (condVal->getType() &&
        condVal->getType()->getKind() != hir::TypeKind::Int) {
      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      condVal = builder->createBitCast(condVal, i32Ty, "switch.cond.cast",
                                       stmt.getLoc());
    }

    MIRBlock *mergeBlock = newBlock("switch.end");
    MIRBlock *defaultBlock = mergeBlock;

    // Identify the Default case from the cases vector
    const hir::SwitchCase *defaultASTCase = nullptr;
    for (const auto &c : stmt.getCases()) {
      if (c.isDefaultCase()) {
        defaultASTCase = &c;
        break;
      }
    }

    // Create the default block if we found one
    MIRBlock *actualDefaultBlock = nullptr;
    if (defaultASTCase) {
      actualDefaultBlock = newBlock("switch.default");
      defaultBlock = actualDefaultBlock;
    }

    struct RangeValue {
      int64_t start;
      int64_t end;
    };

    struct CaseMapping {
      MIRBlock *block;
      const hir::HIRStmt *body;
      std::vector<MIRValue *> constValues;
      std::vector<RangeValue> rangeValues;
    };
    std::vector<CaseMapping> mappings;

    for (const auto &c : stmt.getCases()) {
      if (c.isDefaultCase())
        continue;

      MIRBlock *caseBlock = newBlock("switch.case");
      CaseMapping mapping;
      mapping.block = caseBlock;
      mapping.body = &c.getBody();

      for (const auto &valExpr : c.getValues()) {
        if (auto *binExpr =
                llvm::dyn_cast_or_null<hir::HIRBinaryExpr>(valExpr.get())) {
          if (binExpr->getOp() == hir::BinaryOp::Range) {
            visit(binExpr->getLHS());
            auto *lhsConst = llvm::dyn_cast_or_null<ConstantInt>(lastExprValue);
            visit(binExpr->getRHS());
            auto *rhsConst = llvm::dyn_cast_or_null<ConstantInt>(lastExprValue);

            if (lhsConst && rhsConst) {
              int64_t start = lhsConst->getValue();
              int64_t end = rhsConst->getValue();
              if (end - start > 64) {
                mapping.rangeValues.push_back({start, end});
              } else {
                for (int64_t v = start; v <= end; ++v) {
                  mapping.constValues.push_back(
                      mirModule->getOrInsertConstant<ConstantInt>(
                          v, condVal->getType()));
                }
              }
            } else {
              diags.report(binExpr->getLoc(), DiagID::err_invalid_type)
                  << "Switch range bounds must be constant integers";
            }
            continue;
          }
        }

        visit(valExpr.get());
        if (lastExprValue) {
          MIRValue *caseVal = lastExprValue;
          if (auto *castInst = llvm::dyn_cast_or_null<CastInst>(caseVal)) {
            caseVal = castInst->getValue();
          }
          if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(caseVal)) {
            if (auto *globalVar =
                    llvm::dyn_cast_or_null<MIRGlobal>(loadInst->getPointer())) {
              std::string variantName = globalVar->getName();

              if (enumVariantValues.find(variantName) ==
                  enumVariantValues.end()) {
                uint64_t currentVal = 0;
                std::string baseN = variantName;
                size_t dotPos = variantName.find('.');
                if (dotPos != std::string::npos)
                  baseN = variantName.substr(0, dotPos);

                for (const auto &pair : enumVariantValues) {
                  if (pair.first.find(baseN + ".") == 0)
                    currentVal++;
                }
                enumVariantValues[variantName] = currentVal;
              }

              caseVal = mirModule->getOrInsertConstant<ConstantInt>(
                  enumVariantValues[variantName], condVal->getType());
            }
          }

          if (auto *cInt = llvm::dyn_cast_or_null<ConstantInt>(caseVal)) {
            mapping.constValues.push_back(
                mirModule->getOrInsertConstant<ConstantInt>(
                    cInt->getValue(), condVal->getType()));
          } else {
            mapping.constValues.push_back(caseVal);
          }
        }
      }
      mappings.push_back(mapping);
    }

    MIRBlock *switchHeaderBlock = newBlock("switch.header");
    MIRBlock *currentBlock = builder->getInsertBlock();
    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();
    for (const auto &m : mappings) {
      for (const auto &range : m.rangeValues) {
        MIRBlock *nextCheckBlock = newBlock("switch.next_range");
        builder->setInsertPoint(currentBlock);
        auto *startConst = mirModule->getOrInsertConstant<ConstantInt>(
            range.start, condVal->getType());
        auto *endConst = mirModule->getOrInsertConstant<ConstantInt>(
            range.end, condVal->getType());
        MIRValue *ge =
            builder->createICmp(CompareInst::Predicate::GE, condVal, startConst,
                                boolTy, "range.ge", stmt.getLoc());
        MIRValue *le =
            builder->createICmp(CompareInst::Predicate::LE, condVal, endConst,
                                boolTy, "range.le", stmt.getLoc());
        MIRValue *inRange = builder->insert(std::make_unique<BinaryInst>(
            Opcode::And, ge, le, "range.and", stmt.getLoc()));

        builder->createCondBr(inRange, m.block, nextCheckBlock);

        currentBlock = nextCheckBlock;
      }
    }

    builder->setInsertPoint(currentBlock);
    builder->createBr(switchHeaderBlock);
    builder->setInsertPoint(switchHeaderBlock);
    auto *switchInst =
        builder->createSwitch(condVal, defaultBlock, stmt.getLoc());
    for (const auto &m : mappings) {
      for (MIRValue *v : m.constValues) {
        builder->addSwitchCase(switchInst, v, m.block);
      }
    }

    loopMergeBlocks.push(mergeBlock);
    breakScopeDepths.push(scopeStack.size());

    for (size_t i = 0; i < mappings.size(); ++i) {
      const auto &m = mappings[i];
      builder->setInsertPoint(m.block);
      bool isEmpty = true;
      if (m.body) {
        if (auto *block = llvm::dyn_cast_or_null<hir::BlockStmt>(m.body)) {
          isEmpty = block->getStatements().empty();
        } else {
          isEmpty = false;
        }
        visit(m.body);
      }

      if (!getTerminator(builder->getInsertBlock())) {
        if (isEmpty) {
          if (i < mappings.size() - 1) {
            builder->createBr(mappings[i + 1].block);
          } else if (actualDefaultBlock) {
            builder->createBr(actualDefaultBlock);
          } else {
            builder->createBr(mergeBlock);
          }
        } else {
          builder->createBr(mergeBlock);
        }
      }
    }

    if (actualDefaultBlock && defaultASTCase) {
      builder->setInsertPoint(actualDefaultBlock);
      visit(&defaultASTCase->getBody());
      if (!getTerminator(builder->getInsertBlock())) {
        builder->createBr(mergeBlock);
      }
    }

    loopMergeBlocks.pop();
    breakScopeDepths.pop();
    builder->setInsertPoint(mergeBlock);

    lastExprValue = nullptr;
  }

  void visitBreakStmt(const hir::BreakStmt &stmt) override {
    if (!loopMergeBlocks.empty() && !breakScopeDepths.empty()) {
      size_t targetDepth = breakScopeDepths.top();
      for (size_t i = scopeStack.size(); i > targetDepth; --i) {
        emitScopeCleanup(i - 1, stmt.getLoc(), true);
      }
      if (!getTerminator(builder->getInsertBlock())) {
        builder->createBr(loopMergeBlocks.top());
      }
    }
  }

  void visitContinueStmt(const hir::ContinueStmt &stmt) override {
    if (!loopCondBlocks.empty() && !continueScopeDepths.empty()) {
      size_t targetDepth = continueScopeDepths.top();
      for (size_t i = scopeStack.size(); i > targetDepth; --i) {
        emitScopeCleanup(i - 1, stmt.getLoc(), true);
      }
      if (!getTerminator(builder->getInsertBlock())) {
        builder->createBr(loopCondBlocks.top());
      }
    }
  }

  void visitDeferStmt(const hir::DeferStmt &stmt) override {
    if (!scopeStack.empty()) {
      scopeStack.back().deferredStmts.push_back(stmt.getDeferredStmt());
    }
  }

  void visitLockStmt(const hir::LockStmt &stmt) override {
    visit(stmt.getMutex());
    MIRValue *mutexPtr = lastExprValue;
    if (mutexPtr &&
        stmt.getMutex()->getValueCategory() == hir::ValueCategory::LValue) {
      if (auto *ptrTy =
              llvm::dyn_cast_or_null<hir::PointerType>(mutexPtr->getType())) {
        if (ptrTy->getPointee()->getKind() == hir::TypeKind::Pointer ||
            ptrTy->getPointee()->getKind() == hir::TypeKind::Reference) {
          mutexPtr = builder->createLoad(mutexPtr, "mutex.load", stmt.getLoc());
        }
      }
    }
    if (!mutexPtr)
      return;
    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);

    if (stmt.isAsyncLock()) {
      auto *promTy =
          const_cast<hir::HIRModule *>(hirModule)->getPromiseType(voidTy);
      std::string retStr = promTy->toString();
      std::replace(retStr.begin(), retStr.end(), '*', 'p');
      std::replace(retStr.begin(), retStr.end(), '<', '_');
      std::replace(retStr.begin(), retStr.end(), '>', '_');
      while (!retStr.empty() && retStr.back() == '_') {
        retStr.pop_back();
      }

      std::string lockMethodName = "AsyncMutex_lock_ret_" + retStr;
      MIRFunction *lockFunc = mirModule->getFunction(lockMethodName);
      if (!lockFunc) {
        auto fn = std::make_unique<MIRFunction>(promTy, lockMethodName,
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        lockFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *castMutex = builder->createBitCast(
          mutexPtr, voidPtrTy, "mutex.lock.cast", stmt.getLoc());
      MIRValue *promiseVal =
          builder->createCall(lockFunc, {castMutex}, promTy, "async.lock.prom",
                              false, stmt.getLoc());
      builder->insert(std::make_unique<AwaitInst>(
          promiseVal, voidTy, "async.lock.await", stmt.getLoc()));
    } else {
      MIRValue *castMutex = builder->createBitCast(
          mutexPtr, voidPtrTy, "mutex.lock.cast", stmt.getLoc());
      MIRFunction *lockFunc = mirModule->getFunction("__moksha_lock");
      if (!lockFunc) {
        auto fn = std::make_unique<MIRFunction>(voidTy, "__moksha_lock",
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        lockFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      builder->createCall(lockFunc, {castMutex}, voidTy, "", false,
                          stmt.getLoc());
    }

    MIRBlock *cleanupBlock = newBlock("lock.cleanup");
    MIRBlock *contBlock = newBlock("lock.cont");
    MIRBlock *oldUnwind = currentUnwindDest;
    currentUnwindDest = cleanupBlock;

    if (!scopeStack.empty()) {
      scopeStack.back().deferredStmts.push_back(&stmt);
    }

    if (stmt.getBody())
      visit(stmt.getBody());

    if (!scopeStack.empty() && !scopeStack.back().deferredStmts.empty()) {
      if (scopeStack.back().deferredStmts.back() == &stmt) {
        scopeStack.back().deferredStmts.pop_back();
      }
    }

    currentUnwindDest = oldUnwind;

    if (!getTerminator(builder->getInsertBlock())) {
      builder->createBr(contBlock);
    }

    builder->setInsertPoint(cleanupBlock);
    if (stmt.isAsyncLock()) {
      std::string unlockMethodName = "AsyncMutex_unlock_ret_void";
      MIRFunction *unlockFunc = mirModule->getFunction(unlockMethodName);
      if (!unlockFunc) {
        auto fn = std::make_unique<MIRFunction>(voidTy, unlockMethodName,
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        unlockFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *unlockCast = mutexPtr;
      if (!unlockFunc->getRawArguments().empty()) {
        const hir::HIRType *expectedTy =
            unlockFunc->getRawArguments()[0]->getType();
        if (mutexPtr->getType() != expectedTy) {
          unlockCast = builder->createBitCast(
              mutexPtr, expectedTy, "mutex.unlock.cast.unwind", stmt.getLoc());
        }
      }
      builder->createCall(unlockFunc, std::vector<MIRValue *>{unlockCast},
                          voidTy, "", false, stmt.getLoc());
    } else {
      MIRFunction *unlockFunc = mirModule->getFunction("__moksha_unlock");
      if (!unlockFunc) {
        auto fn = std::make_unique<MIRFunction>(voidTy, "__moksha_unlock",
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        unlockFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *unlockCast = mutexPtr;
      if (!unlockFunc->getRawArguments().empty()) {
        const hir::HIRType *expectedTy =
            unlockFunc->getRawArguments()[0]->getType();
        if (mutexPtr->getType() != expectedTy) {
          unlockCast = builder->createBitCast(
              mutexPtr, expectedTy, "mutex.unlock.cast.unwind", stmt.getLoc());
        }
      }
      builder->createCall(unlockFunc, std::vector<MIRValue *>{unlockCast},
                          voidTy, "", false, stmt.getLoc());
    }

    if (oldUnwind) {
      builder->createBr(oldUnwind);
    } else {
      MIRValue *lpad = builder->createLoad(getExceptionPayloadGlobal(),
                                           "ex.lock", stmt.getLoc());
      builder->insert(std::make_unique<ResumeInst>(lpad, stmt.getLoc()));
    }

    builder->setInsertPoint(contBlock);
    if (stmt.isAsyncLock()) {
      std::string unlockMethodName = "AsyncMutex_unlock_ret_void";
      MIRFunction *unlockFunc = mirModule->getFunction(unlockMethodName);
      if (!unlockFunc) {
        auto fn = std::make_unique<MIRFunction>(voidTy, unlockMethodName,
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        unlockFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *normalUnlockCast = mutexPtr;
      if (!unlockFunc->getRawArguments().empty()) {
        const hir::HIRType *expectedTy =
            unlockFunc->getRawArguments()[0]->getType();
        if (mutexPtr->getType() != expectedTy) {
          normalUnlockCast = builder->createBitCast(
              mutexPtr, expectedTy, "mutex.unlock.cast", stmt.getLoc());
        }
      }
      builder->createCall(unlockFunc, std::vector<MIRValue *>{normalUnlockCast},
                          voidTy, "", false, stmt.getLoc());
    } else {
      MIRFunction *unlockFunc = mirModule->getFunction("__moksha_unlock");
      if (!unlockFunc) {
        auto fn = std::make_unique<MIRFunction>(voidTy, "__moksha_unlock",
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        unlockFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *normalUnlockCast = mutexPtr;
      if (!unlockFunc->getRawArguments().empty()) {
        const hir::HIRType *expectedTy =
            unlockFunc->getRawArguments()[0]->getType();
        if (mutexPtr->getType() != expectedTy) {
          normalUnlockCast = builder->createBitCast(
              mutexPtr, expectedTy, "mutex.unlock.cast", stmt.getLoc());
        }
      }
      builder->createCall(unlockFunc, std::vector<MIRValue *>{normalUnlockCast},
                          voidTy, "", false, stmt.getLoc());
    }
  }

  void visitExprStmt(const hir::ExprStmt &stmt) override {
    if (stmt.getExpr())
      visit(stmt.getExpr());
  }

  void visitTryCatchStmt(const hir::TryCatchStmt &stmt) override {
    auto oldSymbolMap = symbolMap;

    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);
    auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
    auto *i32Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    scopeStack.push_back({});
    if (stmt.getFinallyBlock()) {
      scopeStack.back().deferredStmts.push_back(stmt.getFinallyBlock());
    }

    MIRBlock *tryBodyBlock = newBlock("try.body");
    MIRBlock *lpadBlock = newBlock("lpad");
    MIRBlock *catchDispatchBlock = newBlock("catch.dispatch");
    MIRBlock *contBlock = newBlock("try.cont");
    MIRBlock *resumeBlock = newBlock("try.resume");

    builder->createBr(tryBodyBlock);

    // Try Body
    builder->setInsertPoint(tryBodyBlock);
    MIRBlock *oldUnwind = currentUnwindDest;
    MIRBlock *oldUnwindBody = currentUnwindBody;
    currentUnwindDest = lpadBlock;
    currentUnwindBody = catchDispatchBlock;

    if (stmt.getTryBlock()) {
      tryScopeDepths.push(scopeStack.size());
      visit(stmt.getTryBlock());
      tryScopeDepths.pop();
    }

    currentUnwindDest = oldUnwind;
    currentUnwindBody = oldUnwindBody;

    if (!getTerminator(builder->getInsertBlock())) {
      builder->createBr(contBlock);
    }

    builder->setInsertPoint(lpadBlock);
    auto *i8Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
    auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        i8Ty, hir::Ownership::None);
    auto *lpadType = const_cast<hir::HIRModule *>(hirModule)->getStructType(
        "eh_result", {i8PtrTy, i32Ty});

    auto *lpadInst =
        builder->createLandingPad(lpadType, "eh.lpad", stmt.getLoc());
    lpadInst->addCatchType(voidPtrTy);
    const auto &catches = stmt.getCatches();
    if (!catches.empty()) {
      lpadInst->addCatchType(voidPtrTy);
    }

    builder->createBr(catchDispatchBlock);

    if (catches.empty()) {
      builder->setInsertPoint(catchDispatchBlock);
      builder->createBr(resumeBlock);
    } else {
      MIRBlock *nextDispatchBlock = catchDispatchBlock;
      for (size_t i = 0; i < catches.size(); ++i) {
        builder->setInsertPoint(nextDispatchBlock);
        const auto &catchClause = catches[i];
        MIRValue *cmp = nullptr;
        if (!catchClause.varType ||
            catchClause.varType->getKind() == hir::TypeKind::Any) {
          cmp = mirModule->getOrInsertConstant<ConstantBool>(true, boolTy);
        } else {
          uint32_t typeId = 19;
          auto coreTy = catchClause.varType;

          if (coreTy->getKind() == hir::TypeKind::Bool)
            typeId = 0;
          else if (coreTy->getKind() == hir::TypeKind::Int) {
            auto *intTy = static_cast<const hir::HIRIntType *>(coreTy);
            if (intTy->isSize())
              typeId = intTy->isSigned() ? 9 : 10;
            else if (intTy->getWidth() == 8)
              typeId = intTy->isSigned() ? 1 : 2;
            else if (intTy->getWidth() == 16)
              typeId = intTy->isSigned() ? 3 : 4;
            else if (intTy->getWidth() == 32)
              typeId = intTy->isSigned() ? 5 : 6;
            else if (intTy->getWidth() == 64)
              typeId = intTy->isSigned() ? 7 : 8;
          } else if (coreTy->getKind() == hir::TypeKind::Float) {
            auto *fltTy = static_cast<const hir::HIRFloatType *>(coreTy);
            if (fltTy->getWidth() == 8)
              typeId = 11;
            else if (fltTy->getWidth() == 16)
              typeId = 12;
            else if (fltTy->getWidth() == 32)
              typeId = 13;
            else if (fltTy->getWidth() == 64)
              typeId = 14;
          } else if (coreTy->getKind() == hir::TypeKind::Decimal)
            typeId = 15;
          else if (coreTy->getKind() == hir::TypeKind::String)
            typeId = 16;
          else if (coreTy->getKind() == hir::TypeKind::Map)
            typeId = 17;
          else if (coreTy->getKind() == hir::TypeKind::Array)
            typeId = 18;

          std::string getTypeName = "__moksha_get_type";
          ensureBuiltinMIR(getTypeName);
          MIRFunction *getTypeFunc = mirModule->getFunction(getTypeName);
          if (!getTypeFunc) {
            auto fn = std::make_unique<MIRFunction>(i32Ty, getTypeName,
                                                    Linkage::External);
            fn->addArgument(
                std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
            getTypeFunc = fn.get();
            mirModule->addFunction(std::move(fn));
          }

          MIRValue *payloadVal = builder->createLoad(
              getExceptionPayloadGlobal(), "ex.payload", catchClause.loc);
          MIRValue *actualTypeID =
              builder->createCall(getTypeFunc, {payloadVal}, i32Ty,
                                  "actual_type", false, catchClause.loc);
          MIRValue *expectedTypeID =
              mirModule->getOrInsertConstant<ConstantInt>(typeId, i32Ty);
          cmp = builder->createICmp(CompareInst::Predicate::EQ, actualTypeID,
                                    expectedTypeID, boolTy, "type.match",
                                    catchClause.loc);
        }

        MIRBlock *thisCatchBody = newBlock("catch.body." + std::to_string(i));
        nextDispatchBlock = (i == catches.size() - 1)
                                ? resumeBlock
                                : newBlock("catch.dispatch.next");

        builder->createCondBr(cmp, thisCatchBody, nextDispatchBlock);
        builder->setInsertPoint(thisCatchBody);

        std::string consumeName = "moksha_rt_consume_exception";
        ensureBuiltinMIR(consumeName);
        MIRFunction *consumeFunc = mirModule->getFunction(consumeName);
        if (!consumeFunc) {
          auto fn = std::make_unique<MIRFunction>(voidPtrTy, consumeName,
                                                  Linkage::External);
          consumeFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }
        MIRValue *consumedEx = builder->createCall(
            consumeFunc, {}, voidPtrTy, "ex.consumed", false, catchClause.loc);

        auto oldSymbolMapCatch = symbolMap;
        scopeStack.push_back({});
        auto *exHiddenAlloca =
            builder->createAlloca(voidPtrTy, "ex.hidden", catchClause.loc);
        builder->insert(std::make_unique<StoreInst>(consumedEx, exHiddenAlloca,
                                                    catchClause.loc));
        scopeStack.back().refCountedVars.push_back(exHiddenAlloca);

        if (!catchClause.varName.empty()) {
          const hir::HIRType *catchType =
              catchClause.varType ? catchClause.varType : voidPtrTy;

          auto *alloca = builder->createAlloca(catchType, catchClause.varName,
                                               catchClause.loc);
          symbolMap[catchClause.varName] = alloca;
          MIRValue *valToStore = consumedEx;

          if (catchType->getKind() == hir::TypeKind::Pointer ||
              catchType->getKind() == hir::TypeKind::Reference) {
            if (consumedEx->getType() != catchType) {
              valToStore = builder->insert(std::make_unique<CastInst>(
                  Opcode::BitCast, consumedEx, catchType, "ex.cast",
                  catchClause.loc));
            }
          } else {
            auto *typedPtrTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    catchType, hir::Ownership::None);
            MIRValue *typedPtr = builder->insert(std::make_unique<CastInst>(
                Opcode::BitCast, consumedEx, typedPtrTy, "ex.typed.ptr",
                catchClause.loc));
            valToStore = builder->insert(std::make_unique<LoadInst>(
                typedPtr, "ex.unboxed", catchClause.loc));
          }

          builder->insert(
              std::make_unique<StoreInst>(valToStore, alloca, catchClause.loc));
        } else {
          auto *alloca =
              builder->createAlloca(voidPtrTy, "dummy.ex", catchClause.loc);
          builder->insert(
              std::make_unique<StoreInst>(consumedEx, alloca, catchClause.loc));
          scopeStack.back().refCountedVars.push_back(alloca);
        }

        if (catchClause.body) {
          visit(catchClause.body.get());
        }

        if (!getTerminator(builder->getInsertBlock())) {
          emitScopeCleanup(scopeStack.size() - 1, catchClause.loc);
          scopeStack.back().refCountedVars.clear();
          builder->createBr(contBlock);
        }
        scopeStack.pop_back();
        symbolMap = oldSymbolMapCatch;
      }
    }

    builder->setInsertPoint(resumeBlock);
    emitScopeCleanup(scopeStack.size() - 1, stmt.getLoc(), true);

    if (oldUnwind) {
      if (!getTerminator(builder->getInsertBlock())) {
        builder->createBr(oldUnwindBody ? oldUnwindBody : oldUnwind);
      }
    } else {
      if (!getTerminator(builder->getInsertBlock())) {
        MIRValue *exPayload = builder->createLoad(
            getExceptionPayloadGlobal(), "ex.payload.resume", stmt.getLoc());
        auto *undefLpad =
            mirModule->getOrInsertConstant<ConstantUndef>(lpadInst->getType());
        MIRValue *insert1 = builder->insert(std::make_unique<InsertValueInst>(
            undefLpad, exPayload, 0, "res.insert1", stmt.getLoc()));
        auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
        MIRValue *insert2 = builder->insert(std::make_unique<InsertValueInst>(
            insert1, zero, 1, "res.insert2", stmt.getLoc()));
        builder->insert(std::make_unique<ResumeInst>(insert2, stmt.getLoc()));
      }
    }

    builder->setInsertPoint(contBlock);
    emitScopeCleanup(scopeStack.size() - 1, stmt.getLoc());
    scopeStack.pop_back();
    symbolMap = oldSymbolMap;
    lastExprValue = nullptr;
  }

  void visitThrowStmt(const hir::HIRThrowStmt &stmt) override {
    visit(stmt.getExpr());
    MIRValue *exVal = lastExprValue;

    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);
    MIRValue *boxedEx = nullptr;

    if (exVal->getType()->getKind() == hir::TypeKind::Pointer ||
        exVal->getType()->getKind() == hir::TypeKind::Reference) {
      boxedEx =
          builder->createBitCast(exVal, voidPtrTy, "throw.cast", stmt.getLoc());
    } else {
      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              exVal->getType(), hir::Ownership::None));
      auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
      auto *sizeGep = builder->createGEP(nullPtr, {one}, exVal->getType(),
                                         "sizeof.gep", stmt.getLoc());
      auto *i64Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
      MIRValue *sizeVal = builder->insert(std::make_unique<CastInst>(
          Opcode::PtrToInt, sizeGep, i64Ty, "sizeof.i64", stmt.getLoc()));

      std::string allocName = "__moksha_alloc";
      ensureBuiltinMIR(allocName);
      MIRFunction *allocFunc = mirModule->getFunction(allocName);
      if (!allocFunc) {
        auto fn = std::make_unique<MIRFunction>(voidPtrTy, allocName,
                                                Linkage::External);
        auto *i64Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 1));
        allocFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      uint32_t typeId = 19;
      auto coreTy = exVal->getType();
      if (coreTy->getKind() == hir::TypeKind::Bool)
        typeId = 0;
      else if (coreTy->getKind() == hir::TypeKind::Int) {
        auto *intTy = static_cast<const hir::HIRIntType *>(coreTy);
        if (intTy->isSize())
          typeId = intTy->isSigned() ? 9 : 10;
        else if (intTy->getWidth() == 8)
          typeId = intTy->isSigned() ? 1 : 2;
        else if (intTy->getWidth() == 16)
          typeId = intTy->isSigned() ? 3 : 4;
        else if (intTy->getWidth() == 32)
          typeId = intTy->isSigned() ? 5 : 6;
        else if (intTy->getWidth() == 64)
          typeId = intTy->isSigned() ? 7 : 8;
      } else if (coreTy->getKind() == hir::TypeKind::Float) {
        auto *fltTy = static_cast<const hir::HIRFloatType *>(coreTy);
        if (fltTy->getWidth() == 8)
          typeId = 11;
        else if (fltTy->getWidth() == 16)
          typeId = 12;
        else if (fltTy->getWidth() == 32)
          typeId = 13;
        else if (fltTy->getWidth() == 64)
          typeId = 14;
      } else if (coreTy->getKind() == hir::TypeKind::Decimal)
        typeId = 15;
      else if (coreTy->getKind() == hir::TypeKind::String)
        typeId = 16;
      else if (coreTy->getKind() == hir::TypeKind::Map)
        typeId = 17;
      else if (coreTy->getKind() == hir::TypeKind::Array)
        typeId = 18;

      MIRValue *typeIdVal =
          mirModule->getOrInsertConstant<ConstantInt>(typeId, i32Ty);
      boxedEx = builder->createCall(allocFunc, {sizeVal, typeIdVal}, voidPtrTy,
                                    "throw.alloc", false, stmt.getLoc());

      auto *typedPtrTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              exVal->getType(), hir::Ownership::None);
      MIRValue *typedBoxPtr = builder->createBitCast(
          boxedEx, typedPtrTy, "throw.typed_ptr", stmt.getLoc());
      builder->insert(
          std::make_unique<StoreInst>(exVal, typedBoxPtr, stmt.getLoc()));
    }

    builder->insert(std::make_unique<StoreInst>(
        boxedEx, getExceptionPayloadGlobal(), stmt.getLoc()));

    size_t targetDepth = tryScopeDepths.empty() ? 0 : tryScopeDepths.top();
    for (size_t i = scopeStack.size(); i > targetDepth; --i) {
      emitScopeCleanup(i - 1, stmt.getLoc(), true);
    }

    if (!getTerminator(builder->getInsertBlock())) {
      builder->createThrow(boxedEx, currentUnwindDest, stmt.getLoc());
    }

    lastExprValue = nullptr;
  }

  void visitAsmExpr(const hir::HIRAsmExpr &expr) override {
    std::vector<MIRValue *> mirOperands;
    std::string flatConstraints = "";
    std::vector<MIRValue *> outputLValues;

    std::vector<std::string> outConstraints;
    std::vector<std::string> inConstraints;
    int outputIndex = 0;

    // Process Standard Outputs (e.g. "=r")
    for (const auto &op : expr.getOutputs()) {
      std::string cons = op.constraint;
      cons.erase(std::remove(cons.begin(), cons.end(), '"'), cons.end());
      outConstraints.push_back(cons);

      MIRValue *lval = evaluateAsLValue(op.expr.get());
      outputLValues.push_back(lval);
      outputIndex++;
    }

    // Process InOuts (e.g. "+r") -> Split into Output (=r) and Tied Input ("0")
    std::vector<std::pair<int, MIRValue *>> tiedInputs;
    for (const auto &op : expr.getInouts()) {
      std::string cons = op.constraint;
      cons.erase(std::remove(cons.begin(), cons.end(), '"'), cons.end());

      if (!cons.empty() && cons[0] == '+') {
        cons[0] = '=';
      } else if (cons.find('=') == std::string::npos) {
        cons = "=" + cons;
      }
      outConstraints.push_back(cons);

      MIRValue *lval = evaluateAsLValue(op.expr.get());
      outputLValues.push_back(lval);

      // Evaluate the input value to pass to the asm block
      visit(op.expr.get());
      if (lastExprValue) {
        tiedInputs.push_back({outputIndex, lastExprValue});
      }
      outputIndex++;
    }

    // Process Tied Inputs generated from InOuts
    for (const auto &tied : tiedInputs) {
      inConstraints.push_back(std::to_string(tied.first));
      mirOperands.push_back(tied.second);
    }

    // Process Standard Inputs (e.g. "r")
    for (const auto &op : expr.getInputs()) {
      std::string cons = op.constraint;
      cons.erase(std::remove(cons.begin(), cons.end(), '"'), cons.end());
      inConstraints.push_back(cons);

      visit(op.expr.get());
      if (lastExprValue) {
        mirOperands.push_back(lastExprValue);
      }
    }

    // Combine Output and Input constraints
    for (const auto &c : outConstraints) {
      if (!flatConstraints.empty())
        flatConstraints += ",";
      flatConstraints += c;
    }
    for (const auto &c : inConstraints) {
      if (!flatConstraints.empty())
        flatConstraints += ",";
      flatConstraints += c;
    }

    // Format and Flatten Clobbers for LLVM (e.g. "rax" -> "~{rax}")
    for (const auto &clobber : expr.getClobbers()) {
      if (!flatConstraints.empty())
        flatConstraints += ",";

      std::string cleanClob = clobber;
      cleanClob.erase(std::remove(cleanClob.begin(), cleanClob.end(), '~'),
                      cleanClob.end());
      cleanClob.erase(std::remove(cleanClob.begin(), cleanClob.end(), '{'),
                      cleanClob.end());
      cleanClob.erase(std::remove(cleanClob.begin(), cleanClob.end(), '}'),
                      cleanClob.end());
      cleanClob.erase(std::remove(cleanClob.begin(), cleanClob.end(), '"'),
                      cleanClob.end());

      flatConstraints += "~{" + cleanClob + "}";
    }

    // Determine the correct return type for the inline asm
    const hir::HIRType *asmRetTy = expr.getType();
    if (!asmRetTy || asmRetTy->getKind() == hir::TypeKind::Void) {
      if (outputLValues.size() == 1) {
        if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(
                outputLValues[0]->getType())) {
          asmRetTy = ptrTy->getPointee();
        }
      } else if (outputLValues.size() > 1) {
        std::vector<const hir::HIRType *> fieldTys;
        for (auto *lval : outputLValues) {
          if (auto *ptrTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(lval->getType())) {
            fieldTys.push_back(ptrTy->getPointee());
          }
        }
        asmRetTy = const_cast<hir::HIRModule *>(hirModule)->getStructType(
            "asm.result", fieldTys);
      }
    }

    MIRValue *asmResult = builder->createInlineAsm(
        expr.getAssemblyStr(), flatConstraints, std::move(mirOperands),
        expr.getIsVolatile(), asmRetTy, expr.getLoc());
    if (asmResult && !outputLValues.empty()) {
      if (outputLValues.size() == 1) {
        builder->insert(std::make_unique<StoreInst>(asmResult, outputLValues[0],
                                                    expr.getLoc()));
      } else {
        for (size_t i = 0; i < outputLValues.size(); ++i) {
          const hir::HIRType *fieldTy = nullptr;
          if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(
                  outputLValues[i]->getType())) {
            fieldTy = ptrTy->getPointee();
          }
          auto ext = builder->insert(std::make_unique<ExtractValueInst>(
              asmResult, i, fieldTy, "asm.ext", expr.getLoc()));
          builder->insert(std::make_unique<StoreInst>(ext, outputLValues[i],
                                                      expr.getLoc()));
        }
      }
    }
    if (asmResult) {
      applyBorrowKind(asmResult, asmRetTy);
    }
    lastExprValue = asmResult;
  }

  void visitUnsafeBlockStmt(const hir::UnsafeBlockStmt &stmt) override {
    visitBlockStmt(stmt);
  }

  void visitVarDeclStmt(const hir::HIRVarDeclStmt &stmt) override {
    const hir::HIRVarDeclStmt *varDecl = &stmt;

    MIRValue *initVal = nullptr;
    if (varDecl->getInit()) {
      expectedLambdaReturnType = varDecl->getType();
      visit(varDecl->getInit());
      expectedLambdaReturnType = nullptr;
      initVal = lastExprValue;
    }

    if (varDecl->getType() &&
        varDecl->getType()->getKind() == hir::TypeKind::Reference) {
      if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(initVal)) {
        initVal = loadInst->getPointer();
        auto &insts = builder->getInsertBlock()->getInstructionsMut();
        if (!insts.empty() && insts.back().get() == loadInst) {
          insts.pop_back();
        }
      }
    }

    MIRValue *valToStore = initVal;
    const hir::HIRType *actualType = resolveType(varDecl->getType());
    const hir::HIRType *rawType = varDecl->getType();

    bool isManagedTarget = false;
    if (rawType && rawType->getKind() == hir::TypeKind::Pointer) {
      auto own = static_cast<const hir::PointerType *>(rawType)->getOwnership();
      if (own == hir::Ownership::Shared || own == hir::Ownership::Owned) {
        isManagedTarget = true;
      }
    }

    if (initVal && initVal->getType() && actualType &&
        actualType->getKind() == hir::TypeKind::Pointer) {
      if (auto *initPtrTy =
              llvm::dyn_cast_or_null<hir::PointerType>(initVal->getType())) {
        if (initPtrTy->getPointee()->getKind() == hir::TypeKind::Array) {
          actualType = initVal->getType();
        }
      }
    }

    if (!actualType) {
      if (initVal && initVal->getType()) {
        actualType = initVal->getType();
      } else {
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        actualType = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            voidTy, hir::Ownership::None);
      }
    }

    if (initVal && (initVal->getType() != actualType || isManagedTarget)) {
      if (auto *cInt = llvm::dyn_cast_or_null<ConstantInt>(initVal);
          cInt && actualType->getKind() == hir::TypeKind::Int) {
        valToStore = mirModule->getOrInsertConstant<ConstantInt>(
            cInt->getValue(), actualType);
      } else if (auto *cFloat = llvm::dyn_cast_or_null<ConstantFloat>(initVal);
                 cFloat && (actualType->getKind() == hir::TypeKind::Float ||
                            actualType->getKind() == hir::TypeKind::Decimal)) {
        valToStore = mirModule->getOrInsertConstant<ConstantFloat>(
            cFloat->getValue(), actualType);
      } else if (llvm::dyn_cast_or_null<ConstantNull>(initVal)) {
        valToStore = mirModule->getOrInsertConstant<ConstantNull>(actualType);
      } else if (actualType->getKind() == hir::TypeKind::Any) {
        valToStore = builder->insert(
            std::make_unique<CastInst>(Opcode::AnyCast, initVal, actualType,
                                       "init.any", varDecl->getLoc()));
        applyBorrowKind(valToStore, actualType);
      } else {
        bool isDestStruct = actualType->getKind() == hir::TypeKind::Struct;
        bool isSrcPtr =
            initVal->getType() &&
            (initVal->getType()->getKind() == hir::TypeKind::Pointer ||
             initVal->getType()->getKind() == hir::TypeKind::Reference);

        if (isSrcPtr && actualType->getKind() != hir::TypeKind::Pointer &&
            actualType->getKind() != hir::TypeKind::Reference &&
            actualType->getKind() != hir::TypeKind::String &&
            actualType->getKind() != hir::TypeKind::Slice &&
            actualType->getKind() != hir::TypeKind::Map) {
          auto *rawRefTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  actualType, isDestStruct ? hir::Ownership::Borrowed
                                           : hir::Ownership::None);
          auto *castPtr = builder->createBitCast(
              initVal, rawRefTy, "init.raw_ptr", varDecl->getLoc());
          valToStore = builder->insert(std::make_unique<LoadInst>(
              castPtr, "init.load", varDecl->getLoc()));
        } else if (actualType->getKind() == hir::TypeKind::Any ||
                   isManagedTarget) {
          valToStore =
              boxValue(initVal, initVal->getType(), rawType, varDecl->getLoc());
          if (valToStore->getType() != actualType) {
            valToStore = builder->createBitCast(
                valToStore, actualType, "init.managed.cast", varDecl->getLoc());
          }
          applyBorrowKind(valToStore, actualType);
        } else if (initVal->getType()->getKind() == hir::TypeKind::Any) {
          valToStore = unboxValue(initVal, initVal->getType(), actualType,
                                  varDecl->getLoc());
          applyBorrowKind(valToStore, actualType);
        } else {
          valToStore = coerceValue(initVal, actualType, varDecl->getLoc());
          applyBorrowKind(valToStore, actualType);
        }
      }
    }

    auto *ptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        rawType, hir::Ownership::None);

    uint64_t explicitAlign = varDecl->getAlignment();
    if (explicitAlign == 0 && rawType) {
      const hir::HIRType *baseAlignTy = rawType;

      // Unwrap Arrays, Pointers, and References to find the core struct
      while (baseAlignTy) {
        if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(baseAlignTy)) {
          baseAlignTy = pTy->getPointee();
        } else if (auto *arrTy =
                       llvm::dyn_cast_or_null<hir::ArrayType>(baseAlignTy)) {
          baseAlignTy = arrTy->getElementType();
        } else if (auto *slcTy =
                       llvm::dyn_cast_or_null<hir::SliceType>(baseAlignTy)) {
          baseAlignTy = slcTy->getElementType();
        } else if (auto *refTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                       baseAlignTy)) {
          baseAlignTy = refTy->getInner();
        } else if (auto *nullTy = llvm::dyn_cast_or_null<hir::HIRNullableType>(
                       baseAlignTy)) {
          baseAlignTy = nullTy->getInner();
        } else {
          break;
        }
      }

      if (baseAlignTy) {
        std::string className = baseAlignTy->toString();
        while (!className.empty() &&
               (className[0] == '*' || className[0] == '&' ||
                className[0] == ' ')) {
          className = className.substr(1);
        }
        if (className.find("struct.") == 0)
          className = className.substr(7);
        if (className.find("class.") == 0)
          className = className.substr(6);
        if (className.find("union.") == 0)
          className = className.substr(6);

        for (const auto *cls : hirModule->getClasses()) {
          if (cls->getName() == className) {
            explicitAlign = cls->getAlignment();
            break;
          }
        }
      }
    }

    auto allocaInst = std::make_unique<AllocaInst>(
        ptrTy, rawType, varDecl->getName(), varDecl->getLoc(), explicitAlign);
    auto *alloca = allocaInst.get();

    MIRBlock *entryBlock = currFunc->getEntryBlock();
    entryBlock->getInstructionsMut().insert(
        entryBlock->getInstructionsMut().begin(), std::move(allocaInst));

    if (stmt.isVolatileVar()) {
      volatileVars.insert(alloca);
    }
    symbolMap[varDecl->getName()] = alloca;

    if (initVal) {
      if (valToStore->getType() != rawType) {
        valToStore = builder->createBitCast(valToStore, rawType, "store.cast",
                                            varDecl->getLoc());
      }

      if (isWeakMemory(rawType)) {
        builder->createStoreWeak(valToStore, alloca, varDecl->getLoc());
      } else {
        builder->insert(
            std::make_unique<StoreInst>(valToStore, alloca, varDecl->getLoc()));
        if (isVolatilePointer(valToStore)) {
          volatileVars.insert(alloca);
        }
      }
    } else {
      MIRValue *nullVal = mirModule->getOrInsertConstant<ConstantNull>(rawType);
      builder->insert(
          std::make_unique<StoreInst>(nullVal, alloca, varDecl->getLoc()));
    }

    if (!scopeStack.empty() && rawType) {
      bool isShared = false;
      bool isOwned = false;

      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(rawType)) {
        if (ptrTy->getOwnership() == hir::Ownership::Shared) {
          isShared = true;
        } else if (ptrTy->getOwnership() == hir::Ownership::Owned ||
                   ptrTy->getOwnership() == hir::Ownership::None) {
          if (ptrTy->getPointee() &&
              ptrTy->getPointee()->getKind() == hir::TypeKind::Struct) {

            if (ptrTy->getOwnership() == hir::Ownership::Owned) {
              isOwned = true;
            } else {
              MIRValue *trace = initVal;
              while (auto *cast = llvm::dyn_cast_or_null<CastInst>(trace)) {
                trace = cast->getValue();
              }
              if (auto *call = llvm::dyn_cast_or_null<CallInst>(trace)) {
                if (call->getCallee() &&
                    call->getCallee()->getName() == "__moksha_alloc") {
                  isOwned = true;
                }
              } else if (auto *invoke =
                             llvm::dyn_cast_or_null<InvokeInst>(trace)) {
                if (invoke->getCallee() &&
                    invoke->getCallee()->getName() == "__moksha_alloc") {
                  isOwned = true;
                }
              }
            }
          }
        }
      } else if (llvm::dyn_cast_or_null<hir::ReferenceType>(rawType)) {
        // References are always borrowed, do nothing.
      } else {
        std::string tyStr = rawType->toString();
        if (tyStr.find("Arc<") != std::string::npos ||
            tyStr.find("shared ") != std::string::npos) {
          isShared = true;
        } else if (tyStr.find("Box<") != std::string::npos ||
                   tyStr.find("owned ") != std::string::npos) {
          isOwned = true;
        } else if (rawType->getKind() == hir::TypeKind::Struct ||
                   rawType->getKind() == hir::TypeKind::Closure ||
                   rawType->getKind() == hir::TypeKind::Any ||
                   rawType->getKind() == hir::TypeKind::String ||
                   rawType->getKind() == hir::TypeKind::Array ||
                   rawType->getKind() == hir::TypeKind::Slice ||
                   rawType->getKind() == hir::TypeKind::Map ||
                   rawType->getKind() == hir::TypeKind::Nullable ||
                   rawType->getKind() == hir::TypeKind::Promise ||
                   tyStr.find("closure") != std::string::npos) {
          isOwned = true;
        }
      }

      if (isShared) {
        scopeStack.back().refCountedVars.push_back(alloca);
      } else if (isOwned) {
        scopeStack.back().ownedVars.push_back(alloca);
      }
    }
  }

  void visitBinaryExpr(const hir::HIRBinaryExpr &expr) override {
    if (expr.getOp() == hir::BinaryOp::Assign) {
      if (auto *idxExpr =
              llvm::dyn_cast_or_null<hir::HIRIndexExpr>(expr.getLHS())) {
        const hir::HIRType *baseTy =
            stripMemoryModifiers(idxExpr->getBase()->getType());
        if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(baseTy)) {
          baseTy = stripMemoryModifiers(ptrTy->getPointee());
        }
        if (baseTy && baseTy->getKind() == hir::TypeKind::Map) {
          auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
          auto *voidPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  voidTy, hir::Ownership::None);
          auto *anyTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();
          auto *anyPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  anyTy, hir::Ownership::None);
          auto *mapTy = static_cast<const hir::HIRMapType *>(baseTy);
          const hir::HIRType *exactKeyTy = mapTy->getKeyType();
          const hir::HIRType *exactValTy = mapTy->getValueType();

          auto prepareAnyPtr =
              [&](MIRValue *val, const hir::HIRType *expectedTy) -> MIRValue * {
            if (expectedTy->getKind() == hir::TypeKind::Int &&
                val->getType() != expectedTy) {
              if (llvm::isa<ConstantInt>(val)) {
                val = mirModule->getOrInsertConstant<ConstantInt>(
                    static_cast<ConstantInt *>(val)->getValue(), expectedTy);
              } else {
                val = builder->createBitCast(val, expectedTy, "map.force.cast",
                                             expr.getLoc());
              }
            }

            if (val->getType()->getKind() != hir::TypeKind::Any) {
              val = boxValue(val, expectedTy, anyTy, expr.getLoc());
            }

            if (val->getType()->getKind() != hir::TypeKind::Pointer) {
              auto *spill =
                  builder->createAlloca(anyTy, "map.any.spill", expr.getLoc());
              builder->insert(
                  std::make_unique<StoreInst>(val, spill, expr.getLoc()));
              val = spill;
            }
            if (val->getType() != anyPtrTy) {
              val = builder->createBitCast(val, anyPtrTy, "map.any.cast",
                                           expr.getLoc());
            }
            return val;
          };

          MIRValue *lvalueBase = evaluateAsLValue(idxExpr->getBase());
          MIRValue *mapBase = lvalueBase;
          if (lvalueBase && lvalueBase->getType() &&
              lvalueBase->getType()->getKind() == hir::TypeKind::Pointer) {
            mapBase =
                builder->createLoad(lvalueBase, "map.base.load", expr.getLoc());
          }
          MIRValue *mapPtr = builder->createBitCast(
              mapBase, voidPtrTy, "map.ptr.cast", expr.getLoc());

          visit(idxExpr->getIndex());
          MIRValue *keyPtr = prepareAnyPtr(lastExprValue, exactKeyTy);

          visit(expr.getRHS());
          MIRValue *rawRhsVal = lastExprValue;
          MIRValue *valPtr = prepareAnyPtr(rawRhsVal, exactValTy);

          std::string insertName = "moksha_rt_map_insert";
          MIRFunction *insertFunc = mirModule->getFunction(insertName);
          if (!insertFunc) {
            auto fn = std::make_unique<MIRFunction>(voidTy, insertName,
                                                    Linkage::External);
            fn->addArgument(
                std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
            fn->addArgument(
                std::make_unique<MIRArgument>(fn.get(), anyPtrTy, 1));
            fn->addArgument(
                std::make_unique<MIRArgument>(fn.get(), anyPtrTy, 2));
            insertFunc = fn.get();
            mirModule->addFunction(std::move(fn));
          }

          builder->createCall(insertFunc, {mapPtr, keyPtr, valPtr}, voidTy, "",
                              false, expr.getLoc());

          lastExprValue = rawRhsVal;
          return;
        }
      }

      if (auto *memExpr =
              llvm::dyn_cast_or_null<hir::HIRMemberExpr>(expr.getLHS())) {
        auto memberInfo = memExpr->getMemberInfo();

        if (memberInfo.isBitfield) {
          visit(expr.getRHS());
          MIRValue *rhsVal = lastExprValue;

          MIRValue *objPtr = nullptr;
          if (auto *ident = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(
                  memExpr->getObject())) {
            std::string name = ident->getName();
            if (symbolMap.count(name))
              objPtr = symbolMap[name];
            else
              objPtr = mirModule->getGlobal(name);
          } else {
            visit(memExpr->getObject());
            if (auto *loadInst =
                    llvm::dyn_cast_or_null<LoadInst>(lastExprValue)) {
              objPtr = loadInst->getPointer();
            } else {
              objPtr = lastExprValue;
            }
          }

          if (auto *load = llvm::dyn_cast_or_null<LoadInst>(objPtr)) {
            objPtr = load->getPointer();
          }

          auto *i32Ty =
              const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
          auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
          auto *idx = mirModule->getOrInsertConstant<ConstantInt>(
              memberInfo.index, i32Ty);

          std::vector<MIRValue *> gepIndices = {zero, idx};

          const hir::HIRType *structTy = nullptr;
          if (auto *pTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(objPtr->getType())) {
            structTy = pTy->getPointee();
          }

          MIRValue *containerPtr = builder->createGEP(
              objPtr, gepIndices, structTy, "bf.gep", expr.getLoc());

          auto *expectedPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  memExpr->getType(), hir::Ownership::None);

          if (containerPtr->getType() != expectedPtrTy) {
            containerPtr = builder->createBitCast(containerPtr, expectedPtrTy,
                                                  "bf.gep.cast", expr.getLoc());
          }

          MIRValue *oldVal =
              builder->createLoad(containerPtr, "bf.load", expr.getLoc());

          uint64_t fieldMask = (1ULL << memberInfo.bitWidth) - 1;
          uint64_t clearMask = ~(fieldMask << memberInfo.bitOffset);

          auto *clearConst = mirModule->getOrInsertConstant<ConstantInt>(
              clearMask, oldVal->getType());
          MIRValue *cleared =
              builder->createAnd(oldVal, clearConst, "bf.clear", expr.getLoc());

          MIRValue *castRhs = rhsVal;
          if (castRhs->getType() != oldVal->getType()) {
            castRhs = builder->insert(std::make_unique<CastInst>(
                Opcode::ZExt, rhsVal, oldVal->getType(), "bf.zext",
                expr.getLoc()));
          }

          auto *fieldMaskConst = mirModule->getOrInsertConstant<ConstantInt>(
              fieldMask, oldVal->getType());
          MIRValue *maskedRhs = builder->createAnd(
              castRhs, fieldMaskConst, "bf.mask.rhs", expr.getLoc());

          auto *shiftConst = mirModule->getOrInsertConstant<ConstantInt>(
              memberInfo.bitOffset, oldVal->getType());
          MIRValue *shifted = builder->createShl(maskedRhs, shiftConst,
                                                 "bf.shl", expr.getLoc());

          MIRValue *combined =
              builder->createOr(cleared, shifted, "bf.set", expr.getLoc());

          builder->insert(std::make_unique<StoreInst>(combined, containerPtr,
                                                      expr.getLoc()));

          lastExprValue = rhsVal;
          return;
        }
      }

      // Normal Assignment Logic
      MIRValue *lhsPtr = nullptr;
      if (auto *ident =
              llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(expr.getLHS())) {
        std::string name = ident->getName();
        if (symbolMap.count(name))
          lhsPtr = symbolMap[name];
        else
          lhsPtr = mirModule->getGlobal(name);
      } else {
        lhsPtr = evaluateAsLValue(expr.getLHS());
      }

      MIRBlock *optMergeBlock = nullptr;
      if (isOptionalLHS(expr.getLHS())) {
        MIRBlock *rhsBlock = newBlock("opt.assign.rhs");
        optMergeBlock = newBlock("opt.assign.end");
        MIRValue *checkPtr = lhsPtr;
        if (auto *gep = llvm::dyn_cast_or_null<GetElementPtrInst>(lhsPtr)) {
          checkPtr = gep->getPointer();
        }

        auto *nullConst =
            mirModule->getOrInsertConstant<ConstantNull>(checkPtr->getType());
        const hir::HIRType *boolTy =
            const_cast<hir::HIRModule *>(hirModule)->getBoolType();
        MIRValue *isNotNull =
            builder->createICmp(CompareInst::Predicate::NE, checkPtr, nullConst,
                                boolTy, "opt.assign.notnull", expr.getLoc());
        builder->createCondBr(isNotNull, rhsBlock, optMergeBlock);

        builder->setInsertPoint(rhsBlock);
      }

      if (lhsPtr && lhsPtr->getType() &&
          lhsPtr->getType()->getKind() == hir::TypeKind::Pointer) {
        if (auto *pTy =
                llvm::dyn_cast_or_null<hir::PointerType>(lhsPtr->getType())) {
          if (pTy->getPointee()->getKind() == hir::TypeKind::Reference) {
            lhsPtr =
                builder->createLoad(lhsPtr, "ref.dest.load", expr.getLoc());
          }
        }
      }

      const hir::HIRType *oldExpected = expectedLambdaReturnType;
      if (lhsPtr && lhsPtr->getType()) {
        if (auto *pTy =
                llvm::dyn_cast_or_null<hir::PointerType>(lhsPtr->getType())) {
          expectedLambdaReturnType = pTy->getPointee();
        }
      }

      MIRValue *savedLhsPtr = lhsPtr;
      lastExprValue = nullptr;

      visit(expr.getRHS());
      expectedLambdaReturnType = oldExpected;

      MIRValue *rhs = lastExprValue;
      lhsPtr = savedLhsPtr;

      if (lhsPtr && rhs) {
        const hir::HIRType *expectedTy = lhsPtr->getType();
        if (auto *ptrTy =
                llvm::dyn_cast_or_null<hir::PointerType>(expectedTy)) {
          expectedTy = ptrTy->getPointee();
        }

        if (auto *castInst = llvm::dyn_cast_or_null<CastInst>(rhs)) {
          if (castInst->getValue()->getType() == expectedTy) {
            rhs = castInst->getValue();
            auto &insts = builder->getInsertBlock()->getInstructionsMut();
            if (!insts.empty() && insts.back().get() == castInst) {
              insts.pop_back();
            }
          }
        }

        if (expectedTy && rhs->getType() && expectedTy != rhs->getType()) {
          if (auto *cInt = llvm::dyn_cast_or_null<ConstantInt>(rhs);
              cInt && expectedTy->getKind() == hir::TypeKind::Int) {
            rhs = mirModule->getOrInsertConstant<ConstantInt>(cInt->getValue(),
                                                              expectedTy);
          } else if (auto *cFloat = llvm::dyn_cast_or_null<ConstantFloat>(rhs);
                     cFloat &&
                     (expectedTy->getKind() == hir::TypeKind::Float ||
                      expectedTy->getKind() == hir::TypeKind::Decimal)) {
            rhs = mirModule->getOrInsertConstant<ConstantFloat>(
                cFloat->getValue(), expectedTy);
          } else if (expectedTy->getKind() == hir::TypeKind::Any ||
                     (expectedTy->getKind() == hir::TypeKind::Pointer &&
                      (static_cast<const hir::PointerType *>(expectedTy)
                               ->getOwnership() == hir::Ownership::Shared ||
                       static_cast<const hir::PointerType *>(expectedTy)
                               ->getOwnership() == hir::Ownership::Owned))) {
            rhs = boxValue(rhs, rhs->getType(), expectedTy, expr.getLoc());
            if (rhs->getType() != expectedTy) {
              rhs = builder->createBitCast(
                  rhs, expectedTy, "assign.managed.cast", expr.getLoc());
            }
            applyBorrowKind(rhs, expectedTy);
          } else if (expr.getRHS()->getType()->getKind() ==
                     hir::TypeKind::Any) {
            rhs = unboxValue(rhs, rhs->getType(), expectedTy, expr.getLoc());
            applyBorrowKind(rhs, expectedTy);
          } else {
            bool isDestStruct = expectedTy->getKind() == hir::TypeKind::Struct;
            bool isSrcPtr =
                rhs->getType()->getKind() == hir::TypeKind::Pointer ||
                rhs->getType()->getKind() == hir::TypeKind::Reference;

            if (isSrcPtr && expectedTy->getKind() != hir::TypeKind::Pointer &&
                expectedTy->getKind() != hir::TypeKind::Reference &&
                expectedTy->getKind() != hir::TypeKind::String &&
                expectedTy->getKind() != hir::TypeKind::Slice &&
                expectedTy->getKind() != hir::TypeKind::Map) {
              auto *rawRefTy =
                  const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                      expectedTy, isDestStruct ? hir::Ownership::Borrowed
                                               : hir::Ownership::None);
              auto *castPtr = builder->createBitCast(
                  rhs, rawRefTy, "assign.raw_ptr", expr.getLoc());
              rhs = builder->insert(std::make_unique<LoadInst>(
                  castPtr, "assign.load", expr.getLoc()));
            } else if (rhs->getType() != expectedTy) {
              rhs = coerceValue(rhs, expectedTy, expr.getLoc());
              applyBorrowKind(rhs, expectedTy);
            }
          }
        }

        if (lhsPtr && lhsPtr->getType()) {
          if (auto *pTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(lhsPtr->getType())) {
            auto *rawTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    pTy->getPointee(), hir::Ownership::None);
            if (lhsPtr->getType() != rawTy) {
              lhsPtr = builder->createBitCast(lhsPtr, rawTy, "store.dest.cast",
                                              expr.getLoc());
            }
          } else if (auto *rTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                         lhsPtr->getType())) {
            auto *rawTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    rTy->getInner(), hir::Ownership::None);
            lhsPtr = builder->createBitCast(lhsPtr, rawTy, "store.dest.cast",
                                            expr.getLoc());
          }
        }

        if (isWeakMemory(expectedTy)) {
          builder->createStoreWeak(rhs, lhsPtr, expr.getLoc());
        } else {
          bool isARC = false;
          const hir::HIRType *checkTy = expectedTy;

          if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(checkTy)) {
            checkTy = ptrTy->getPointee();
            if (ptrTy->getOwnership() == hir::Ownership::Shared ||
                ptrTy->getOwnership() == hir::Ownership::Owned) {
              isARC = true;
            }
          }

          if (auto *nullTy =
                  llvm::dyn_cast_or_null<hir::HIRNullableType>(checkTy)) {
            checkTy = nullTy->getInner();
          }

          if (checkTy) {
            auto k = checkTy->getKind();
            if (k == hir::TypeKind::String || k == hir::TypeKind::Slice ||
                k == hir::TypeKind::Map || k == hir::TypeKind::Any ||
                k == hir::TypeKind::Closure || k == hir::TypeKind::Promise ||
                k == hir::TypeKind::Struct) {
              isARC = true;
            }
          }

          if (isARC && rhs->getType()->getKind() != hir::TypeKind::Null) {
            MIRValue *rawRhs = rhs;
            if (rhs->getType()->getKind() != hir::TypeKind::Closure) {
              while (auto *c = llvm::dyn_cast_or_null<CastInst>(rawRhs)) {
                rawRhs = c->getValue();
              }
            }
            bool isNewAlloc = false;
            if (auto *call = llvm::dyn_cast_or_null<CallInst>(rawRhs)) {
              if (call->getCallee() &&
                  call->getCallee()->getName() == "__moksha_alloc") {
                isNewAlloc = true;
              }
            }

            if (!isNewAlloc) {
              builder->insert(std::make_unique<ARCInst>(
                  Opcode::Retain, rawRhs, nullptr, expr.getLoc()));
            }
          }

          auto *storeInst = builder->insert(
              std::make_unique<StoreInst>(rhs, lhsPtr, expr.getLoc()));

          if (isVolatilePointer(lhsPtr)) {
            storeInst->setVolatile(true);
          }
          if (isVolatilePointer(rhs)) {
            volatileVars.insert(lhsPtr);
          }

          MIRValue *movedAlloca = nullptr;
          MIRValue *tracedRhs = rhs;
          while (auto *cast = llvm::dyn_cast_or_null<CastInst>(tracedRhs)) {
            tracedRhs = cast->getValue();
          }
          if (auto *load = llvm::dyn_cast_or_null<LoadInst>(tracedRhs)) {
            movedAlloca = load->getPointer();
          }

          if (movedAlloca) {
            for (auto &scope : scopeStack) {
              auto &owned = scope.ownedVars;
              owned.erase(std::remove(owned.begin(), owned.end(), movedAlloca),
                          owned.end());
            }
          }
        }
      }
      if (optMergeBlock) {
        builder->createBr(optMergeBlock);
        builder->setInsertPoint(optMergeBlock);
      }
      lastExprValue = rhs;
      return;
    }

    // Handle short-circuiting logic directly
    if (expr.getOp() == hir::BinaryOp::And ||
        expr.getOp() == hir::BinaryOp::Or ||
        expr.getOp() == hir::BinaryOp::NullCoalesce) {

      // HANDLE NULL COALESCING (??)
      if (expr.getOp() == hir::BinaryOp::NullCoalesce) {
        visit(expr.getLHS());
        MIRValue *lhsVal = lastExprValue;

        const hir::HIRType *lhsTy = lhsVal ? lhsVal->getType() : nullptr;
        const hir::HIRType *rhsTy = expr.getRHS()->getType();

        const hir::HIRType *coreTy = expr.getType();
        if (!coreTy || coreTy->getKind() == hir::TypeKind::Void) {
          coreTy = lhsTy ? lhsTy : rhsTy;
        }

        while (coreTy) {
          if (auto *nullTy =
                  llvm::dyn_cast_or_null<hir::HIRNullableType>(coreTy)) {
            coreTy = nullTy->getInner();
          } else if (auto *vTy =
                         llvm::dyn_cast_or_null<hir::HIRViewType>(coreTy)) {
            coreTy = vTy->getInner();
          } else if (auto *mTy =
                         llvm::dyn_cast_or_null<hir::HIRMutType>(coreTy)) {
            coreTy = mTy->getInner();
          } else if (auto *lTy =
                         llvm::dyn_cast_or_null<hir::HIRLockType>(coreTy)) {
            coreTy = lTy->getInner();
          } else {
            break;
          }
        }

        if (!coreTy) {
          if (rhsTy) {
            coreTy = rhsTy;
          } else {
            coreTy =
                const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
          }
        }
        const hir::HIRType *resTy = coreTy;

        MIRBlock *lhsUnboxBlock = newBlock("nullcoal.unbox");
        MIRBlock *rhsBlock = newBlock("nullcoal.rhs");
        MIRBlock *mergeBlock = newBlock("nullcoal.end");

        const hir::HIRType *cmpTy = lhsVal->getType();
        auto *nullConst = mirModule->getOrInsertConstant<ConstantNull>(cmpTy);

        MIRValue *safeNull = nullConst;
        if (safeNull->getType()->toString() != cmpTy->toString()) {
          safeNull = builder->insert(
              std::make_unique<CastInst>(Opcode::BitCast, nullConst, cmpTy,
                                         "null.sync.force", expr.getLoc()));
        }

        const hir::HIRType *boolTy =
            const_cast<hir::HIRModule *>(hirModule)->getBoolType();

        MIRValue *isNotNull =
            builder->createICmp(CompareInst::Predicate::NE, lhsVal, safeNull,
                                boolTy, "notnull", expr.getLoc());

        builder->createCondBr(isNotNull, lhsUnboxBlock, rhsBlock);

        builder->setInsertPoint(lhsUnboxBlock);
        MIRValue *lhsCast = lhsVal;
        if (lhsVal->getType() != resTy) {
          if (lhsVal->getType()->toString() != resTy->toString()) {
            bool isPrimitiveRes = (resTy->getKind() == hir::TypeKind::Int ||
                                   resTy->getKind() == hir::TypeKind::Float ||
                                   resTy->getKind() == hir::TypeKind::Decimal ||
                                   resTy->getKind() == hir::TypeKind::Bool);

            if ((lhsVal->getType()->getKind() == hir::TypeKind::Pointer ||
                 lhsVal->getType()->getKind() == hir::TypeKind::Nullable) &&
                isPrimitiveRes) {

              auto *rawPtrTy =
                  const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                      resTy, hir::Ownership::None);
              auto *ptrCast = builder->createBitCast(
                  lhsVal, rawPtrTy, "unwrap.ptr", expr.getLoc());
              lhsCast = builder->insert(std::make_unique<LoadInst>(
                  ptrCast, "unwrap.load", expr.getLoc()));
            } else {
              lhsCast = builder->createBitCast(lhsVal, resTy, "unwrap.cast",
                                               expr.getLoc());
            }
          }
        }
        MIRBlock *lhsUnboxEndBlock = builder->getInsertBlock();
        builder->createBr(mergeBlock);

        builder->setInsertPoint(rhsBlock);
        visit(expr.getRHS());
        MIRValue *rhsVal = lastExprValue;

        MIRValue *rhsCast = rhsVal;
        if (rhsVal->getType() != resTy) {
          if (rhsVal->getType()->toString() != resTy->toString()) {
            if (llvm::isa<ConstantNull>(rhsVal)) {
              if (resTy->getKind() == hir::TypeKind::Int) {
                rhsCast = mirModule->getOrInsertConstant<ConstantInt>(0, resTy);
              } else if (resTy->getKind() == hir::TypeKind::Float ||
                         resTy->getKind() == hir::TypeKind::Decimal) {
                rhsCast =
                    mirModule->getOrInsertConstant<ConstantFloat>(0.0, resTy);
              } else if (resTy->getKind() == hir::TypeKind::Bool) {
                rhsCast =
                    mirModule->getOrInsertConstant<ConstantBool>(false, resTy);
              } else {
                rhsCast = mirModule->getOrInsertConstant<ConstantNull>(resTy);
              }
            } else {
              bool isPrimitiveRes =
                  (resTy->getKind() == hir::TypeKind::Int ||
                   resTy->getKind() == hir::TypeKind::Float ||
                   resTy->getKind() == hir::TypeKind::Decimal ||
                   resTy->getKind() == hir::TypeKind::Bool);

              if ((rhsVal->getType()->getKind() == hir::TypeKind::Pointer ||
                   rhsVal->getType()->getKind() == hir::TypeKind::Nullable) &&
                  isPrimitiveRes) {

                auto *rawPtrTy =
                    const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                        resTy, hir::Ownership::None);
                auto *ptrCast = builder->createBitCast(
                    rhsVal, rawPtrTy, "unwrap.ptr", expr.getLoc());
                rhsCast = builder->insert(std::make_unique<LoadInst>(
                    ptrCast, "unwrap.load", expr.getLoc()));
              } else {
                rhsCast = builder->createBitCast(rhsVal, resTy, "unwrap.cast",
                                                 expr.getLoc());
              }
            }
          }
        }
        MIRBlock *rhsEndBlock = builder->getInsertBlock();
        builder->createBr(mergeBlock);

        builder->setInsertPoint(mergeBlock);
        auto *phi = builder->createPhi(resTy, "nullcoal.phi", expr.getLoc());
        phi->addIncoming(lhsCast, lhsUnboxEndBlock);
        phi->addIncoming(rhsCast, rhsEndBlock);

        lastExprValue = phi;

        MIRBlock *safeContinuation = newBlock("nullcoal.safe.cont");
        builder->createBr(safeContinuation);
        builder->setInsertPoint(safeContinuation);

        return;
      }

      const hir::HIRType *boolTy =
          const_cast<hir::HIRModule *>(hirModule)->getBoolType();

      if (expr.getOp() == hir::BinaryOp::And) {
        visit(expr.getLHS());
        MIRValue *lhsVal = coerceToBool(lastExprValue, expr.getLoc());

        MIRBlock *lhsBlock = builder->getInsertBlock();
        MIRBlock *rhsBlock = newBlock("land.rhs");
        MIRBlock *mergeBlock = newBlock("land.end");

        builder->createCondBr(lhsVal, rhsBlock, mergeBlock);

        builder->setInsertPoint(rhsBlock);
        visit(expr.getRHS());
        MIRValue *rhsVal = coerceToBool(lastExprValue, expr.getLoc());

        MIRBlock *rhsEndBlock = builder->getInsertBlock();
        builder->createBr(mergeBlock);

        builder->setInsertPoint(mergeBlock);
        auto *phi = builder->createPhi(boolTy, "land.phi", expr.getLoc());
        MIRValue *falseVal =
            mirModule->getOrInsertConstant<ConstantBool>(false, boolTy);
        phi->addIncoming(falseVal, lhsBlock);
        phi->addIncoming(rhsVal, rhsEndBlock);

        lastExprValue = phi;
        return;

      } else if (expr.getOp() == hir::BinaryOp::Or) {
        visit(expr.getLHS());
        MIRValue *lhsVal = coerceToBool(lastExprValue, expr.getLoc());

        MIRBlock *lhsBlock = builder->getInsertBlock();
        MIRBlock *rhsBlock = newBlock("lor.rhs");
        MIRBlock *mergeBlock = newBlock("lor.end");

        builder->createCondBr(lhsVal, mergeBlock, rhsBlock);

        builder->setInsertPoint(rhsBlock);
        visit(expr.getRHS());
        MIRValue *rhsVal = coerceToBool(lastExprValue, expr.getLoc());

        MIRBlock *rhsEndBlock = builder->getInsertBlock();
        builder->createBr(mergeBlock);

        builder->setInsertPoint(mergeBlock);
        auto *phi = builder->createPhi(boolTy, "lor.phi", expr.getLoc());
        MIRValue *trueVal =
            mirModule->getOrInsertConstant<ConstantBool>(true, boolTy);
        phi->addIncoming(trueVal, lhsBlock);
        phi->addIncoming(rhsVal, rhsEndBlock);

        lastExprValue = phi;
        return;
      }
    }

    visit(expr.getLHS());
    MIRValue *lhs = lastExprValue;
    visit(expr.getRHS());
    MIRValue *rhs = lastExprValue;

    if (!lhs || !rhs) {
      lastExprValue = nullptr;
      return;
    }

    // STRING CONCATENATION INTERCEPTOR
    bool lhsIsStr =
        lhs->getType() && lhs->getType()->getKind() == hir::TypeKind::String;
    bool rhsIsStr =
        rhs->getType() && rhs->getType()->getKind() == hir::TypeKind::String;

    if ((lhsIsStr || rhsIsStr) && expr.getOp() == hir::BinaryOp::Add) {
      auto *stringTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();

      MIRValue *strLhs = coerceToString(lhs, expr.getLoc());
      MIRValue *strRhs = coerceToString(rhs, expr.getLoc());

      std::string concatName = "__moksha_string_concat";
      MIRFunction *concatFunc = mirModule->getFunction(concatName);
      if (!concatFunc) {
        auto fn = std::make_unique<MIRFunction>(stringTy, concatName,
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), stringTy, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), stringTy, 1));
        concatFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      lastExprValue =
          builder->createCall(concatFunc, {strLhs, strRhs}, stringTy, "str.add",
                              false, expr.getLoc());
      return;
    }

    // POINTER ARITHMETIC INTERCEPTOR
    bool lhsIsPtr = lhs->getType() &&
                    (lhs->getType()->getKind() == hir::TypeKind::Pointer ||
                     lhs->getType()->getKind() == hir::TypeKind::Reference);
    bool rhsIsPtr = rhs->getType() &&
                    (rhs->getType()->getKind() == hir::TypeKind::Pointer ||
                     rhs->getType()->getKind() == hir::TypeKind::Reference);

    if (lhsIsPtr || rhsIsPtr) {
      if (expr.getOp() == hir::BinaryOp::Add) {
        MIRValue *ptr = lhsIsPtr ? lhs : rhs;
        MIRValue *idx = lhsIsPtr ? rhs : lhs;

        if (idx->getType() && idx->getType()->getKind() != hir::TypeKind::Int) {
        } else {
          const hir::HIRType *pointeeTy = nullptr;
          if (auto *pTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(ptr->getType())) {
            pointeeTy = pTy->getPointee();
          } else if (auto *rTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                         ptr->getType())) {
            pointeeTy = rTy->getInner();
          }
          lastExprValue = builder->createGEP(ptr, {idx}, pointeeTy, "ptr.add",
                                             expr.getLoc());
          return;
        }
      } else if (expr.getOp() == hir::BinaryOp::Sub) {
        if (lhsIsPtr && !rhsIsPtr) {
          // Pointer - Integer
          auto *zero =
              mirModule->getOrInsertConstant<ConstantInt>(0, rhs->getType());
          MIRValue *negIdx =
              builder->createSub(zero, rhs, "neg.idx", expr.getLoc());
          const hir::HIRType *pointeeTy = nullptr;
          if (auto *pTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(lhs->getType()))
            pointeeTy = pTy->getPointee();
          lastExprValue = builder->createGEP(lhs, {negIdx}, pointeeTy,
                                             "ptr.sub", expr.getLoc());
          return;
        } else if (lhsIsPtr && rhsIsPtr) {
          // Pointer - Pointer
          const hir::HIRType *isizeTy =
              const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true,
                                                                  true);
          MIRValue *lhsInt = builder->createBitCast(
              lhs, isizeTy, "ptrtoint.lhs", expr.getLoc());
          MIRValue *rhsInt = builder->createBitCast(
              rhs, isizeTy, "ptrtoint.rhs", expr.getLoc());
          MIRValue *byteDiff =
              builder->createSub(lhsInt, rhsInt, "byte.diff", expr.getLoc());

          const hir::HIRType *pointeeTy = nullptr;
          if (auto *pTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(lhs->getType()))
            pointeeTy = pTy->getPointee();

          if (pointeeTy && pointeeTy->getKind() != hir::TypeKind::Void) {
            auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    pointeeTy, hir::Ownership::None));
            auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, isizeTy);
            auto *gep = builder->createGEP(nullPtr, {one}, pointeeTy,
                                           "sizeof.gep", expr.getLoc());
            MIRValue *sizeVal = builder->createBitCast(
                gep, isizeTy, "sizeof.int", expr.getLoc());
            lastExprValue = builder->createDiv(byteDiff, sizeVal, "ptr.diff",
                                               expr.getLoc());
          } else {
            lastExprValue = byteDiff;
          }
          return;
        }
      }
    }

    // Implicit Dereference for Managed Pointers
    auto autoDeref = [&](MIRValue *val) -> MIRValue * {
      if (!val || !val->getType())
        return val;
      const hir::HIRType *ty = val->getType();

      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(ty)) {
        auto kind = ptrTy->getPointee()->getKind();
        if (ptrTy->getOwnership() != hir::Ownership::None &&
            kind != hir::TypeKind::Struct && kind != hir::TypeKind::Array &&
            kind != hir::TypeKind::Slice) {
          return builder->insert(
              std::make_unique<LoadInst>(val, "auto.deref", expr.getLoc()));
        }
      } else if (auto *refTy = llvm::dyn_cast_or_null<hir::ReferenceType>(ty)) {
        auto kind = refTy->getInner()->getKind();
        if (kind != hir::TypeKind::Struct && kind != hir::TypeKind::Array &&
            kind != hir::TypeKind::Slice) {
          return builder->insert(
              std::make_unique<LoadInst>(val, "auto.deref", expr.getLoc()));
        }
      }
      return val;
    };

    lhs = autoDeref(lhs);
    rhs = autoDeref(rhs);

    // Operator Overloading Interception for Binary Expressions
    if (lhs->getType() && lhs->getType()->getKind() == hir::TypeKind::Struct) {
      std::string opName = "operator";
      switch (expr.getOp()) {
      case hir::BinaryOp::Add:
        opName += "+";
        break;
      case hir::BinaryOp::Sub:
        opName += "-";
        break;
      case hir::BinaryOp::Mul:
        opName += "*";
        break;
      case hir::BinaryOp::Div:
        opName += "/";
        break;
      case hir::BinaryOp::Equal:
        opName += "==";
        break;
      case hir::BinaryOp::NotEqual:
        opName += "!=";
        break;
      case hir::BinaryOp::Less:
        opName += "<";
        break;
      case hir::BinaryOp::LessEqual:
        opName += "<=";
        break;
      case hir::BinaryOp::Greater:
        opName += ">";
        break;
      case hir::BinaryOp::GreaterEqual:
        opName += ">=";
        break;
      default:
        break;
      }

      if (opName != "operator") {
        std::string className = "";
        const hir::HIRType *baseTy = lhs->getType();
        while (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(baseTy)) {
          baseTy = ptrTy->getPointee();
        }
        if (baseTy)
          className = baseTy->toString();

        // Strip smart pointers if any
        if (className.find("Box<") == 0)
          className = className.substr(4, className.length() - 5);
        if (className.find("Arc<") == 0)
          className = className.substr(4, className.length() - 5);

        std::string mangledName =
            mangleName(className + "." + opName, {rhs->getType()});
        MIRFunction *opFunc = mirModule->getFunction(mangledName);

        if (opFunc) {
          MIRValue *thisArg = lhs;
          if (lhs->getType() &&
              lhs->getType()->getKind() != hir::TypeKind::Pointer) {
            thisArg =
                builder->createAlloca(lhs->getType(), "op.this", expr.getLoc());
            builder->createStore(lhs, thisArg, expr.getLoc());
          }

          if (!opFunc->getRawArguments().empty()) {
            const hir::HIRType *expectedThisTy =
                opFunc->getRawArguments()[0]->getType();
            if (thisArg->getType() != expectedThisTy) {
              thisArg = builder->createBitCast(thisArg, expectedThisTy, "",
                                               expr.getLoc());
            }
          }

          lastExprValue =
              builder->createCall(opFunc, {thisArg, rhs}, opFunc->getType(), "",
                                  false, expr.getLoc());
          return;
        }
      }
    }

    auto rescueVoidVal = [&](MIRValue *voidVal,
                             const hir::HIRType *targetTy) -> MIRValue * {
      if (auto *load = llvm::dyn_cast_or_null<LoadInst>(voidVal)) {
        MIRValue *ptr = load->getPointer();
        auto *ptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            targetTy, hir::Ownership::None);
        MIRValue *castPtr = builder->createBitCast(
            ptr, ptrTy, "generic.ptr.cast", expr.getLoc());
        return builder->insert(
            std::make_unique<LoadInst>(castPtr, "generic.val", expr.getLoc()));
      }
      return builder->createBitCast(voidVal, targetTy, "generic.cast",
                                    expr.getLoc());
    };

    if (lhs->getType() && lhs->getType()->getKind() == hir::TypeKind::Void &&
        rhs->getType() && rhs->getType()->getKind() != hir::TypeKind::Void) {
      lhs = rescueVoidVal(lhs, rhs->getType());
    } else if (rhs->getType() &&
               rhs->getType()->getKind() == hir::TypeKind::Void &&
               lhs->getType() &&
               lhs->getType()->getKind() != hir::TypeKind::Void) {
      rhs = rescueVoidVal(rhs, lhs->getType());
    }

    bool lhsIsFloat =
        lhs->getType() && (lhs->getType()->getKind() == hir::TypeKind::Float ||
                           lhs->getType()->getKind() == hir::TypeKind::Decimal);
    bool rhsIsFloat =
        rhs->getType() && (rhs->getType()->getKind() == hir::TypeKind::Float ||
                           rhs->getType()->getKind() == hir::TypeKind::Decimal);

    bool isFloat = lhsIsFloat || rhsIsFloat;

    bool isRelational = expr.getOp() == hir::BinaryOp::Equal ||
                        expr.getOp() == hir::BinaryOp::NotEqual ||
                        expr.getOp() == hir::BinaryOp::Less ||
                        expr.getOp() == hir::BinaryOp::LessEqual ||
                        expr.getOp() == hir::BinaryOp::Greater ||
                        expr.getOp() == hir::BinaryOp::GreaterEqual;

    if (isRelational) {
      bool lhsIsOpt =
          expr.getLHS()->getType() &&
          expr.getLHS()->getType()->getKind() == hir::TypeKind::Nullable;
      bool rhsIsOpt =
          expr.getRHS()->getType() &&
          expr.getRHS()->getType()->getKind() == hir::TypeKind::Nullable;

      bool isNullCheck =
          llvm::isa<ConstantNull>(lhs) || llvm::isa<ConstantNull>(rhs);

      if (!isNullCheck && (lhsIsOpt || rhsIsOpt)) {
        MIRBlock *checkBlock = builder->getInsertBlock();
        MIRBlock *unboxBlock = newBlock("cmp.opt.unbox");
        MIRBlock *mergeBlock = newBlock("cmp.opt.merge");

        const hir::HIRType *boolTy =
            const_cast<hir::HIRModule *>(hirModule)->getBoolType();

        MIRValue *isNotNull =
            mirModule->getOrInsertConstant<ConstantBool>(true, boolTy);

        if (lhsIsOpt) {
          auto *nullConstL =
              mirModule->getOrInsertConstant<ConstantNull>(lhs->getType());
          MIRValue *notNullL =
              builder->createICmp(CompareInst::Predicate::NE, lhs, nullConstL,
                                  boolTy, "cmp.l.notnull", expr.getLoc());
          isNotNull = builder->insert(
              std::make_unique<BinaryInst>(Opcode::And, isNotNull, notNullL,
                                           "cmp.notnull.and1", expr.getLoc()));
        }

        if (rhsIsOpt) {
          auto *nullConstR =
              mirModule->getOrInsertConstant<ConstantNull>(rhs->getType());
          MIRValue *notNullR =
              builder->createICmp(CompareInst::Predicate::NE, rhs, nullConstR,
                                  boolTy, "cmp.r.notnull", expr.getLoc());
          isNotNull = builder->insert(
              std::make_unique<BinaryInst>(Opcode::And, isNotNull, notNullR,
                                           "cmp.notnull.and2", expr.getLoc()));
        }

        builder->createCondBr(isNotNull, unboxBlock, mergeBlock);
        builder->setInsertPoint(unboxBlock);

        auto unwrapOpt = [&](MIRValue *val,
                             const hir::HIRType *ty) -> MIRValue * {
          auto *nullTy = llvm::cast<hir::HIRNullableType>(ty);
          const hir::HIRType *innerTy = nullTy->getInner();
          if (innerTy->getKind() == hir::TypeKind::Int ||
              innerTy->getKind() == hir::TypeKind::Float ||
              innerTy->getKind() == hir::TypeKind::Decimal ||
              innerTy->getKind() == hir::TypeKind::Bool) {
            auto *ptrTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    innerTy, hir::Ownership::None);
            MIRValue *castPtr = builder->createBitCast(
                val, ptrTy, "cmp.unwrap.cast", expr.getLoc());
            return builder->insert(std::make_unique<LoadInst>(
                castPtr, "cmp.unwrap.load", expr.getLoc()));
          } else {
            auto *rawPtrTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    innerTy, hir::Ownership::None);
            return builder->createBitCast(val, rawPtrTy, "cmp.unwrap.cast",
                                          expr.getLoc());
          }
        };

        MIRValue *finalLhs = lhs;
        if (lhsIsOpt) {
          finalLhs = unwrapOpt(lhs, expr.getLHS()->getType());
        }

        MIRValue *finalRhs = rhs;
        if (rhsIsOpt) {
          finalRhs = unwrapOpt(rhs, expr.getRHS()->getType());
        }

        if (finalLhs->getType() != finalRhs->getType()) {
          if (lhsIsOpt && !rhsIsOpt) {
            finalLhs =
                coerceValue(finalLhs, finalRhs->getType(), expr.getLoc());
          } else if (rhsIsOpt && !lhsIsOpt) {
            finalRhs =
                coerceValue(finalRhs, finalLhs->getType(), expr.getLoc());
          } else {
            finalRhs =
                coerceValue(finalRhs, finalLhs->getType(), expr.getLoc());
          }
        }

        MIRValue *primCmp = nullptr;

        if (finalLhs->getType()->getKind() == hir::TypeKind::String) {
          std::string cmpName = "__moksha_string_eq";
          ensureBuiltinMIR(cmpName);
          MIRFunction *cmpFunc = mirModule->getFunction(cmpName);
          auto *voidPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
                  hir::Ownership::None);

          if (!cmpFunc) {
            auto fn = std::make_unique<MIRFunction>(boolTy, cmpName,
                                                    Linkage::External);
            fn->addArgument(
                std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
            fn->addArgument(
                std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
            cmpFunc = fn.get();
            mirModule->addFunction(std::move(fn));
          }

          MIRValue *castLhs = builder->createBitCast(
              finalLhs, voidPtrTy, "cmp.cast.l", expr.getLoc());
          MIRValue *castRhs = builder->createBitCast(
              finalRhs, voidPtrTy, "cmp.cast.r", expr.getLoc());

          primCmp = builder->createCall(cmpFunc, {castLhs, castRhs}, boolTy,
                                        "str.eq", false, expr.getLoc());
          if (expr.getOp() == hir::BinaryOp::NotEqual) {
            auto *trueVal =
                mirModule->getOrInsertConstant<ConstantBool>(true, boolTy);
            primCmp = builder->insert(std::make_unique<BinaryInst>(
                Opcode::Xor, primCmp, trueVal, "str.ne", expr.getLoc()));
          }
        } else if (isFloat) {
          FCmpInst::Predicate fpred;
          switch (expr.getOp()) {
          case hir::BinaryOp::Equal:
            fpred = FCmpInst::Predicate::OEQ;
            break;
          case hir::BinaryOp::NotEqual:
            fpred = FCmpInst::Predicate::UNE;
            break;
          case hir::BinaryOp::Less:
            fpred = FCmpInst::Predicate::OLT;
            break;
          case hir::BinaryOp::LessEqual:
            fpred = FCmpInst::Predicate::OLE;
            break;
          case hir::BinaryOp::Greater:
            fpred = FCmpInst::Predicate::OGT;
            break;
          case hir::BinaryOp::GreaterEqual:
            fpred = FCmpInst::Predicate::OGE;
            break;
          default:
            fpred = FCmpInst::Predicate::OEQ;
          }
          primCmp = builder->createFCmp(fpred, finalLhs, finalRhs, boolTy,
                                        "cmp.prim", expr.getLoc());
        } else {
          CompareInst::Predicate ipred;
          switch (expr.getOp()) {
          case hir::BinaryOp::Equal:
            ipred = CompareInst::Predicate::EQ;
            break;
          case hir::BinaryOp::NotEqual:
            ipred = CompareInst::Predicate::NE;
            break;
          case hir::BinaryOp::Less:
            ipred = CompareInst::Predicate::LT;
            break;
          case hir::BinaryOp::LessEqual:
            ipred = CompareInst::Predicate::LE;
            break;
          case hir::BinaryOp::Greater:
            ipred = CompareInst::Predicate::GT;
            break;
          case hir::BinaryOp::GreaterEqual:
            ipred = CompareInst::Predicate::GE;
            break;
          default:
            ipred = CompareInst::Predicate::EQ;
          }
          primCmp = builder->createICmp(ipred, finalLhs, finalRhs, boolTy,
                                        "cmp.prim", expr.getLoc());
        }

        MIRBlock *unboxEnd = builder->getInsertBlock();
        builder->createBr(mergeBlock);

        builder->setInsertPoint(mergeBlock);
        auto *phi = builder->createPhi(boolTy, "cmp.opt.phi", expr.getLoc());

        bool isNullResult = (expr.getOp() == hir::BinaryOp::NotEqual);
        auto *nullRes =
            mirModule->getOrInsertConstant<ConstantBool>(isNullResult, boolTy);
        phi->addIncoming(nullRes, checkBlock);
        phi->addIncoming(primCmp, unboxEnd);

        lastExprValue = phi;
        return;
      }
    }

    // UNIVERSAL COERCION BLOCK FOR BINARY OPERANDS
    if (lhs->getType() != rhs->getType()) {
      if (llvm::dyn_cast_or_null<ConstantNull>(rhs) && lhs->getType()) {
        rhs = mirModule->getOrInsertConstant<ConstantNull>(lhs->getType());
      } else if (llvm::dyn_cast_or_null<ConstantNull>(lhs) && rhs->getType()) {
        lhs = mirModule->getOrInsertConstant<ConstantNull>(rhs->getType());
      } else if (lhs->getType() && rhs->getType()) {
        bool lhsIsAny = lhs->getType()->getKind() == hir::TypeKind::Any;
        bool rhsIsAny = rhs->getType()->getKind() == hir::TypeKind::Any;

        if (lhsIsAny && !rhsIsAny) {
          lhs = unboxValue(lhs, lhs->getType(), rhs->getType(), expr.getLoc());
        } else if (rhsIsAny && !lhsIsAny) {
          rhs = unboxValue(rhs, rhs->getType(), lhs->getType(), expr.getLoc());
        }

        bool lhsIsFloat = lhs->getType() &&
                          (lhs->getType()->getKind() == hir::TypeKind::Float ||
                           lhs->getType()->getKind() == hir::TypeKind::Decimal);
        bool rhsIsFloat = rhs->getType() &&
                          (rhs->getType()->getKind() == hir::TypeKind::Float ||
                           rhs->getType()->getKind() == hir::TypeKind::Decimal);
        isFloat = lhsIsFloat || rhsIsFloat;

        if (rhsIsFloat && !lhsIsFloat) {
          lhs = builder->createIntToFloat(lhs, rhs->getType(), "prom.lhs",
                                          expr.getLoc());
        } else if (lhsIsFloat && !rhsIsFloat) {
          rhs = builder->createIntToFloat(rhs, lhs->getType(), "prom.rhs",
                                          expr.getLoc());
        }

        if (lhs->getType() != rhs->getType()) {
          if (auto *cIntRhs = llvm::dyn_cast_or_null<ConstantInt>(rhs);
              cIntRhs && lhs->getType()->getKind() == hir::TypeKind::Int) {
            rhs = mirModule->getOrInsertConstant<ConstantInt>(
                cIntRhs->getValue(), lhs->getType());
          } else if (auto *cIntLhs = llvm::dyn_cast_or_null<ConstantInt>(lhs);
                     cIntLhs &&
                     rhs->getType()->getKind() == hir::TypeKind::Int) {
            lhs = mirModule->getOrInsertConstant<ConstantInt>(
                cIntLhs->getValue(), rhs->getType());
          } else if (auto *cFloatRhs =
                         llvm::dyn_cast_or_null<ConstantFloat>(rhs);
                     cFloatRhs &&
                     (lhs->getType()->getKind() == hir::TypeKind::Float ||
                      lhs->getType()->getKind() == hir::TypeKind::Decimal)) {
            rhs = mirModule->getOrInsertConstant<ConstantFloat>(
                cFloatRhs->getValue(), lhs->getType());
          } else if (auto *cFloatLhs =
                         llvm::dyn_cast_or_null<ConstantFloat>(lhs);
                     cFloatLhs &&
                     (rhs->getType()->getKind() == hir::TypeKind::Float ||
                      rhs->getType()->getKind() == hir::TypeKind::Decimal)) {
            lhs = mirModule->getOrInsertConstant<ConstantFloat>(
                cFloatLhs->getValue(), rhs->getType());
          } else {
            if (lhs->getType()->getKind() == hir::TypeKind::Any) {
              rhs =
                  boxValue(rhs, rhs->getType(), lhs->getType(), expr.getLoc());
            } else {
              rhs = builder->createBitCast(rhs, lhs->getType(), "bin.cast",
                                           expr.getLoc());
            }
          }
        }
      }
    }

    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();

    switch (expr.getOp()) {
    // Math Operators (Int vs Float Separation)
    case hir::BinaryOp::Add:
      if (isFloat)
        lastExprValue = builder->insert(std::make_unique<BinaryInst>(
            Opcode::FAdd, lhs, rhs, "", expr.getLoc()));
      else
        lastExprValue = builder->createAdd(lhs, rhs, "", expr.getLoc());
      break;
    case hir::BinaryOp::Sub:
      if (isFloat)
        lastExprValue = builder->insert(std::make_unique<BinaryInst>(
            Opcode::FSub, lhs, rhs, "", expr.getLoc()));
      else
        lastExprValue = builder->createSub(lhs, rhs, "", expr.getLoc());
      break;
    case hir::BinaryOp::Mul:
      if (isFloat)
        lastExprValue = builder->insert(std::make_unique<BinaryInst>(
            Opcode::FMul, lhs, rhs, "", expr.getLoc()));
      else
        lastExprValue = builder->createMul(lhs, rhs, "", expr.getLoc());
      break;
    case hir::BinaryOp::Div:
      if (isFloat)
        lastExprValue = builder->insert(std::make_unique<BinaryInst>(
            Opcode::FDiv, lhs, rhs, "", expr.getLoc()));
      else
        lastExprValue = builder->createDiv(lhs, rhs, "", expr.getLoc());
      break;
    case hir::BinaryOp::Mod:
      lastExprValue = builder->createMod(lhs, rhs, "", expr.getLoc());
      break;

    // Bitwise Operators
    case hir::BinaryOp::BitAnd:
      lastExprValue = builder->insert(std::make_unique<BinaryInst>(
          Opcode::And, lhs, rhs, "", expr.getLoc()));
      break;
    case hir::BinaryOp::BitOr:
      lastExprValue = builder->insert(std::make_unique<BinaryInst>(
          Opcode::Or, lhs, rhs, "", expr.getLoc()));
      break;
    case hir::BinaryOp::BitXor:
      lastExprValue = builder->insert(std::make_unique<BinaryInst>(
          Opcode::Xor, lhs, rhs, "", expr.getLoc()));
      break;
    case hir::BinaryOp::Shl:
      lastExprValue = builder->insert(std::make_unique<BinaryInst>(
          Opcode::Shl, lhs, rhs, "", expr.getLoc()));
      break;
    case hir::BinaryOp::Shr:
      lastExprValue = builder->insert(std::make_unique<BinaryInst>(
          Opcode::Shr, lhs, rhs, "", expr.getLoc()));
      break;
    case hir::BinaryOp::Pow: {
      if (isFloat) {
        // Determine precision width
        bool isF64 = false;
        if (auto *fTy =
                llvm::dyn_cast_or_null<hir::HIRFloatType>(lhs->getType())) {
          if (fTy->getWidth() == 64)
            isF64 = true;
        } else if (lhs->getType()->getKind() == hir::TypeKind::Decimal) {
          isF64 = true;
        }

        std::string powName = isF64 ? "llvm.pow.f64" : "llvm.pow.f32";
        MIRFunction *powFunc = mirModule->getFunction(powName);

        if (!powFunc) {
          auto *fTyObj =
              isF64 ? const_cast<hir::HIRModule *>(hirModule)->getFloatType(64)
                    : const_cast<hir::HIRModule *>(hirModule)->getFloatType(32);
          auto fn =
              std::make_unique<MIRFunction>(fTyObj, powName, Linkage::External);
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), fTyObj, 0));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), fTyObj, 1));
          powFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        lastExprValue =
            builder->createCall(powFunc, {lhs, rhs}, powFunc->getType(),
                                "pow.call", false, expr.getLoc());
      } else {
        // Fallback for standard integers
        lastExprValue = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Pow, lhs, rhs, "", expr.getLoc()));
      }
      break;
    }

    // Comparison Operators (ICmp vs FCmp Separation)
    case hir::BinaryOp::Equal:
    case hir::BinaryOp::NotEqual: {
      bool isEq = (expr.getOp() == hir::BinaryOp::Equal);

      const hir::HIRType *astTy = expr.getLHS()->getType();
      const hir::HIRType *coreTy = astTy;
      while (coreTy) {
        if (auto *p = llvm::dyn_cast_or_null<hir::PointerType>(coreTy))
          coreTy = p->getPointee();
        else if (auto *r = llvm::dyn_cast_or_null<hir::ReferenceType>(coreTy))
          coreTy = r->getInner();
        else if (auto *n = llvm::dyn_cast_or_null<hir::HIRNullableType>(coreTy))
          coreTy = n->getInner();
        else
          break;
      }

      bool isString = false;
      bool isCollection = false;

      if (coreTy) {
        if (coreTy->getKind() == hir::TypeKind::String)
          isString = true;
        else if (coreTy->getKind() == hir::TypeKind::Array ||
                 coreTy->getKind() == hir::TypeKind::Slice)
          isCollection = true;
      }

      if (!isString && !isCollection && lhs->getType()) {
        if (lhs->getType()->getKind() == hir::TypeKind::String)
          isString = true;
        if (auto *ptrTy =
                llvm::dyn_cast_or_null<hir::PointerType>(lhs->getType())) {
          if (auto *intTy = llvm::dyn_cast_or_null<hir::HIRIntType>(
                  ptrTy->getPointee())) {
            if (intTy->getWidth() == 8)
              isString = true;
          }
        }
      }

      if (isString) {
        std::string cmpName = "__moksha_string_eq";
        ensureBuiltinMIR(cmpName);
        MIRFunction *cmpFunc = mirModule->getFunction(cmpName);
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
                hir::Ownership::None);

        if (!cmpFunc) {
          auto fn =
              std::make_unique<MIRFunction>(boolTy, cmpName, Linkage::External);
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
          cmpFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        MIRValue *castLhs = lhs;
        if (lhs->getType() != voidPtrTy)
          castLhs = builder->createBitCast(lhs, voidPtrTy, "cmp.cast.l",
                                           expr.getLoc());

        MIRValue *castRhs = rhs;
        if (rhs->getType() != voidPtrTy)
          castRhs = builder->createBitCast(rhs, voidPtrTy, "cmp.cast.r",
                                           expr.getLoc());

        MIRValue *res = builder->createCall(cmpFunc, {castLhs, castRhs}, boolTy,
                                            "str.eq", false, expr.getLoc());

        if (!isEq) {
          auto *trueVal =
              mirModule->getOrInsertConstant<ConstantBool>(true, boolTy);
          res = builder->insert(std::make_unique<BinaryInst>(
              Opcode::Xor, res, trueVal, "str.ne", expr.getLoc()));
        }
        lastExprValue = res;
      } else if (isCollection) {
        std::string cmpName = "__moksha_array_eq";
        ensureBuiltinMIR(cmpName);
        MIRFunction *cmpFunc = mirModule->getFunction(cmpName);
        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
                hir::Ownership::None);

        if (!cmpFunc) {
          auto fn =
              std::make_unique<MIRFunction>(boolTy, cmpName, Linkage::External);
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 1));
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 2));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 3));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 4));
          cmpFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        auto extractData =
            [&](MIRValue *val,
                const hir::HIRType *ty) -> std::pair<MIRValue *, MIRValue *> {
          const hir::HIRType *uTy = ty;
          while (uTy) {
            if (auto *p = llvm::dyn_cast_or_null<hir::PointerType>(uTy))
              uTy = p->getPointee();
            else if (auto *r = llvm::dyn_cast_or_null<hir::ReferenceType>(uTy))
              uTy = r->getInner();
            else
              break;
          }
          if (uTy->getKind() == hir::TypeKind::Slice) {
            ensureBuiltinMIR("moksha_rt_array_data");
            ensureBuiltinMIR("moksha_rt_array_length");
            MIRFunction *dataFunc =
                mirModule->getFunction("moksha_rt_array_data");
            MIRFunction *lenFunc =
                mirModule->getFunction("moksha_rt_array_length");
            if (!dataFunc) {
              auto fn = std::make_unique<MIRFunction>(
                  voidPtrTy, "moksha_rt_array_data", Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              dataFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }
            if (!lenFunc) {
              auto fn = std::make_unique<MIRFunction>(
                  i32Ty, "moksha_rt_array_length", Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              lenFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }
            MIRValue *voidVal = builder->createBitCast(
                val, voidPtrTy, "slice.void", expr.getLoc());
            MIRValue *ptr =
                builder->createCall(dataFunc, {voidVal}, voidPtrTy,
                                    "slice.ptr.ext", false, expr.getLoc());
            MIRValue *len = builder->createCall(
                lenFunc, {voidVal}, i32Ty, "slice.len", false, expr.getLoc());
            return {ptr, len};
          } else {
            auto *arrTy = static_cast<const hir::ArrayType *>(uTy);
            MIRValue *ptr = val;
            if (ptr->getType() != voidPtrTy)
              ptr = builder->createBitCast(ptr, voidPtrTy, "arr.ptr.cast",
                                           expr.getLoc());
            MIRValue *len = mirModule->getOrInsertConstant<ConstantInt>(
                arrTy->getSize(), i32Ty);
            return {ptr, len};
          }
        };

        auto p1 = extractData(lhs, expr.getLHS()->getType());
        auto p2 = extractData(rhs, expr.getRHS()->getType());

        const hir::HIRType *elemTy =
            (coreTy->getKind() == hir::TypeKind::Slice)
                ? static_cast<const hir::SliceType *>(coreTy)->getElementType()
                : static_cast<const hir::ArrayType *>(coreTy)->getElementType();

        auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                elemTy, hir::Ownership::None));
        auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
        auto *sizeGep = builder->createGEP(nullPtr, {one}, elemTy, "sizeof.gep",
                                           expr.getLoc());
        MIRValue *elemSize =
            builder->createBitCast(sizeGep, i32Ty, "sizeof.int", expr.getLoc());

        MIRValue *res = builder->createCall(
            cmpFunc, {p1.first, p1.second, p2.first, p2.second, elemSize},
            boolTy, "arr.eq", false, expr.getLoc());

        if (!isEq) {
          auto *trueVal =
              mirModule->getOrInsertConstant<ConstantBool>(true, boolTy);
          res = builder->insert(std::make_unique<BinaryInst>(
              Opcode::Xor, res, trueVal, "arr.ne", expr.getLoc()));
        }
        lastExprValue = res;
      } else {
        if (isFloat) {
          if (isEq) {
            lastExprValue = builder->createFCmp(FCmpInst::Predicate::OEQ, lhs,
                                                rhs, boolTy, "", expr.getLoc());
          } else {
            MIRValue *eq =
                builder->createFCmp(FCmpInst::Predicate::OEQ, lhs, rhs, boolTy,
                                    "eq.tmp", expr.getLoc());
            auto *allOnes =
                mirModule->getOrInsertConstant<ConstantBool>(true, boolTy);
            lastExprValue = builder->insert(std::make_unique<BinaryInst>(
                Opcode::Xor, eq, allOnes, "neq.tmp", expr.getLoc()));
          }
        } else {
          CompareInst::Predicate pred =
              isEq ? CompareInst::Predicate::EQ : CompareInst::Predicate::NE;
          lastExprValue =
              builder->createICmp(pred, lhs, rhs, boolTy,
                                  isEq ? "cmp.eq" : "cmp.ne", expr.getLoc());
        }
      }
      break;
    }
    case hir::BinaryOp::Less:
      if (isFloat)
        lastExprValue = builder->createFCmp(FCmpInst::Predicate::OLT, lhs, rhs,
                                            boolTy, "", expr.getLoc());
      else
        lastExprValue = builder->createICmp(CompareInst::Predicate::LT, lhs,
                                            rhs, boolTy, "", expr.getLoc());
      break;
    case hir::BinaryOp::LessEqual:
      if (isFloat)
        lastExprValue = builder->createFCmp(FCmpInst::Predicate::OLE, lhs, rhs,
                                            boolTy, "", expr.getLoc());
      else
        lastExprValue = builder->createICmp(CompareInst::Predicate::LE, lhs,
                                            rhs, boolTy, "", expr.getLoc());
      break;
    case hir::BinaryOp::Greater:
      if (isFloat)
        lastExprValue = builder->createFCmp(FCmpInst::Predicate::OGT, lhs, rhs,
                                            boolTy, "", expr.getLoc());
      else
        lastExprValue = builder->createICmp(CompareInst::Predicate::GT, lhs,
                                            rhs, boolTy, "", expr.getLoc());
      break;
    case hir::BinaryOp::GreaterEqual:
      if (isFloat)
        lastExprValue = builder->createFCmp(FCmpInst::Predicate::OGE, lhs, rhs,
                                            boolTy, "", expr.getLoc());
      else
        lastExprValue = builder->createICmp(CompareInst::Predicate::GE, lhs,
                                            rhs, boolTy, "", expr.getLoc());
      break;
    default:
      lastExprValue = builder->createAdd(lhs, rhs, "", expr.getLoc());
      break;
    }
  }

  void visitUnaryExpr(const hir::HIRUnaryExpr &expr) override {
    auto op = expr.getOp();
    bool isIncDec =
        (op == hir::UnaryOp::PreInc || op == hir::UnaryOp::PostInc ||
         op == hir::UnaryOp::PreDec || op == hir::UnaryOp::PostDec);

    // 1. Handle Mutations (++, --)
    if (isIncDec) {
      MIRValue *ptr = nullptr;
      if (auto *ident = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(
              expr.getOperand())) {
        std::string name = ident->getName();
        if (symbolMap.count(name))
          ptr = symbolMap[name];
        else
          ptr = mirModule->getGlobal(name);
      } else {
        visit(expr.getOperand());
        if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(lastExprValue)) {
          ptr = loadInst->getPointer();
        } else {
          ptr = lastExprValue;
        }
      }

      if (!ptr)
        return;

      MIRValue *loaded = builder->insert(
          std::make_unique<LoadInst>(ptr, "incdec.load", expr.getLoc()));

      const hir::HIRType *ty = loaded->getType();
      MIRValue *one = nullptr;
      if (ty && (ty->getKind() == hir::TypeKind::Float ||
                 ty->getKind() == hir::TypeKind::Decimal)) {
        one = mirModule->getOrInsertConstant<ConstantFloat>(1.0, ty);
      } else {
        one = mirModule->getOrInsertConstant<ConstantInt>(1, ty);
      }

      MIRValue *newVal = nullptr;
      if (op == hir::UnaryOp::PreInc || op == hir::UnaryOp::PostInc) {
        newVal = builder->createAdd(loaded, one, "inc", expr.getLoc());
      } else {
        newVal = builder->createSub(loaded, one, "dec", expr.getLoc());
      }

      if (ptr && ptr->getType()) {
        if (auto *pTy =
                llvm::dyn_cast_or_null<hir::PointerType>(ptr->getType())) {
          if (pTy->getOwnership() != hir::Ownership::None) {
            auto *rawTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    pTy->getPointee(), hir::Ownership::None);
            ptr = builder->createBitCast(ptr, rawTy, "store.dest.cast",
                                         expr.getLoc());
          }
        } else if (auto *rTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                       ptr->getType())) {
          if (rTy->getOwnership() != hir::Ownership::None) {
            auto *rawTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    rTy->getInner(), hir::Ownership::None);
            ptr = builder->createBitCast(ptr, rawTy, "store.dest.cast",
                                         expr.getLoc());
          }
        }
      }

      builder->insert(std::make_unique<StoreInst>(newVal, ptr, expr.getLoc()));
      if (op == hir::UnaryOp::PostInc || op == hir::UnaryOp::PostDec) {
        lastExprValue = loaded;
      } else {
        lastExprValue = newVal;
      }
      return;
    }

    // 2. Handle Standard Unary Operators (!, ~, -)
    visit(expr.getOperand());
    MIRValue *val = lastExprValue;
    if (!val)
      return;

    // Operator Overloading Interception for Unary Expressions
    if (val->getType() && val->getType()->getKind() == hir::TypeKind::Struct) {
      std::string opName = "operator";
      switch (op) {
      case hir::UnaryOp::Neg:
        opName += "-";
        break;
      case hir::UnaryOp::Not:
        opName += "!";
        break;
      case hir::UnaryOp::BitNot:
        opName += "~";
        break;
      case hir::UnaryOp::PreInc:
      case hir::UnaryOp::PostInc:
        opName += "++";
        break;
      case hir::UnaryOp::PreDec:
      case hir::UnaryOp::PostDec:
        opName += "--";
        break;
      default:
        break;
      }

      if (opName != "operator") {
        std::string className = "";
        const hir::HIRType *baseTy = val->getType();
        while (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(baseTy)) {
          baseTy = ptrTy->getPointee();
        }
        if (baseTy)
          className = baseTy->toString();

        // Strip smart pointers if any
        if (className.find("Box<") == 0)
          className = className.substr(4, className.length() - 5);
        if (className.find("Arc<") == 0)
          className = className.substr(4, className.length() - 5);

        std::string mangledName = mangleName(className + "." + opName, {});
        MIRFunction *opFunc = mirModule->getFunction(mangledName);

        if (opFunc) {
          MIRValue *thisArg = val;
          if (val->getType() &&
              val->getType()->getKind() != hir::TypeKind::Pointer) {
            thisArg =
                builder->createAlloca(val->getType(), "op.this", expr.getLoc());
            builder->createStore(val, thisArg, expr.getLoc());
          }

          if (!opFunc->getRawArguments().empty()) {
            const hir::HIRType *expectedThisTy =
                opFunc->getRawArguments()[0]->getType();
            if (thisArg->getType() != expectedThisTy) {
              thisArg = builder->createBitCast(thisArg, expectedThisTy, "",
                                               expr.getLoc());
            }
          }

          lastExprValue =
              builder->createCall(opFunc, {thisArg}, opFunc->getType(),
                                  "op.call", false, expr.getLoc());
          return;
        }
      }
    }

    switch (op) {
    case hir::UnaryOp::Not: {
      auto *zero =
          mirModule->getOrInsertConstant<ConstantInt>(0, val->getType());
      const hir::HIRType *boolTy =
          const_cast<hir::HIRModule *>(hirModule)->getBoolType();
      lastExprValue = builder->createICmp(CompareInst::Predicate::EQ, val, zero,
                                          boolTy, "lnot", expr.getLoc());
      break;
    }
    case hir::UnaryOp::BitNot: {
      visit(expr.getOperand());
      MIRValue *operand = lastExprValue;
      uint64_t mask = ~0ULL;
      if (auto *intTy =
              llvm::dyn_cast_or_null<hir::HIRIntType>(operand->getType())) {
        if (intTy->getWidth() < 64) {
          mask = (1ULL << intTy->getWidth()) - 1;
        }
      }

      MIRValue *allOnes =
          mirModule->getOrInsertConstant<ConstantInt>(mask, operand->getType());

      lastExprValue = builder->insert(std::make_unique<BinaryInst>(
          Opcode::Xor, operand, allOnes, "bnot", expr.getLoc()));
      break;
    }
    case hir::UnaryOp::Neg: {
      if (val->getType()->getKind() == hir::TypeKind::Float ||
          val->getType()->getKind() == hir::TypeKind::Decimal) {
        auto *negZero =
            mirModule->getOrInsertConstant<ConstantFloat>(-0.0, val->getType());
        lastExprValue = builder->insert(std::make_unique<BinaryInst>(
            Opcode::FSub, negZero, val, "fneg", expr.getLoc()));
      } else {
        auto *zero =
            mirModule->getOrInsertConstant<ConstantInt>(0, val->getType());
        lastExprValue = builder->createSub(zero, val, "neg", expr.getLoc());
      }
      break;
    }
    default:
      lastExprValue = val;
      break;
    }
  }

  void visitMemberExpr(const hir::HIRMemberExpr &expr) override {
    /** @brief Detect Namespace Access (e.g., `io.open`) or Enums (e.g.,
    /* `Color.Green`) */
    if (auto *ident =
            llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(expr.getObject())) {
      std::string baseName = ident->getName();
      if (symbolMap.find(baseName) == symbolMap.end() &&
          !mirModule->getGlobal(baseName)) {
        std::string fullName = baseName + "." + expr.getMemberName();

        if (MIRFunction *func = mirModule->getFunction(fullName)) {
          const hir::HIRType *targetTy = expr.getType();
          if (targetTy && (targetTy->getKind() == hir::TypeKind::Pointer ||
                           targetTy->getKind() == hir::TypeKind::Function)) {
            lastExprValue =
                mirModule->getOrInsertConstant<ConstantBitCast>(func, targetTy);
          } else {
            lastExprValue = func;
          }
          return;
        }
        if (MIRGlobal *glob = mirModule->getGlobal(fullName)) {
          lastExprValue = builder->createLoad(glob, "enum.val", expr.getLoc());
          return;
        }

        const hir::HIRType *actualTy = expr.getType();
        const hir::HIRType *unwrappedTy = actualTy;
        if (actualTy) {
          if (auto *ptrTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(actualTy)) {
            unwrappedTy = ptrTy->getPointee();
          }
        }

        bool isFunction =
            unwrappedTy && unwrappedTy->getKind() == hir::TypeKind::Function;

        if (isFunction) {
          auto *fnTy = llvm::dyn_cast_or_null<hir::FunctionType>(unwrappedTy);
          auto externFunc = std::make_unique<MIRFunction>(
              fnTy->getReturnType(), fullName, Linkage::External);
          unsigned idx = 0;
          for (const auto *pTy : fnTy->getParamTypes()) {
            externFunc->addArgument(
                std::make_unique<MIRArgument>(externFunc.get(), pTy, idx++));
          }
          lastExprValue = externFunc.get();
          mirModule->addFunction(std::move(externFunc));
        } else {
          if (!actualTy || actualTy->getKind() == hir::TypeKind::Void) {
            actualTy =
                const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
          }
          if ((actualTy->getKind() == hir::TypeKind::Int &&
               !mirModule->getGlobal(fullName))) {
            if (enumVariantValues.find(fullName) == enumVariantValues.end()) {
              uint64_t currentVal = 0;
              for (const auto &pair : enumVariantValues) {
                if (pair.first.find(baseName + ".") == 0) {
                  currentVal++;
                }
              }
              enumVariantValues[fullName] = currentVal;
            }
            lastExprValue = mirModule->getOrInsertConstant<ConstantInt>(
                enumVariantValues[fullName], actualTy);
            return;
          }
          auto *enumGlobal =
              builder->createGlobal(mirModule.get(), fullName, actualTy,
                                    nullptr, true, Linkage::External);
          lastExprValue =
              builder->createLoad(enumGlobal, "enum.val", expr.getLoc());
        }
        return;
      }
    }

    // Standard Object Member Access
    MIRValue *base = evaluateAsLValue(expr.getObject());
    if (!base)
      return;

    if (base && base->getType() &&
        base->getType()->getKind() == hir::TypeKind::Pointer) {
      auto *ptrTy = static_cast<const hir::PointerType *>(base->getType());
      const auto *pointee = ptrTy->getPointee();

      bool isManaged = false;
      if (ptrTy->getOwnership() != hir::Ownership::None &&
          ptrTy->getOwnership() != hir::Ownership::Borrowed) {
        isManaged = true;
      }

      if (pointee &&
          (pointee->getKind() == hir::TypeKind::Pointer ||
           pointee->getKind() == hir::TypeKind::Reference ||
           pointee->getKind() == hir::TypeKind::Nullable || isManaged ||
           pointee->toString().find("shared") != std::string::npos ||
           pointee->toString().find("weak") != std::string::npos ||
           pointee->toString().find("Box<") != std::string::npos ||
           pointee->toString().find("Arc<") != std::string::npos)) {

        if (llvm::isa<AllocaInst>(base) || llvm::isa<GetElementPtrInst>(base) ||
            llvm::isa<MIRGlobal>(base) || llvm::isa<CastInst>(base) ||
            llvm::isa<MIRArgument>(base)) {
          auto *baseLoad =
              builder->createLoad(base, "base.load", expr.getLoc());
          baseLoad->setBorrowKind(mir::BorrowKind::View);
          base = baseLoad;
        } else if (auto *existingLoad =
                       llvm::dyn_cast_or_null<LoadInst>(base)) {
          existingLoad->setBorrowKind(mir::BorrowKind::View);
        }
      }
    }

    if (base && base->getType() &&
        base->getType()->getKind() == hir::TypeKind::Pointer) {
      auto *ptrTy = static_cast<const hir::PointerType *>(base->getType());
      if (ptrTy->getPointee()->getKind() == hir::TypeKind::Nullable) {
        if (llvm::isa<PhiInst>(base) || llvm::isa<AllocaInst>(base) ||
            llvm::isa<GetElementPtrInst>(base) || llvm::isa<MIRGlobal>(base) ||
            llvm::isa<CastInst>(base) || llvm::isa<MIRArgument>(base)) {

          MIRBlock *validBlock = newBlock("chain.valid");
          MIRBlock *mergeBlock = newBlock("chain.merge");

          auto *nullConst =
              mirModule->getOrInsertConstant<ConstantNull>(base->getType());
          auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
          MIRValue *isNotNull =
              builder->createICmp(CompareInst::Predicate::NE, base, nullConst,
                                  boolTy, "chain.notnull", expr.getLoc());

          MIRBlock *checkBlock = builder->getInsertBlock();
          builder->createCondBr(isNotNull, validBlock, mergeBlock);

          builder->setInsertPoint(validBlock);
          MIRValue *loadedVal =
              builder->createLoad(base, "chain.load", expr.getLoc());
          loadedVal->setBorrowKind(mir::BorrowKind::View);
          MIRBlock *loadedBlock = builder->getInsertBlock();
          builder->createBr(mergeBlock);

          builder->setInsertPoint(mergeBlock);
          auto *phi = builder->createPhi(loadedVal->getType(), "chain.phi",
                                         expr.getLoc());
          phi->addIncoming(mirModule->getOrInsertConstant<ConstantNull>(
                               loadedVal->getType()),
                           checkBlock);
          phi->addIncoming(loadedVal, loadedBlock);
          base = phi;
        }
      }
    }

    // Optional Chaining Short-Circuit (?. operator)
    bool isOptional = base->getType() &&
                      base->getType()->getKind() == hir::TypeKind::Nullable;
    MIRBlock *checkBlock = nullptr;
    MIRBlock *accessBlock = nullptr;
    MIRBlock *mergeBlock = nullptr;

    if (isOptional) {
      checkBlock = builder->getInsertBlock();
      accessBlock = newBlock("opt.access");
      mergeBlock = newBlock("opt.end");

      auto *nullConst =
          mirModule->getOrInsertConstant<ConstantNull>(base->getType());
      const hir::HIRType *boolTy =
          const_cast<hir::HIRModule *>(hirModule)->getBoolType();

      MIRValue *isNotNull =
          builder->createICmp(CompareInst::Predicate::NE, base, nullConst,
                              boolTy, "opt.notnull", expr.getLoc());
      builder->createCondBr(isNotNull, accessBlock, mergeBlock);

      builder->setInsertPoint(accessBlock);
    }

    if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(base)) {
      const hir::HIRType *loadedTy = loadInst->getType();
      bool isPointerOrRef =
          loadedTy && (loadedTy->getKind() == hir::TypeKind::Pointer ||
                       loadedTy->getKind() == hir::TypeKind::Reference);
      bool isSmartPtr = false;
      if (auto *st = llvm::dyn_cast_or_null<hir::StructType>(loadedTy)) {
        std::string name = st->getName().str();
        if (name.find("Box<") != std::string::npos ||
            name.find("Arc<") != std::string::npos) {
          isSmartPtr = true;
        }
      }
      if (!isPointerOrRef && !isSmartPtr && loadedTy &&
          (loadedTy->getKind() == hir::TypeKind::Struct ||
           loadedTy->getKind() == hir::TypeKind::Union)) {
        base = loadInst->getPointer();
      }
    } else if (base->getType() &&
               base->getType()->getKind() == hir::TypeKind::Struct) {
      auto *tempAlloca =
          builder->createAlloca(base->getType(), "struct.spill", expr.getLoc());
      builder->insert(
          std::make_unique<StoreInst>(base, tempAlloca, expr.getLoc()));
      base = tempAlloca;
    }

    std::string fieldName = expr.getMemberName();
    unsigned fieldIndex = 0;
    const hir::HIRType *fieldType = nullptr;
    const hir::HIRType *resolvedAggregateTy = nullptr;
    bool isUnionField = false;

    const hir::HIRType *objAstTy = expr.getObject()->getType();
    if (!objAstTy && base)
      objAstTy = base->getType();

    // Unwrap smart pointers to find the inner struct fields
    while (objAstTy) {
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(objAstTy)) {
        objAstTy = ptrTy->getPointee();
      } else if (auto *refTy =
                     llvm::dyn_cast_or_null<hir::ReferenceType>(objAstTy)) {
        objAstTy = refTy->getInner();
      } else {
        break;
      }
    }

    std::string typeName;
    if (objAstTy) {
      typeName = objAstTy->toString();
      while (!typeName.empty() &&
             (typeName[0] == '&' || typeName[0] == '*' || typeName[0] == ' ')) {
        typeName = typeName.substr(1);
      }
    }

    size_t boxPos = typeName.find("Box<");
    size_t arcPos = typeName.find("Arc<");
    size_t startPos = (boxPos != std::string::npos) ? boxPos : arcPos;

    if (startPos != std::string::npos) {
      typeName = typeName.substr(startPos + 4);
      size_t endPos = typeName.rfind(">");
      if (endPos != std::string::npos) {
        typeName = typeName.substr(0, endPos);
      }
    }

    // Force Lookup in Classes
    const hir::HIRClass *targetCls = nullptr;
    for (const auto *cls : hirModule->getClasses()) {
      if (!typeName.empty() &&
          typeName.find(cls->getName()) != std::string::npos) {
        targetCls = cls;
        break;
      }
    }

    auto memberInfo = expr.getMemberInfo();

    // --- FIX: Use exact physical index for Bitfields to prevent offset 0
    // corruption ---
    if (memberInfo.isBitfield) {
      fieldIndex = memberInfo.index;
      fieldType = expr.getType();
      if (auto *pTy =
              llvm::dyn_cast_or_null<hir::PointerType>(base->getType())) {
        resolvedAggregateTy = pTy->getPointee();
      }
    } else {
      /** @brief Helper to recursively extract the field, bypassing opaque
      wrappers like Box/Arc */
      std::vector<const hir::HIRType *> visitedTy;
      auto searchField = [&](auto &self, const hir::HIRType *ty,
                             const hir::HIRClass *currentCls) -> bool {
        if (!ty)
          return false;

        const hir::HIRType *innerTy = ty;
        while (innerTy) {
          if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(innerTy)) {
            innerTy = ptrTy->getPointee();
          } else if (auto *refTy =
                         llvm::dyn_cast_or_null<hir::ReferenceType>(innerTy)) {
            innerTy = refTy->getInner();
          } else {
            break;
          }
        }

        for (auto *v : visitedTy) {
          if (v == innerTy)
            return false;
        }
        visitedTy.push_back(innerTy);

        if (auto *st = llvm::dyn_cast_or_null<hir::StructType>(innerTy)) {
          int idx = st->getFieldIndex(fieldName);
          if (idx >= 0) {
            if (innerTy->getKind() == hir::TypeKind::Union) {
              isUnionField = true;
            }

            //  INHERITANCE FIELD OFFSET ACCUMULATION
            size_t baseOffset = 0;
            if (currentCls) {
              const hir::HIRClass *c = currentCls;
              while (!c->getParentTypes().empty()) {
                std::string pName = c->getParentTypes()[0]->toString();
                while (!pName.empty() && !isalnum(pName[0]))
                  pName = pName.substr(1);

                const hir::HIRClass *pCls = nullptr;
                for (const auto *cls : hirModule->getClasses()) {
                  std::string clsName = cls->getName();
                  size_t bPos = clsName.find('<');
                  if (bPos != std::string::npos)
                    clsName = clsName.substr(0, bPos);

                  std::string searchName = pName;
                  size_t pPos = searchName.find('<');
                  if (pPos != std::string::npos)
                    searchName = searchName.substr(0, pPos);

                  if (clsName == searchName) {
                    pCls = cls;
                    break;
                  }
                }

                if (pCls) {
                  const hir::HIRType *pTy = pCls->getType();
                  if (auto *ptrTy =
                          llvm::dyn_cast_or_null<hir::PointerType>(pTy))
                    pTy = ptrTy->getPointee();
                  if (auto *pSt =
                          llvm::dyn_cast_or_null<hir::StructType>(pTy)) {
                    baseOffset += pSt->getFields().size();
                  }
                  c = pCls;
                } else {
                  break;
                }
              }
            }

            fieldIndex = isUnionField ? 0 : (baseOffset + idx);
            fieldType = st->getFields()[idx];
            resolvedAggregateTy = st;
            return true;
          }

          // Inheritance Traversal
          if (currentCls) {
            for (const auto *parentTy : currentCls->getParentTypes()) {
              std::string pName = parentTy->toString();
              while (!pName.empty() &&
                     (pName[0] == '&' || pName[0] == '*' || pName[0] == ' ')) {
                pName = pName.substr(1);
              }
              const hir::HIRClass *pCls = nullptr;
              for (const auto *c : hirModule->getClasses()) {
                if (c->getName() == pName) {
                  pCls = c;
                  break;
                }
              }
              if (self(self, parentTy, pCls)) {
                return true;
              }
            }
          }

          for (const auto *fieldTy : st->getFields()) {
            if (self(self, fieldTy, nullptr))
              return true;
          }
        } else if (auto *ut = llvm::dyn_cast_or_null<hir::UnionType>(innerTy)) {
          int idx = ut->getFieldIndex(fieldName);
          if (idx >= 0) {
            fieldIndex = idx;
            fieldType = ut->getFields()[idx];
            resolvedAggregateTy = ut;
            isUnionField = true;
            return true;
          }
        }
        return false;
      };

      bool found = searchField(searchField, objAstTy, targetCls);
      if (!found && targetCls) {
        found = searchField(searchField, targetCls->getType(), targetCls);
      }

      if (!fieldType) {
        fieldType = expr.getType();
        if (!fieldType) {
          fieldType = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        }
      }
      fieldType = stripMemoryModifiers(fieldType);
    }

    MIRValue *accessPtr = nullptr;
    if (resolvedAggregateTy) {
      bool needsCast = true;
      if (base->getType() &&
          base->getType()->getKind() == hir::TypeKind::Pointer) {
        auto *ptrTy = static_cast<const hir::PointerType *>(base->getType());
        if (ptrTy->getPointee() == resolvedAggregateTy) {
          needsCast = false;
        }
      }

      if (needsCast) {
        auto *expectedPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                resolvedAggregateTy, hir::Ownership::None);
        base = builder->createBitCast(base, expectedPtrTy, "base.cast",
                                      expr.getLoc());
      }

      if (isUnionField) {
        // UNION ACCESS: All fields share the same memory address (offset 0).
        auto *expectedPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                fieldType, hir::Ownership::None);
        accessPtr = builder->createBitCast(base, expectedPtrTy,
                                           fieldName + ".ptr", expr.getLoc());
      } else {
        // STRUCT ACCESS: Use GEP to calculate the sequential memory offset.
        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
        auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
        auto *idxVal =
            mirModule->getOrInsertConstant<ConstantInt>(fieldIndex, i32Ty);

        accessPtr =
            builder->createGEP(base, {zero, idxVal}, resolvedAggregateTy,
                               fieldName + ".ptr", expr.getLoc());
      }

      auto *correctPtrTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              fieldType, hir::Ownership::None);
      if (accessPtr->getType() != correctPtrTy) {
        accessPtr = builder->createBitCast(
            accessPtr, correctPtrTy, fieldName + ".gep.cast", expr.getLoc());
      }
    } else {
      accessPtr = builder->createBitCast(
          base,
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              fieldType, hir::Ownership::None),
          fieldName + ".ptr", expr.getLoc());
    }

    if (isLValueContext) {
      lastExprValue = accessPtr;
    } else {
      if (isWeakMemory(fieldType)) {
        lastExprValue = builder->createLoadWeak(
            accessPtr, fieldType, fieldName + ".weak", expr.getLoc());
      } else {
        auto *loadInst =
            builder->createLoad(accessPtr, fieldName + ".val", expr.getLoc());
        if (isVolatilePointer(accessPtr))
          loadInst->setVolatile(true);
        loadInst->setBorrowKind(mir::BorrowKind::View);
        auto memberInfo = expr.getMemberInfo();
        if (memberInfo.isBitfield) {
          auto *shiftAmt = mirModule->getOrInsertConstant<ConstantInt>(
              memberInfo.bitOffset, loadInst->getType());
          MIRValue *shifted = builder->createShr(
              loadInst, shiftAmt, fieldName + ".shr", expr.getLoc());
          uint64_t mask = (1ULL << memberInfo.bitWidth) - 1;
          auto *maskConst = mirModule->getOrInsertConstant<ConstantInt>(
              mask, loadInst->getType());
          lastExprValue = builder->createAnd(
              shifted, maskConst, fieldName + ".mask", expr.getLoc());
        } else {
          lastExprValue = loadInst;
        }
      }
    }

    if (isOptional) {
      MIRValue *loadedVal = lastExprValue;
      const hir::HIRType *resTy = expr.getType();
      if (isLValueContext) {
        resTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            fieldType, hir::Ownership::None);
      } else if (!resTy || resTy->getKind() == hir::TypeKind::Void) {
        resTy = loadedVal->getType();
      }

      MIRValue *castLoaded = loadedVal;
      if (loadedVal->getType() != resTy) {
        bool isPrimitiveNullable = false;
        if (resTy->getKind() == hir::TypeKind::Nullable) {
          auto *nullTy = static_cast<const hir::HIRNullableType *>(resTy);
          auto kind = nullTy->getInner()->getKind();
          if (kind == hir::TypeKind::Int || kind == hir::TypeKind::Float ||
              kind == hir::TypeKind::Decimal || kind == hir::TypeKind::Bool) {
            isPrimitiveNullable = true;
          }
        }

        if (isPrimitiveNullable &&
            loadedVal->getType()->getKind() != hir::TypeKind::Pointer) {
          MIRValue *spill = builder->createAlloca(
              loadedVal->getType(), "opt.idx.spill", expr.getLoc());
          builder->insert(
              std::make_unique<StoreInst>(loadedVal, spill, expr.getLoc()));
          castLoaded = builder->createBitCast(spill, resTy, "opt.idx.box.cast",
                                              expr.getLoc());
        } else if (resTy->getKind() == hir::TypeKind::Nullable &&
                   loadedVal->getType()->getKind() == hir::TypeKind::Array) {
          MIRValue *spill = builder->createAlloca(
              loadedVal->getType(), "opt.idx.spill", expr.getLoc());
          builder->insert(
              std::make_unique<StoreInst>(loadedVal, spill, expr.getLoc()));
          castLoaded = builder->createBitCast(spill, resTy, "opt.idx.box.cast",
                                              expr.getLoc());
        } else if (resTy->getKind() == hir::TypeKind::Any) {
          MIRValue *boxed =
              boxValue(loadedVal, loadedVal->getType(), resTy, expr.getLoc());
          castLoaded = builder->createBitCast(boxed, resTy, "opt.any.cast",
                                              expr.getLoc());
        } else {
          castLoaded = builder->createBitCast(loadedVal, resTy, "opt.cast",
                                              expr.getLoc());
        }
      }

      MIRBlock *accessEndBlock = builder->getInsertBlock();
      builder->createBr(mergeBlock);
      builder->setInsertPoint(mergeBlock);

      auto *phi = builder->createPhi(resTy, "opt.phi", expr.getLoc());
      auto *nullRes = mirModule->getOrInsertConstant<ConstantNull>(resTy);
      phi->addIncoming(nullRes, checkBlock);
      phi->addIncoming(castLoaded, accessEndBlock);
      lastExprValue = phi;
    }
  }

  void visitIndexExpr(const hir::HIRIndexExpr &expr) override {
    if (!builder->getInsertBlock()) {
      diags.report(expr.getLoc(), DiagID::err_invalid_type)
          << "Global initializer must be a constant expression";
      lastExprValue = nullptr;
      return;
    }

    MIRValue *lvalueBase = evaluateAsLValue(expr.getBase());
    MIRValue *base = lvalueBase;

    // Optional Chaining Short-Circuit
    MIRBlock *checkBlock = nullptr;
    MIRBlock *accessBlock = nullptr;
    MIRBlock *mergeBlock = nullptr;

    if (expr.isOptionalAccess()) {
      checkBlock = builder->getInsertBlock();
      accessBlock = newBlock("opt.idx.access");
      mergeBlock = newBlock("opt.idx.end");

      auto *nullConst =
          mirModule->getOrInsertConstant<ConstantNull>(base->getType());
      const hir::HIRType *boolTy =
          const_cast<hir::HIRModule *>(hirModule)->getBoolType();

      MIRValue *isNotNull =
          builder->createICmp(CompareInst::Predicate::NE, base, nullConst,
                              boolTy, "opt.idx.notnull", expr.getLoc());

      builder->createCondBr(isNotNull, accessBlock, mergeBlock);
      builder->setInsertPoint(accessBlock);
    }

    if (base && base->getType() &&
        base->getType()->getKind() == hir::TypeKind::Pointer) {
      auto *pointeeTy =
          static_cast<const hir::PointerType *>(base->getType())->getPointee();
      if (pointeeTy && pointeeTy->getKind() == hir::TypeKind::Array) {
      } else {
        auto *baseLoad = builder->createLoad(base, "base.load", expr.getLoc());
        baseLoad->setBorrowKind(mir::BorrowKind::View);
        base = baseLoad;
      }
    }

    if (expr.isOptionalAccess() && base && base->getType() &&
        base->getType()->getKind() == hir::TypeKind::Pointer) {
      if (llvm::isa<AllocaInst>(base) || llvm::isa<GetElementPtrInst>(base) ||
          llvm::isa<MIRGlobal>(base) || llvm::isa<CastInst>(base)) {
        auto *optLoad =
            builder->createLoad(base, "opt.arr.load", expr.getLoc());
        optLoad->setBorrowKind(mir::BorrowKind::View);
        base = optLoad;
      }
    }

    bool savedLValueContext = isLValueContext;
    isLValueContext = false;
    visit(expr.getIndex());
    isLValueContext = savedLValueContext;
    MIRValue *idx = lastExprValue;

    if (!base || !idx)
      return;

    /** @brief Tracks whether this is a Map or Any lookup */
    bool isMapLookup = false;
    bool isAnyLookup = false;

    const hir::HIRType *colBaseTy = stripMemoryModifiers(base->getType());
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(colBaseTy)) {
      colBaseTy = stripMemoryModifiers(ptrTy->getPointee());
    }

    if (auto *nullTy =
            llvm::dyn_cast_or_null<hir::HIRNullableType>(colBaseTy)) {
      colBaseTy = stripMemoryModifiers(nullTy->getInner());
    }

    if (colBaseTy && (colBaseTy->getKind() == hir::TypeKind::Map)) {
      isMapLookup = true;
    } else if (colBaseTy && (colBaseTy->getKind() == hir::TypeKind::Any)) {
      isAnyLookup = true;
    }

    if (isMapLookup || isAnyLookup) {
      std::string funcName =
          isAnyLookup ? "moksha_rt_any_get" : "moksha_rt_map_get";

      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          voidTy, hir::Ownership::None);
      auto *anyTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();
      auto *anyPtrTy = getABICoercedType(anyTy, true); // Any*

      MIRFunction *mapGet = mirModule->getFunction(funcName);
      if (!mapGet) {
        auto fn = std::make_unique<MIRFunction>(anyPtrTy, funcName,
                                                Linkage::External);
        const hir::HIRType *firstArgTy = isAnyLookup ? anyPtrTy : voidPtrTy;
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), firstArgTy, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), anyPtrTy, 1));
        mapGet = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *castBase = base;

      if (isAnyLookup) {
        if (castBase->getType()->getKind() == hir::TypeKind::Any) {
          auto *spill =
              builder->createAlloca(anyTy, "any.base.spill", expr.getLoc());
          builder->insert(
              std::make_unique<StoreInst>(castBase, spill, expr.getLoc()));
          castBase = spill;
        }
        if (castBase->getType() != anyPtrTy) {
          castBase = builder->createBitCast(castBase, anyPtrTy, "any.base.cast",
                                            expr.getLoc());
        }
      } else {
        if (castBase->getType() != voidPtrTy) {
          castBase = builder->createBitCast(castBase, voidPtrTy,
                                            "map.base.cast", expr.getLoc());
        }
      }

      MIRValue *castIdx = idx;
      const hir::HIRType *exactKeyTy = nullptr;
      if (colBaseTy->getKind() == hir::TypeKind::Map) {
        exactKeyTy =
            static_cast<const hir::HIRMapType *>(colBaseTy)->getKeyType();
      }

      if (exactKeyTy && exactKeyTy->getKind() == hir::TypeKind::Int &&
          castIdx->getType() != exactKeyTy) {
        if (llvm::isa<ConstantInt>(castIdx)) {
          castIdx = mirModule->getOrInsertConstant<ConstantInt>(
              static_cast<ConstantInt *>(castIdx)->getValue(), exactKeyTy);
        } else {
          castIdx = builder->createBitCast(castIdx, exactKeyTy,
                                           "map.lookup.cast", expr.getLoc());
        }
      }

      if (castIdx->getType()->getKind() != hir::TypeKind::Any) {
        castIdx = boxValue(castIdx, castIdx->getType(), anyTy, expr.getLoc());
      }

      if (castIdx->getType()->getKind() != hir::TypeKind::Pointer) {
        auto *spill = builder->createAlloca(castIdx->getType(), "map.key.spill",
                                            expr.getLoc());
        builder->insert(
            std::make_unique<StoreInst>(castIdx, spill, expr.getLoc()));
        castIdx = spill;
      }

      if (castIdx->getType() != anyPtrTy) {
        castIdx = builder->createBitCast(castIdx, anyPtrTy, "map.key.cast",
                                         expr.getLoc());
      }

      MIRValue *returnedAnyPtr =
          builder->createCall(mapGet, {castBase, castIdx}, anyPtrTy,
                              "map.val.anyptr", false, expr.getLoc());

      const hir::HIRType *targetRetTy = expr.getType();
      bool targetIsNullable =
          targetRetTy && targetRetTy->getKind() == hir::TypeKind::Nullable;

      const hir::HIRType *innerTy = targetRetTy;
      if (targetIsNullable) {
        innerTy = llvm::cast<hir::HIRNullableType>(innerTy)->getInner();
      } else if (!innerTy) {
        innerTy = anyTy;
      }

      MIRBlock *checkBlock = builder->getInsertBlock();
      MIRBlock *validBlock = newBlock("map.valid");
      MIRBlock *mergeBlock = newBlock("map.merge");

      auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
      auto *nullAnyPtr = mirModule->getOrInsertConstant<ConstantNull>(anyPtrTy);

      MIRValue *isNotNull =
          builder->createICmp(CompareInst::Predicate::NE, returnedAnyPtr,
                              nullAnyPtr, boolTy, "map.notnull", expr.getLoc());
      builder->createCondBr(isNotNull, validBlock, mergeBlock);
      builder->setInsertPoint(validBlock);

      MIRValue *loadedAny =
          builder->createLoad(returnedAnyPtr, "map.any.load", expr.getLoc());
      MIRValue *rawDataPtr = builder->insert(std::make_unique<ExtractValueInst>(
          loadedAny, 0, voidPtrTy, "map.data.ptr", expr.getLoc()));

      MIRValue *finalValidVal = nullptr;

      if (innerTy->getKind() == hir::TypeKind::Any) {
        if (targetIsNullable) {
          finalValidVal = returnedAnyPtr;
        } else {
          finalValidVal = loadedAny;
        }
      } else {
        bool isPtrType = innerTy->getKind() == hir::TypeKind::Pointer ||
                         innerTy->getKind() == hir::TypeKind::Reference ||
                         innerTy->getKind() == hir::TypeKind::String ||
                         innerTy->getKind() == hir::TypeKind::Map ||
                         innerTy->getKind() == hir::TypeKind::Closure ||
                         innerTy->getKind() == hir::TypeKind::Promise ||
                         innerTy->getKind() == hir::TypeKind::Array ||
                         innerTy->getKind() == hir::TypeKind::Slice;

        if (isPtrType) {
          finalValidVal = builder->createBitCast(
              rawDataPtr, innerTy, "map.typed.cast", expr.getLoc());
        } else {
          auto *targetDataPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  innerTy, hir::Ownership::None);
          MIRValue *typedDataPtr = builder->createBitCast(
              rawDataPtr, targetDataPtrTy, "map.typed.ptr", expr.getLoc());

          if (targetIsNullable) {
            finalValidVal = typedDataPtr;
          } else {
            finalValidVal = builder->createLoad(typedDataPtr, "map.val.load",
                                                expr.getLoc());
          }
        }
      }

      MIRBlock *validEnd = builder->getInsertBlock();
      builder->createBr(mergeBlock);
      builder->setInsertPoint(mergeBlock);
      auto *phi = builder->createPhi(targetRetTy, "map.phi", expr.getLoc());

      if (targetIsNullable) {
        auto *nullRes =
            mirModule->getOrInsertConstant<ConstantNull>(targetRetTy);
        phi->addIncoming(nullRes, checkBlock);
      } else {
        MIRValue *defaultRes = nullptr;
        if (targetRetTy->getKind() == hir::TypeKind::Int) {
          defaultRes =
              mirModule->getOrInsertConstant<ConstantInt>(0, targetRetTy);
        } else if (targetRetTy->getKind() == hir::TypeKind::Float ||
                   targetRetTy->getKind() == hir::TypeKind::Decimal) {
          defaultRes =
              mirModule->getOrInsertConstant<ConstantFloat>(0.0, targetRetTy);
        } else if (targetRetTy->getKind() == hir::TypeKind::Bool) {
          defaultRes =
              mirModule->getOrInsertConstant<ConstantBool>(false, targetRetTy);
        } else {
          defaultRes =
              mirModule->getOrInsertConstant<ConstantNull>(targetRetTy);
        }
        phi->addIncoming(defaultRes, checkBlock);
      }

      phi->addIncoming(finalValidVal, validEnd);

      lastExprValue = phi;
      lastExprValue->setBorrowKind(mir::BorrowKind::View);
      return;
    } else {
      const hir::HIRType *colTy = base->getType();
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(colTy)) {
        colTy = ptrTy->getPointee();
      }

      if (auto *nullTy = llvm::dyn_cast_or_null<hir::HIRNullableType>(colTy)) {
        colTy = nullTy->getInner();
      }

      MIRValue *lengthVal = nullptr;
      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      auto *i64Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);

      if (colTy && colTy->getKind() == hir::TypeKind::Slice) {
        ensureBuiltinMIR("moksha_rt_array_length");
        MIRFunction *lenFunc = mirModule->getFunction("moksha_rt_array_length");
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);
        if (!lenFunc) {
          auto fn = std::make_unique<MIRFunction>(
              i32Ty, "moksha_rt_array_length", Linkage::External);
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
          lenFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        MIRValue *voidCol =
            builder->createBitCast(base, voidPtrTy, "col.cast", expr.getLoc());
        MIRValue *len32 = builder->createCall(
            lenFunc, {voidCol}, i32Ty, "slice.len.i32", false, expr.getLoc());
        lengthVal = builder->insert(std::make_unique<CastInst>(
            Opcode::ZExt, len32, i64Ty, "slice.len", expr.getLoc()));
      } else if (auto *arrTy = llvm::dyn_cast_or_null<hir::ArrayType>(colTy)) {
        lengthVal = mirModule->getOrInsertConstant<ConstantInt>(
            arrTy->getSize(), i64Ty);
      }

      if (lengthVal) {
        const hir::HIRType *boolTy =
            const_cast<hir::HIRModule *>(hirModule)->getBoolType();

        MIRValue *idxI64 = idx;
        if (idxI64->getType() != i64Ty) {
          idxI64 = builder->insert(std::make_unique<CastInst>(
              Opcode::SExt, idx, i64Ty, "idx.sext", expr.getLoc()));
        }

        auto *zeroI64 = mirModule->getOrInsertConstant<ConstantInt>(0, i64Ty);
        MIRValue *isNeg =
            builder->createICmp(CompareInst::Predicate::LT, idxI64, zeroI64,
                                boolTy, "idx.is_neg", expr.getLoc());
        MIRValue *isOob =
            builder->createICmp(CompareInst::Predicate::GE, idxI64, lengthVal,
                                boolTy, "idx.is_oob", expr.getLoc());
        MIRValue *isInvalid = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Or, isNeg, isOob, "idx.invalid", expr.getLoc()));

        MIRBlock *panicBlock = newBlock("bounds.panic");
        MIRBlock *contBlock = newBlock("bounds.cont");
        builder->createCondBr(isInvalid, panicBlock, contBlock);

        builder->setInsertPoint(panicBlock);
        std::string panicName = "moksha_rt_panic_out_of_bounds";
        MIRFunction *panicFunc = mirModule->getFunction(panicName);
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();

        if (!panicFunc) {
          auto fn = std::make_unique<MIRFunction>(voidTy, panicName,
                                                  Linkage::External);
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 0));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 1));
          panicFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        builder->createCall(panicFunc, {idxI64, lengthVal}, voidTy, "", false,
                            expr.getLoc());
        builder->insert(std::make_unique<UnreachableInst>(expr.getLoc()));
        builder->setInsertPoint(contBlock);
      }

      const hir::HIRType *trueElemTy = stripMemoryModifiers(expr.getType());
      if (colTy) {
        if (auto *sliceTy = llvm::dyn_cast_or_null<hir::SliceType>(colTy)) {
          trueElemTy = sliceTy->getElementType();
        } else if (auto *arrayTy =
                       llvm::dyn_cast_or_null<hir::ArrayType>(colTy)) {
          trueElemTy = arrayTy->getElementType();
        }
      }

      MIRValue *gep = nullptr;
      if (colTy && colTy->getKind() == hir::TypeKind::Array) {
        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
        auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
        gep = builder->createGEP(base, {zero, idx}, colTy, "index.ptr",
                                 expr.getLoc());
      } else if (colTy && colTy->getKind() == hir::TypeKind::Slice) {
        auto *expectedPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                trueElemTy, hir::Ownership::None);

        ensureBuiltinMIR("moksha_rt_array_data");
        MIRFunction *dataFunc = mirModule->getFunction("moksha_rt_array_data");
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);

        if (!dataFunc) {
          auto fn = std::make_unique<MIRFunction>(
              voidPtrTy, "moksha_rt_array_data", Linkage::External);
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
          dataFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        MIRValue *voidBase = builder->createBitCast(
            base, voidPtrTy, "base.void.cast", expr.getLoc());
        MIRValue *rawData =
            builder->createCall(dataFunc, {voidBase}, voidPtrTy,
                                "slice.data.raw", false, expr.getLoc());
        MIRValue *dataPtr = builder->createBitCast(
            rawData, expectedPtrTy, "slice.data.extract", expr.getLoc());
        gep = builder->createGEP(dataPtr, {idx}, trueElemTy, "index.ptr",
                                 expr.getLoc());
      } else {
        const hir::HIRType *gepPointeeTy = trueElemTy;
        if (auto *ptrTy =
                llvm::dyn_cast_or_null<hir::PointerType>(base->getType())) {
          if (ptrTy->getPointee()->getKind() != hir::TypeKind::Nullable) {
            gepPointeeTy = ptrTy->getPointee();
          }
        }
        gep = builder->createGEP(base, {idx}, gepPointeeTy, "index.ptr",
                                 expr.getLoc());
      }
      auto *expectedPtrTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              trueElemTy, hir::Ownership::None);
      if (gep->getType() != expectedPtrTy) {
        gep = builder->createBitCast(gep, expectedPtrTy, "index.ptr.cast",
                                     expr.getLoc());
      }
      if (isLValueContext) {
        lastExprValue = gep;
      } else {
        auto *loadInst = builder->insert(
            std::make_unique<LoadInst>(gep, "idx.load", expr.getLoc()));
        if (isVolatilePointer(gep)) {
          loadInst->setVolatile(true);
        }
        lastExprValue = loadInst;
      }
    }

    if (expr.isOptionalAccess()) {
      MIRValue *loadedVal = lastExprValue;
      const hir::HIRType *resTy = expr.getType();
      if (isLValueContext) {
        resTy = loadedVal->getType();
      } else if (!resTy || resTy->getKind() == hir::TypeKind::Void) {
        resTy = loadedVal->getType();
      }

      MIRValue *castLoaded = loadedVal;
      if (!isLValueContext && loadedVal->getType() != resTy) {
        bool isPrimitiveNullable = false;
        if (resTy->getKind() == hir::TypeKind::Nullable) {
          auto *nullTy = static_cast<const hir::HIRNullableType *>(resTy);
          auto kind = nullTy->getInner()->getKind();
          if (kind == hir::TypeKind::Int || kind == hir::TypeKind::Float ||
              kind == hir::TypeKind::Decimal || kind == hir::TypeKind::Bool) {
            isPrimitiveNullable = true;
          }
        }

        if (isPrimitiveNullable &&
            loadedVal->getType()->getKind() != hir::TypeKind::Pointer) {
          MIRValue *spill = builder->createAlloca(
              loadedVal->getType(), "opt.idx.spill", expr.getLoc());
          builder->insert(
              std::make_unique<StoreInst>(loadedVal, spill, expr.getLoc()));
          castLoaded = builder->createBitCast(spill, resTy, "opt.idx.box.cast",
                                              expr.getLoc());
        } else if (resTy->getKind() == hir::TypeKind::Any) {
          MIRValue *boxed =
              boxValue(loadedVal, loadedVal->getType(), resTy, expr.getLoc());
          castLoaded = builder->createBitCast(boxed, resTy, "opt.idx.any.cast",
                                              expr.getLoc());
        } else {
          castLoaded = builder->createBitCast(loadedVal, resTy, "opt.idx.cast",
                                              expr.getLoc());
        }
      }

      MIRBlock *accessEndBlock = builder->getInsertBlock();
      builder->createBr(mergeBlock);
      builder->setInsertPoint(mergeBlock);

      auto *phi = builder->createPhi(resTy, "opt.idx.phi", expr.getLoc());
      auto *nullRes = mirModule->getOrInsertConstant<ConstantNull>(resTy);
      phi->addIncoming(nullRes, checkBlock);
      phi->addIncoming(castLoaded, accessEndBlock);
      lastExprValue = phi;
    }
  }

  void visitTernaryExpr(const hir::HIRTernaryExpr &expr) override {
    visit(expr.getCond());
    MIRValue *cond = coerceToBool(lastExprValue, expr.getLoc());

    MIRBlock *trueBlock = newBlock("ternary.true");
    MIRBlock *falseBlock = newBlock("ternary.false");
    MIRBlock *mergeBlock = newBlock("ternary.end");

    builder->createCondBr(cond, trueBlock, falseBlock);

    // True Branch
    builder->setInsertPoint(trueBlock);
    visit(expr.getTrueExpr());
    MIRValue *trueVal = lastExprValue;
    if (trueVal->getType() != expr.getType()) {
      if (expr.getType()->getKind() == hir::TypeKind::Any ||
          trueVal->getType()->getKind() == hir::TypeKind::Any) {
        ensureStringifierForAny(trueVal->getType());
        trueVal = builder->insert(
            std::make_unique<CastInst>(Opcode::AnyCast, trueVal, expr.getType(),
                                       "ternary.any", expr.getLoc()));
      } else if (auto *ptrTy =
                     llvm::dyn_cast_or_null<hir::PointerType>(expr.getType());
                 ptrTy &&
                 (ptrTy->getOwnership() == hir::Ownership::Shared ||
                  ptrTy->getOwnership() == hir::Ownership::Owned) &&
                 trueVal->getType()->getKind() != hir::TypeKind::Pointer) {
        trueVal = builder->insert(std::make_unique<MakeSharedInst>(
            trueVal, expr.getType(), expr.getLoc()));
      } else {
        trueVal = builder->createBitCast(trueVal, expr.getType(),
                                         "ternary.cast", expr.getLoc());
      }
    }
    MIRBlock *trueEnd = builder->getInsertBlock();
    builder->createBr(mergeBlock);

    // False Branch
    builder->setInsertPoint(falseBlock);
    visit(expr.getFalseExpr());
    MIRValue *falseVal = lastExprValue;
    if (falseVal->getType() != expr.getType()) {
      if (expr.getType()->getKind() == hir::TypeKind::Any ||
          falseVal->getType()->getKind() == hir::TypeKind::Any) {
        ensureStringifierForAny(falseVal->getType());
        falseVal = builder->insert(std::make_unique<CastInst>(
            Opcode::AnyCast, falseVal, expr.getType(), "ternary.any",
            expr.getLoc()));
      } else if (auto *ptrTy =
                     llvm::dyn_cast_or_null<hir::PointerType>(expr.getType());
                 ptrTy &&
                 (ptrTy->getOwnership() == hir::Ownership::Shared ||
                  ptrTy->getOwnership() == hir::Ownership::Owned) &&
                 falseVal->getType()->getKind() != hir::TypeKind::Pointer) {
        falseVal = builder->insert(std::make_unique<MakeSharedInst>(
            falseVal, expr.getType(), expr.getLoc()));
      } else {
        falseVal = builder->createBitCast(falseVal, expr.getType(),
                                          "ternary.cast", expr.getLoc());
      }
    }
    MIRBlock *falseEnd = builder->getInsertBlock();
    builder->createBr(mergeBlock);

    // Merge & Phi
    builder->setInsertPoint(mergeBlock);
    auto *phi =
        builder->createPhi(expr.getType(), "ternary.phi", expr.getLoc());
    phi->addIncoming(trueVal, trueEnd);
    phi->addIncoming(falseVal, falseEnd);

    lastExprValue = phi;
  }

  void visitCastExpr(const hir::HIRCastExpr &expr) override {
    const hir::HIRType *destTy = expr.getType();

    const hir::HIRType *checkDestTy = stripMemoryModifiers(destTy);
    if (auto *nullTy =
            llvm::dyn_cast_or_null<hir::HIRNullableType>(checkDestTy)) {
      checkDestTy = nullTy->getInner();
    }
    checkDestTy = stripMemoryModifiers(checkDestTy);

    if (destTy && destTy->getKind() == hir::TypeKind::Pointer) {
      const hir::HIRType *srcAstTy = expr.getExpr()->getType();
      if (srcAstTy && (srcAstTy->getKind() == hir::TypeKind::Slice ||
                       srcAstTy->getKind() == hir::TypeKind::Array ||
                       srcAstTy->getKind() == hir::TypeKind::Struct)) {

        MIRValue *lvalue = evaluateAsLValue(expr.getExpr());
        if (lvalue) {
          if (lvalue->getType() != destTy) {
            lastExprValue = builder->createBitCast(lvalue, destTy, "lval.cast",
                                                   expr.getLoc());
          } else {
            lastExprValue = lvalue;
          }
          applyBorrowKind(lastExprValue, destTy);
          return;
        }
      }
    }

    const hir::HIRType *oldExpected = expectedLambdaReturnType;
    expectedLambdaReturnType = checkDestTy;
    visit(expr.getExpr());
    expectedLambdaReturnType = oldExpected;

    MIRValue *val = lastExprValue;
    if (!val)
      return;

    if (!destTy || destTy->getKind() == hir::TypeKind::Void)
      return;

    if (val->getType() == destTy) {
      lastExprValue = val;
      return;
    }

    if (auto *prevCast = llvm::dyn_cast_or_null<CastInst>(val)) {
      if (prevCast->getValue()->getType() == destTy) {
        lastExprValue = prevCast->getValue();
        return;
      }
    }

    if (destTy->getKind() == hir::TypeKind::String) {
      lastExprValue = coerceToString(val, expr.getLoc());
      applyBorrowKind(lastExprValue, destTy);
      return;
    }

    if (destTy->getKind() == hir::TypeKind::Promise) {
      std::string funcName = "moksha_rt_make_resolved_promise";
      ensureBuiltinMIR(funcName);
      MIRFunction *makePromFunc = mirModule->getFunction(funcName);

      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          voidTy, hir::Ownership::None);

      if (!makePromFunc) {
        auto fn = std::make_unique<MIRFunction>(voidPtrTy, funcName,
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        makePromFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *arg = val;
      if (arg->getType() != voidPtrTy) {
        arg = builder->createBitCast(arg, voidPtrTy, "prom.arg.cast",
                                     expr.getLoc());
      }

      MIRValue *rawProm =
          builder->createCall(makePromFunc, {arg}, voidPtrTy, "resolved.prom",
                              false, expr.getLoc());
      lastExprValue =
          builder->createBitCast(rawProm, destTy, "prom.cast", expr.getLoc());
      applyBorrowKind(lastExprValue, destTy);
      return;
    }

    if (checkDestTy->getKind() == hir::TypeKind::Slice) {
      const hir::HIRType *valTy = val->getType();
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(valTy)) {
        valTy = ptrTy->getPointee();
      }

      if (valTy && valTy->getKind() == hir::TypeKind::Array) {
        auto *arrayTy = static_cast<const hir::ArrayType *>(valTy);
        auto loc = expr.getLoc();
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);
        auto *i64Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
        MIRValue *stackPointer = val;
        if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(val)) {
          stackPointer = loadInst->getPointer();
          auto &insts = builder->getInsertBlock()->getInstructionsMut();
          insts.erase(std::remove_if(insts.begin(), insts.end(),
                                     [&](const std::unique_ptr<MIRInst> &i) {
                                       return i.get() == loadInst;
                                     }),
                      insts.end());
        } else if (stackPointer->getType()->getKind() !=
                   hir::TypeKind::Pointer) {
          MIRValue *spill = builder->createAlloca(stackPointer->getType(),
                                                  "array.spill", loc);
          builder->insert(
              std::make_unique<StoreInst>(stackPointer, spill, loc));
          stackPointer = spill;
        }

        if (stackPointer->getType() != voidPtrTy) {
          stackPointer = builder->createBitCast(stackPointer, voidPtrTy,
                                                "array.void.cast", loc);
        }

        MIRValue *arrayLen = mirModule->getOrInsertConstant<ConstantInt>(
            arrayTy->getSize(), i64Ty);
        std::string viewName = "moksha_rt_array_view";
        ensureBuiltinMIR(viewName);
        MIRFunction *viewFunc = mirModule->getFunction(viewName);
        if (!viewFunc) {
          auto fn = std::make_unique<MIRFunction>(voidPtrTy, viewName,
                                                  Linkage::External);
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 1));
          viewFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        MIRValue *slicePtr =
            builder->createCall(viewFunc, {stackPointer, arrayLen}, voidPtrTy,
                                "slice.view", false, loc);

        if (slicePtr->getType() != destTy) {
          lastExprValue =
              builder->createBitCast(slicePtr, destTy, "slice.cast", loc);
        } else {
          lastExprValue = slicePtr;
        }
        applyBorrowKind(lastExprValue, destTy);
        return;
      }
    }

    auto srcKind = val->getType()->getKind();
    auto dstKind = destTy->getKind();

    if (destTy->getKind() == hir::TypeKind::Any) {
      lastExprValue = boxValue(val, val->getType(), destTy, expr.getLoc());
    } else if (val->getType()->getKind() == hir::TypeKind::Any) {
      lastExprValue = unboxValue(val, val->getType(), destTy, expr.getLoc());
    } else if (destTy->getKind() == hir::TypeKind::Nullable ||
               val->getType()->getKind() == hir::TypeKind::Nullable) {
      lastExprValue = coerceValue(val, destTy, expr.getLoc());
    } else if (expr.getOp() == hir::CastOp::Upcast) {
      lastExprValue =
          builder->createUpcast(val, destTy, "cast.upcast", expr.getLoc());
    } else if ((srcKind == hir::TypeKind::Float &&
                dstKind == hir::TypeKind::Decimal) ||
               (srcKind == hir::TypeKind::Decimal &&
                dstKind == hir::TypeKind::Float) ||
               (srcKind == hir::TypeKind::Decimal &&
                dstKind == hir::TypeKind::Decimal)) {
      lastExprValue = coerceValue(val, destTy, expr.getLoc());
    } else {
      mir::Opcode mirOp = mir::Opcode::BitCast;
      switch (expr.getOp()) {
      case hir::CastOp::IntToFloat:
        mirOp = mir::Opcode::IntToFloat;
        break;
      case hir::CastOp::FloatToInt:
        mirOp = mir::Opcode::FloatToInt;
        break;
      case hir::CastOp::Truncate:
        mirOp = mir::Opcode::Trunc;
        break;
      case hir::CastOp::SignExtend:
        mirOp = mir::Opcode::SExt;
        break;
      case hir::CastOp::ZeroExtend:
        mirOp = mir::Opcode::ZExt;
        break;
      case hir::CastOp::PointerCast:
      case hir::CastOp::BitCast:
      default:
        mirOp = mir::Opcode::BitCast;
        break;
      }

      lastExprValue = builder->insert(std::make_unique<CastInst>(
          mirOp, val, destTy, "cast", expr.getLoc()));
    }

    if (val->getBorrowKind() != mir::BorrowKind::None) {
      lastExprValue->setBorrowKind(val->getBorrowKind());
    } else {
      applyBorrowKind(lastExprValue, destTy);
    }
  }

  void visitNewExpr(const hir::HIRNewExpr &expr) override {
    const hir::HIRType *objTy = resolveType(expr.getType());

    if (expectedLambdaReturnType) {
      if (auto *expectedPtrTy = llvm::dyn_cast_or_null<hir::PointerType>(
              expectedLambdaReturnType)) {
        if (expectedPtrTy->getPointee() == objTy ||
            expectedPtrTy->getPointee()->toString() == objTy->toString()) {
          objTy = expectedLambdaReturnType;
        }
      }
    }

    std::string className;
    const hir::HIRType *pointeeTy = objTy;

    auto *i32Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);

    bool isPtr = false;
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(objTy)) {
      pointeeTy = ptrTy->getPointee();
      isPtr = true;
    }

    if (auto *stTy = llvm::dyn_cast_or_null<hir::StructType>(pointeeTy)) {
      className = stTy->getName().str();
      size_t bracketPos = className.find('<');
      if (bracketPos != std::string::npos) {
        className = className.substr(0, bracketPos);
      }

      if (className.find("struct.") == 0) {
        className = className.substr(7);
      } else if (className.find("class.") == 0) {
        className = className.substr(6);
      }
    } else {
      pointeeTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    if (className == "AsyncMutex") {
      std::string factoryName = "AsyncMutex_new";
      ensureBuiltinMIR(factoryName);
      MIRFunction *factoryFunc = mirModule->getFunction(factoryName);
      if (!factoryFunc) {
        auto fn = std::make_unique<MIRFunction>(objTy, factoryName,
                                                Linkage::External);
        factoryFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      lastExprValue = builder->createCall(factoryFunc, {}, objTy, "mtx.new",
                                          false, expr.getLoc());
      return;
    }

    if (className == "Channel" || className.find("Channel_") == 0 ||
        className.find("Channel<") == 0) {
      std::string factoryName = "moksha_builtin_Channel_new";
      ensureBuiltinMIR(factoryName);
      MIRFunction *factoryFunc = mirModule->getFunction(factoryName);
      if (!factoryFunc) {
        auto fn = std::make_unique<MIRFunction>(objTy, factoryName,
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 0));
        factoryFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      visit(expr.getArgs()[0].get());
      MIRValue *capacityVal = lastExprValue;
      if (capacityVal->getType() != i32Ty) {
        capacityVal = builder->createBitCast(capacityVal, i32Ty, "cap.cast",
                                             expr.getLoc());
      }
      lastExprValue = builder->createCall(factoryFunc, {capacityVal}, objTy,
                                          "chan.new", false, expr.getLoc());
      return;
    }

    const hir::HIRClass *targetCls = nullptr;
    for (const auto *cls : hirModule->getClasses()) {
      if (cls->getName() == className) {
        targetCls = cls;
        break;
      }
    }

    bool isExternCtor = false;
    if (targetCls) {
      for (const auto &m : targetCls->getMethods()) {
        if (m->getName() == "constructor" && m->isExtern()) {
          isExternCtor = true;
          break;
        }
      }
    }

    MIRValue *objPtr = nullptr;

    if (isPtr) {
      ensureBuiltinMIR("__moksha_alloc");
      MIRFunction *allocFunc = mirModule->getFunction("__moksha_alloc");

      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
          hir::Ownership::None);

      if (!allocFunc) {
        auto fn = std::make_unique<MIRFunction>(voidPtrTy, "__moksha_alloc",
                                                Linkage::External);
        auto *i64Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 1));
        allocFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *sizeVal = nullptr;
      if (isExternCtor || className == "Channel" || className == "Thread") {
        auto *i64Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
        sizeVal = mirModule->getOrInsertConstant<ConstantInt>(64, i64Ty);
      } else {
        auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                pointeeTy, hir::Ownership::None));
        auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
        auto *sizeGep = builder->createGEP(nullPtr, {one}, pointeeTy,
                                           "sizeof.gep", expr.getLoc());
        auto *i64Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
        sizeVal = builder->insert(std::make_unique<CastInst>(
            Opcode::PtrToInt, sizeGep, i64Ty, "sizeof.i64", expr.getLoc()));
      }

      MIRValue *typeIdVal =
          mirModule->getOrInsertConstant<ConstantInt>(19, i32Ty);

      MIRValue *rawPtr =
          builder->createCall(allocFunc, {sizeVal, typeIdVal}, voidPtrTy,
                              "alloc", false, expr.getLoc());

      objPtr = builder->createBitCast(
          rawPtr,
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              pointeeTy, hir::Ownership::None),
          "new.obj", expr.getLoc());
    } else {
      uint64_t stackAlign = 0;
      if (targetCls) {
        stackAlign = targetCls->getAlignment();
      }
      objPtr = builder->createAlloca(objTy, "new.obj.stack", expr.getLoc(),
                                     stackAlign);
    }

    MIRValue *nullStruct =
        mirModule->getOrInsertConstant<ConstantNull>(pointeeTy);
    builder->insert(
        std::make_unique<StoreInst>(nullStruct, objPtr, expr.getLoc()));
    for (const auto *cls : hirModule->getClasses()) {
      if (cls->getName() == className) {
        targetCls = cls;
        break;
      }
    }

    if (targetCls && targetCls->hasVTable()) {
      std::string vtableName = className + ".vtable";
      MIRGlobal *vtableGlobal = mirModule->getGlobal(vtableName);
      if (vtableGlobal) {
        const hir::HIRType *vtableStructTy = vtableGlobal->getType();
        while (auto *pTy =
                   llvm::dyn_cast_or_null<hir::PointerType>(vtableStructTy)) {
          vtableStructTy = pTy->getPointee();
        }

        auto *vptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            vtableStructTy, hir::Ownership::None);
        auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
        MIRValue *vptrDest =
            builder->createGEP(objPtr, std::vector<MIRValue *>{zero, zero},
                               vptrTy, "vptr.dest", expr.getLoc());

        builder->insert(
            std::make_unique<StoreInst>(vtableGlobal, vptrDest, expr.getLoc()));
      }
    }

    std::vector<MIRValue *> args;
    args.push_back(objPtr);
    for (const auto &arg : expr.getArgs()) {
      visit(arg.get());
      args.push_back(lastExprValue);
    }

    std::vector<const hir::HIRType *> pTys;
    for (size_t i = 1; i < args.size(); ++i) {
      pTys.push_back(args[i]->getType());
    }

    std::string ctorName;
    if (targetCls) {
      for (const auto &m : targetCls->getMethods()) {
        if (m->getName() == "constructor" && m->isExtern()) {
          isExternCtor = true;
          break;
        }
      }
    }

    if (isExternCtor) {
      std::string baseClass = className;
      size_t under = baseClass.find('_');
      if (under != std::string::npos) {
        baseClass = baseClass.substr(0, under);
      }

      if (baseClass == "Channel") {
        ctorName = "moksha_builtin_Channel_constructor";
      } else {
        ctorName = baseClass + "_constructor";
      }
    } else {
      const hir::HIRFunction *bestCtor = nullptr;
      int bestScore = -1;

      if (targetCls) {
        for (const auto &m : targetCls->getMethods()) {
          if (m->getName() == "constructor") {
            const auto &params = m->getParams();
            if (params.size() == pTys.size()) {
              int score = 0;
              bool valid = true;
              for (size_t i = 0; i < params.size(); ++i) {
                const hir::HIRType *aTy = pTys[i];
                if (!aTy) {
                  valid = false;
                  break;
                }

                if (params[i].type == aTy ||
                    params[i].type->toString() == aTy->toString()) {
                  score += 10;
                } else if (params[i].type->getKind() == aTy->getKind()) {
                  score += 5;
                } else if (params[i].type->getKind() ==
                               hir::TypeKind::Nullable ||
                           params[i].type->toString().back() == '?') {
                  score += 3;
                } else if (params[i].type->getKind() == hir::TypeKind::Any) {
                  score += 1;
                } else {
                  valid = false;
                  break;
                }
              }
              if (valid && score > bestScore) {
                bestScore = score;
                bestCtor = m.get();
              }
            }
          }
        }
      }

      if (bestCtor) {
        std::vector<const hir::HIRType *> resolvedPTys;
        for (const auto &p : bestCtor->getParams()) {
          resolvedPTys.push_back(p.type);
        }
        ctorName =
            mangleName(className + ".constructor", resolvedPTys) + "_ret_void";
      } else {
        ctorName = mangleName(className + ".constructor", pTys) + "_ret_void";
      }
    }

    if (targetCls) {
      bool isGenericCall = false;

      if (objTy && objTy->toString().find("<") != std::string::npos) {
        isGenericCall = true;
      }

      if (isGenericCall && instantiatedGenerics.insert(ctorName).second) {
        MonomorphizationTask task;
        task.genericClass = targetCls;
        task.typeArgs = pTys;
        monoQueue.push(task);
      }
    }

    if (!ctorName.empty()) {
      MIRFunction *ctorFunc = mirModule->getFunction(ctorName);

      if (!ctorFunc) {
        auto fn = std::make_unique<MIRFunction>(
            const_cast<hir::HIRModule *>(hirModule)->getVoidType(), ctorName,
            Linkage::External);

        unsigned idx = 0;
        for (auto *a : args) {
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), a->getType(), idx++));
        }
        ctorFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      if (ctorFunc) {
        for (size_t i = 0; i < args.size(); ++i) {
          if (i < ctorFunc->getRawArguments().size()) {
            const hir::HIRType *expectedTy =
                ctorFunc->getRawArguments()[i]->getType();
            if (args[i]->getType() != expectedTy) {
              args[i] = builder->createBitCast(args[i], expectedTy, "arg.cast",
                                               expr.getLoc());
            }
          }
        }
        if (!args.empty() && args[0]) {
          args[0]->setBorrowKind(mir::BorrowKind::View);
        }

        // Exception-Safe Constructor Invocation
        if (currentUnwindDest) {
          MIRBlock *normalDest = newBlock("invoke.cont");
          MIRBlock *cleanupDest = newBlock("invoke.cleanup");

          builder->createInvoke(
              ctorFunc, std::move(args), normalDest, cleanupDest,
              const_cast<hir::HIRModule *>(hirModule)->getVoidType(), "",
              expr.getLoc());

          builder->setInsertPoint(cleanupDest);
          MIRBlock *savedUnwind = currentUnwindDest;
          currentUnwindDest = nullptr;
          size_t targetDepth =
              tryScopeDepths.empty() ? 0 : tryScopeDepths.top();
          for (size_t i = scopeStack.size(); i > targetDepth; --i) {
            emitScopeCleanup(i - 1, expr.getLoc(), true);
          }
          currentUnwindDest = savedUnwind;
          builder->createBr(currentUnwindDest);
          builder->setInsertPoint(normalDest);
        } else {
          builder->createCall(
              ctorFunc, std::move(args),
              const_cast<hir::HIRModule *>(hirModule)->getVoidType(), "", false,
              expr.getLoc());
        }
      }
    }

    if (isPtr) {
      lastExprValue = objPtr;
    } else {
      lastExprValue = builder->insert(
          std::make_unique<LoadInst>(objPtr, "new.val", expr.getLoc()));
    }
  }

  void visitLambdaExpr(const hir::HIRLambdaExpr &expr) override {
    std::string lambdaName = "lambda." + std::to_string(lambdaCounter++);
    const hir::HIRType *i32Ty =
        const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);

    std::vector<std::pair<std::string, MIRValue *>> captures;
    std::vector<const hir::HIRType *> envMemberTypes;
    std::vector<bool> captureByRef;

    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);
    envMemberTypes.push_back(voidPtrTy);

    if (!expr.getCaptures().empty()) {
      for (const auto &cap : expr.getCaptures()) {
        std::string name = cap.name;
        if (symbolMap.count(name)) {
          captures.push_back({name, symbolMap[name]});
          bool isRef = (cap.kind == hir::CaptureKind::ByReference);
          captureByRef.push_back(isRef);

          if (isRef) {
            envMemberTypes.push_back(
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    cap.type, hir::Ownership::None));
          } else {
            envMemberTypes.push_back(cap.type);
          }
        }
      }
    } else {
      for (const auto &[name, val] : symbolMap) {
        bool isParam = false;
        for (const auto &p : expr.getParams()) {
          if (p.name == name) {
            isParam = true;
            break;
          }
        }
        if (isParam)
          continue;

        if (name == "this" || isIdentifierUsed(expr.getBody(), name)) {
          captures.push_back({name, val});

          bool isRef = (expr.getCaptureMode() == hir::CaptureMode::View ||
                        expr.getCaptureMode() == hir::CaptureMode::Mut);
          const hir::HIRType *valTy = val->getType();

          if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(valTy)) {
            valTy = ptrTy->getPointee();
          }

          if (valTy && (expr.getCaptureMode() == hir::CaptureMode::Snapshot ||
                        expr.getCaptureMode() == hir::CaptureMode::Move)) {
            auto kind = valTy->getKind();
            if (kind == hir::TypeKind::String || kind == hir::TypeKind::Slice ||
                kind == hir::TypeKind::Promise ||
                kind == hir::TypeKind::Closure) {
              isRef = false;
            }
          }

          bool isMutated = false;
          if (expr.getBody()) {
            std::string buffer;
            llvm::raw_string_ostream ss(buffer);
            expr.getBody()->dump(ss);
            ss.flush();

            size_t pos = 0;
            while ((pos = buffer.find("Identifier (" + name + ")", pos)) !=
                   std::string::npos) {
              size_t newline = buffer.rfind('\n', pos);
              if (newline != std::string::npos && newline > 0) {
                size_t prevNewline = buffer.rfind('\n', newline - 1);
                size_t start =
                    prevNewline == std::string::npos ? 0 : prevNewline + 1;
                std::string prevLine = buffer.substr(start, newline - start);

                if (prevLine.find("Op: =") != std::string::npos ||
                    prevLine.find("Op: ++") != std::string::npos ||
                    prevLine.find("Op: --") != std::string::npos ||
                    prevLine.find("AddressOf") != std::string::npos) {
                  isMutated = true;
                  break;
                }
              }
              pos++;
            }
          }

          if (isMutated) {
            if (expr.getCaptureMode() != hir::CaptureMode::Move) {
              isRef = true;
            }
          }

          captureByRef.push_back(isRef);

          if (isRef) {
            envMemberTypes.push_back(
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    valTy, hir::Ownership::None));
          } else {
            envMemberTypes.push_back(valTy);
          }
        }
      }
    }

    auto *envStructTy = const_cast<hir::HIRModule *>(hirModule)->getStructType(
        "Env." + lambdaName, envMemberTypes);
    auto *envPtrTy = mirModule->getPointerType(envStructTy);
    auto envDtorF = std::make_unique<MIRFunction>(
        voidTy, "Env." + lambdaName + ".dtor", Linkage::Internal);
    auto *dtorArg = new MIRArgument(envDtorF.get(), voidPtrTy, 0);
    envDtorF->addArgument(std::unique_ptr<MIRArgument>(dtorArg));

    MIRBlock *dtorEntry = new MIRBlock("entry", envDtorF.get());
    envDtorF->addBlock(std::unique_ptr<MIRBlock>(dtorEntry));

    MIRBlock *savedBlock = builder->getInsertBlock();
    MIRFunction *savedFunc = currFunc;
    builder->setInsertPoint(dtorEntry);
    currFunc = envDtorF.get();

    MIRValue *typedEnv =
        builder->createBitCast(dtorArg, envPtrTy, "env.cast", expr.getLoc());
    auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);

    for (size_t i = 0; i < captures.size(); ++i) {
      if (!captureByRef[i]) {
        const hir::HIRType *fieldTy = envMemberTypes[i + 1];
        std::string tyStr = fieldTy->toString();

        // Differentiate ARC-managed pointers from raw stack structs
        bool typeIsARC = fieldTy->getKind() == hir::TypeKind::String ||
                         fieldTy->getKind() == hir::TypeKind::Slice ||
                         fieldTy->getKind() == hir::TypeKind::Array ||
                         fieldTy->getKind() == hir::TypeKind::Map ||
                         fieldTy->getKind() == hir::TypeKind::Closure ||
                         fieldTy->getKind() == hir::TypeKind::Any ||
                         tyStr.find("Arc<") != std::string::npos ||
                         tyStr.find("Box<") != std::string::npos;

        bool isManaged =
            typeIsARC || fieldTy->getKind() == hir::TypeKind::Struct;

        if (isManaged && !isWeakMemory(fieldTy)) {
          auto *idxVal =
              mirModule->getOrInsertConstant<ConstantInt>(i + 1, i32Ty);
          MIRValue *fieldGep = builder->createGEP(
              typedEnv, {zero, idxVal}, envStructTy, "cap.gep", expr.getLoc());
          auto *expectedPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  fieldTy, hir::Ownership::None);
          if (fieldGep->getType() != expectedPtrTy) {
            fieldGep = builder->createBitCast(fieldGep, expectedPtrTy,
                                              "cap.gep.cast", expr.getLoc());
          }

          std::string dropName = "";
          if (fieldTy->getKind() == hir::TypeKind::Struct) {
            std::string fName = fieldTy->toString();
            while (!fName.empty() &&
                   (fName[0] == '&' || fName[0] == '*' || fName[0] == ' '))
              fName = fName.substr(1);
            if (fName.find("struct ") == 0)
              fName = fName.substr(7);
            if (fName.find("class ") == 0)
              fName = fName.substr(6);
            dropName = fName + ".destructor_ret_void";
          }

          MIRFunction *dropFunc =
              dropName.empty() ? nullptr : mirModule->getFunction(dropName);
          if (dropFunc && !typeIsARC) {
            MIRValue *argVal = fieldGep;
            if (!dropFunc->getRawArguments().empty()) {
              const hir::HIRType *expectedTy =
                  dropFunc->getRawArguments()[0]->getType();
              if (argVal->getType() != expectedTy) {
                argVal = builder->createBitCast(argVal, expectedTy, "drop.cast",
                                                expr.getLoc());
              }
            }
            auto *voidRetTy =
                const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            builder->insert(std::make_unique<CallInst>(
                dropFunc, std::vector<MIRValue *>{argVal}, voidRetTy, "", false,
                expr.getLoc()));
          }

          if (typeIsARC) {
            MIRValue *fieldVal =
                builder->createLoad(fieldGep, "cap.load", expr.getLoc());
            builder->insert(std::make_unique<ARCInst>(Opcode::Release, fieldVal,
                                                      dropFunc, expr.getLoc()));
          }
        }
      }
    }
    builder->insert(std::make_unique<ReturnInst>(nullptr, expr.getLoc()));
    envDtorF->numberUnnamedValues();
    builder->setInsertPoint(savedBlock);
    currFunc = savedFunc;

    MIRFunction *dtorFuncPtr = envDtorF.get();
    mirModule->addFunction(std::move(envDtorF));

    MIRValue *envAlloca = nullptr;
    std::vector<MIRValue *> savedCapVals;

    if (!captures.empty()) {
      bool escapes = true;

      if (escapes) {
        ensureBuiltinMIR("__moksha_alloc");
        MIRFunction *allocFunc = mirModule->getFunction("__moksha_alloc");

        auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(envPtrTy);
        auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
        auto *sizeGep = builder->createGEP(nullPtr, {one}, envStructTy,
                                           "env.sizeof.gep", expr.getLoc());
        auto *i64Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
        MIRValue *sizeVal = builder->insert(std::make_unique<CastInst>(
            Opcode::PtrToInt, sizeGep, i64Ty, "env.sizeof.i64", expr.getLoc()));
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
                hir::Ownership::None);

        if (!allocFunc) {
          auto fn = std::make_unique<MIRFunction>(voidPtrTy, "__moksha_alloc",
                                                  Linkage::External);
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 0));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 1));
          allocFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }
        MIRValue *typeIdVal =
            mirModule->getOrInsertConstant<ConstantInt>(19, i32Ty);
        MIRValue *rawAlloc =
            builder->createCall(allocFunc, {sizeVal, typeIdVal}, voidPtrTy,
                                "env.alloc.heap", false, expr.getLoc());
        envAlloca = builder->createBitCast(rawAlloc, envPtrTy, "env.ptr",
                                           expr.getLoc());
      } else {
        envAlloca = builder->createAlloca(envStructTy, "env.alloc.stack",
                                          expr.getLoc());
      }

      auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
      MIRValue *dtorCast = builder->createBitCast(dtorFuncPtr, voidPtrTy,
                                                  "dtor.cast", expr.getLoc());
      MIRValue *dtorGep = builder->createGEP(
          envAlloca, {zero, zero}, envStructTy, "env.dtor.gep", expr.getLoc());
      auto *voidPtrPtrTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              voidPtrTy, hir::Ownership::None);
      MIRValue *dtorPtrCast = builder->createBitCast(
          dtorGep, voidPtrPtrTy, "env.dtor.ptr.cast", expr.getLoc());

      builder->insert(
          std::make_unique<StoreInst>(dtorCast, dtorPtrCast, expr.getLoc()));

      for (size_t i = 0; i < captures.size(); ++i) {
        MIRValue *valToStore = nullptr;

        if (captureByRef[i]) {
          valToStore = captures[i].second;
        } else {
          valToStore = builder->insert(std::make_unique<LoadInst>(
              captures[i].second, "cap.load", expr.getLoc()));

          MIRValue *movedAlloca = captures[i].second;

          for (auto &scope : scopeStack) {
            auto &owned = scope.ownedVars;
            owned.erase(std::remove(owned.begin(), owned.end(), movedAlloca),
                        owned.end());

            auto &shared = scope.refCountedVars;
            shared.erase(std::remove(shared.begin(), shared.end(), movedAlloca),
                         shared.end());
          }
        }

        savedCapVals.push_back(valToStore);
        if (valToStore->getType() != envMemberTypes[i + 1]) {
          valToStore = builder->createBitCast(valToStore, envMemberTypes[i + 1],
                                              "cap.cast", expr.getLoc());
        }

        auto *idxVal =
            mirModule->getOrInsertConstant<ConstantInt>(i + 1, i32Ty);
        MIRValue *capGep =
            builder->createGEP(envAlloca, {zero, idxVal}, envStructTy,
                               "env.cap.gep", expr.getLoc());

        auto *expectedPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                envMemberTypes[i + 1], hir::Ownership::None);
        MIRValue *capPtrCast = builder->createBitCast(
            capGep, expectedPtrTy, "env.cap.ptr.cast", expr.getLoc());
        builder->insert(
            std::make_unique<StoreInst>(valToStore, capPtrCast, expr.getLoc()));
      }
    } else {
      envAlloca = mirModule->getOrInsertConstant<ConstantNull>(envPtrTy);
    }

    const hir::HIRType *returnTy = nullptr;
    const hir::HIRType *closureTy = expr.getType();
    if (!closureTy && expectedLambdaReturnType) {
      closureTy = expectedLambdaReturnType;
    }

    if (closureTy) {
      if (auto *closTy =
              llvm::dyn_cast_or_null<hir::HIRClosureType>(closureTy)) {
        returnTy = closTy->getReturnType();
      } else if (auto *fnTy =
                     llvm::dyn_cast_or_null<hir::FunctionType>(closureTy)) {
        returnTy = fnTy->getReturnType();
      } else if (auto *structTy =
                     llvm::dyn_cast_or_null<hir::StructType>(closureTy)) {
        if (!structTy->getFields().empty()) {
          const hir::HIRType *fnField = structTy->getFields()[0];
          if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(fnField)) {
            fnField = ptrTy->getPointee();
          }
          if (auto *innerFnTy =
                  llvm::dyn_cast_or_null<hir::FunctionType>(fnField)) {
            returnTy = innerFnTy->getReturnType();
          }
        }
      }
    }

    if (!returnTy && expr.getBody()) {
      auto extractType = [&](const hir::HIRExpr *e) -> const hir::HIRType * {
        if (!e)
          return nullptr;
        if (e->getType() && e->getType()->getKind() != hir::TypeKind::Void)
          return e->getType();

        if (auto *bin = llvm::dyn_cast_or_null<hir::HIRBinaryExpr>(e)) {
          const hir::HIRExpr *lhs = bin->getLHS();
          const hir::HIRExpr *rhs = bin->getRHS();
          if (lhs && lhs->getType() &&
              lhs->getType()->getKind() != hir::TypeKind::Void)
            return lhs->getType();
          if (rhs && rhs->getType() &&
              rhs->getType()->getKind() != hir::TypeKind::Void)
            return rhs->getType();

          if (auto *id = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(lhs)) {
            for (const auto &p : expr.getParams()) {
              if (p.name == id->getName())
                return p.type;
            }
          }
        }
        if (auto *id = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(e)) {
          for (const auto &p : expr.getParams()) {
            if (p.name == id->getName())
              return p.type;
          }
        }
        return nullptr;
      };

      const hir::HIRType *inferredTy = nullptr;
      if (auto *retStmt =
              llvm::dyn_cast_or_null<hir::ReturnStmt>(expr.getBody())) {
        inferredTy = extractType(retStmt->getReturnValue());
      } else if (auto *exprStmt =
                     llvm::dyn_cast_or_null<hir::ExprStmt>(expr.getBody())) {
        inferredTy = extractType(exprStmt->getExpr());
      } else if (auto *block =
                     llvm::dyn_cast_or_null<hir::BlockStmt>(expr.getBody())) {
        if (!block->getStatements().empty()) {
          const auto &lastStmt = block->getStatements().back();
          if (auto *retStmt =
                  llvm::dyn_cast_or_null<hir::ReturnStmt>(lastStmt.get())) {
            inferredTy = extractType(retStmt->getReturnValue());
          } else if (auto *exprStmt = llvm::dyn_cast_or_null<hir::ExprStmt>(
                         lastStmt.get())) {
            inferredTy = extractType(exprStmt->getExpr());
          }
        }
      }

      if (inferredTy && inferredTy->getKind() != hir::TypeKind::Void) {
        returnTy = inferredTy;
      }
    }

    if (!returnTy) {
      returnTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    if (expr.isAsyncLambda()) {
      returnTy =
          const_cast<hir::HIRModule *>(hirModule)->getPromiseType(returnTy);
    }

    auto lambdaFunc =
        std::make_unique<MIRFunction>(returnTy, lambdaName, Linkage::Internal);

    auto *envArg = new MIRArgument(lambdaFunc.get(), envPtrTy, 0);
    lambdaFunc->addArgument(std::unique_ptr<MIRArgument>(envArg));

    MIRBlock *entry = new MIRBlock("entry", lambdaFunc.get());
    lambdaFunc->addBlock(std::unique_ptr<MIRBlock>(entry));

    MIRBlock *oldInsertPoint = builder->getInsertBlock();
    auto oldSymbolMap = std::move(symbolMap);
    MIRFunction *oldFunc = currFunc;

    auto savedScopeStack = std::move(scopeStack);
    scopeStack.clear();
    scopeStack.push_back({});

    auto savedTryDepths = std::move(tryScopeDepths);
    while (!tryScopeDepths.empty())
      tryScopeDepths.pop();

    MIRBlock *savedUnwind = currentUnwindDest;
    currentUnwindDest = nullptr;

    builder->setInsertPoint(entry);
    currFunc = lambdaFunc.get();
    symbolMap.clear();

    if (!captures.empty()) {
      auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
      for (size_t i = 0; i < captures.size(); ++i) {
        auto *idxVal =
            mirModule->getOrInsertConstant<ConstantInt>(i + 1, i32Ty);

        MIRValue *capPtr =
            builder->createGEP(envArg, {zero, idxVal}, envStructTy,
                               captures[i].first + ".ptr", expr.getLoc());

        auto *expectedPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                envMemberTypes[i + 1], hir::Ownership::None);

        if (capPtr->getType() != expectedPtrTy) {
          capPtr = builder->createBitCast(capPtr, expectedPtrTy,
                                          captures[i].first + ".gep.cast",
                                          expr.getLoc());
        }

        if (captureByRef[i]) {
          MIRValue *origPtr = builder->createLoad(
              capPtr, captures[i].first + ".ref", expr.getLoc());
          symbolMap[captures[i].first] = origPtr;
        } else {
          symbolMap[captures[i].first] = capPtr;
        }
      }
    }

    unsigned idx = 1;
    for (const auto &param : expr.getParams()) {
      auto *arg = new MIRArgument(lambdaFunc.get(), param.type, idx++);
      lambdaFunc->addArgument(std::unique_ptr<MIRArgument>(arg));

      auto *alloca =
          builder->createAlloca(param.type, param.name, expr.getLoc());
      builder->insert(std::make_unique<StoreInst>(arg, alloca, expr.getLoc()));
      symbolMap[param.name] = alloca;
    }

    lastExprValue = nullptr;
    if (expr.getBody()) {
      if (expr.isAsyncLambda()) {
        MIRBlock *tryBlock = newBlock("async.try");
        MIRBlock *catchBlock = newBlock("async.catch");
        MIRBlock *contBlock = newBlock("async.cont");

        builder->createBr(tryBlock);
        builder->setInsertPoint(tryBlock);

        MIRBlock *oldUnwind = currentUnwindDest;
        currentUnwindDest = catchBlock;
        tryScopeDepths.push(scopeStack.size());

        visit(expr.getBody());

        tryScopeDepths.pop();
        currentUnwindDest = oldUnwind;

        if (!getTerminator(builder->getInsertBlock())) {
          builder->createBr(contBlock);
        }

        // Catch Block: Reject the Promise
        builder->setInsertPoint(catchBlock);

        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);
        auto *i8Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
        auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            i8Ty, hir::Ownership::None);
        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);

        auto *lpadType = const_cast<hir::HIRModule *>(hirModule)->getStructType(
            "eh_result", {i8PtrTy, i32Ty});
        auto *lpadInst =
            builder->createLandingPad(lpadType, "async.lpad", expr.getLoc());
        lpadInst->addCatchType(voidPtrTy);
        std::string consumeName = "moksha_rt_consume_exception";
        ensureBuiltinMIR(consumeName);
        MIRFunction *consumeFunc = mirModule->getFunction(consumeName);
        if (!consumeFunc) {
          auto fn = std::make_unique<MIRFunction>(voidPtrTy, consumeName,
                                                  Linkage::External);
          consumeFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }
        MIRValue *exPayload = builder->createCall(
            consumeFunc, {}, voidPtrTy, "ex.consumed", false, expr.getLoc());

        std::string rejectName = "moksha_rt_make_rejected_promise";
        ensureBuiltinMIR(rejectName);
        MIRFunction *rejectFunc = mirModule->getFunction(rejectName);
        if (!rejectFunc) {
          auto fn = std::make_unique<MIRFunction>(voidPtrTy, rejectName,
                                                  Linkage::External);
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
          rejectFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }
        MIRValue *rejectedProm =
            builder->createCall(rejectFunc, {exPayload}, voidPtrTy,
                                "rejected.prom", false, expr.getLoc());
        MIRValue *finalRet = builder->createBitCast(rejectedProm, returnTy,
                                                    "prom.cast", expr.getLoc());
        emitScopeCleanup(scopeStack.size() - 1, expr.getLoc());
        builder->insert(std::make_unique<ReturnInst>(finalRet, expr.getLoc()));
        builder->setInsertPoint(contBlock);
      } else {
        visit(expr.getBody());
      }
    }

    if (!getTerminator(builder->getInsertBlock())) {
      bool isAsync = expr.isAsyncLambda();
      const hir::HIRType *expectedTy = returnTy;
      emitScopeCleanup(scopeStack.size() - 1, expr.getLoc());
      if (isAsync) {
        std::string funcName = "moksha_rt_make_resolved_promise";
        ensureBuiltinMIR(funcName);
        MIRFunction *makePromFunc = mirModule->getFunction(funcName);
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);
        if (!makePromFunc) {
          auto fn = std::make_unique<MIRFunction>(voidPtrTy, funcName,
                                                  Linkage::External);
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
          makePromFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        MIRValue *nullArg =
            mirModule->getOrInsertConstant<ConstantNull>(voidPtrTy);
        MIRValue *rawProm =
            builder->createCall(makePromFunc, {nullArg}, voidPtrTy,
                                "resolved.prom.def", false, expr.getLoc());
        MIRValue *finalRet = builder->createBitCast(
            rawProm, expectedTy, "prom.ret.cast", expr.getLoc());
        builder->insert(std::make_unique<ReturnInst>(finalRet, expr.getLoc()));
      } else {
        bool isVoidReturn =
            !expectedTy || expectedTy->getKind() == hir::TypeKind::Void;
        if (!isVoidReturn) {
          MIRValue *retVal = lastExprValue;
          if (!retVal || retVal->getType()->getKind() == hir::TypeKind::Void) {
            if (expectedTy->getKind() == hir::TypeKind::Int)
              retVal =
                  mirModule->getOrInsertConstant<ConstantInt>(0, expectedTy);
            else if (expectedTy->getKind() == hir::TypeKind::Float ||
                     expectedTy->getKind() == hir::TypeKind::Decimal)
              retVal = mirModule->getOrInsertConstant<ConstantFloat>(
                  0.0, expectedTy);
            else if (expectedTy->getKind() == hir::TypeKind::Bool)
              retVal = mirModule->getOrInsertConstant<ConstantBool>(false,
                                                                    expectedTy);
            else
              retVal = mirModule->getOrInsertConstant<ConstantNull>(expectedTy);
          } else if (retVal->getType() != expectedTy) {
            retVal = builder->createBitCast(retVal, expectedTy, "ret.cast",
                                            expr.getLoc());
          }

          builder->insert(std::make_unique<ReturnInst>(retVal, expr.getLoc()));
        } else {
          builder->insert(std::make_unique<ReturnInst>(nullptr, expr.getLoc()));
        }
      }
    }

    lambdaFunc->numberUnnamedValues();
    builder->setInsertPoint(oldInsertPoint);
    symbolMap = std::move(oldSymbolMap);
    currFunc = oldFunc;
    scopeStack = std::move(savedScopeStack);
    tryScopeDepths = std::move(savedTryDepths);
    currentUnwindDest = savedUnwind;

    MIRFunction *fnPtr = lambdaFunc.get();
    mirModule->addFunction(std::move(lambdaFunc));
    const hir::HIRType *lambdaRetTy = fnPtr->getType();
    if (!lambdaRetTy) {
      lambdaRetTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    std::vector<const hir::HIRType *> actualFnParams;
    actualFnParams.push_back(envPtrTy);
    for (const auto &param : expr.getParams()) {
      actualFnParams.push_back(param.type);
    }

    const hir::HIRType *actualFuncTy =
        const_cast<hir::HIRModule *>(hirModule)->getFunctionType(
            lambdaRetTy, actualFnParams);
    const hir::HIRType *fnPtrTy =
        const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            actualFuncTy, hir::Ownership::None);

    auto *fatPtrStructTy =
        const_cast<hir::HIRModule *>(hirModule)->getStructType(
            "Closure." + lambdaName, {fnPtrTy, envPtrTy});

    std::vector<MIRValue *> packedEnv;
    packedEnv.push_back(envAlloca);

    bool isClosureMut = (expr.getCaptureMode() == hir::CaptureMode::Mut);
    for (size_t i = 0; i < captures.size(); ++i) {
      MIRValue *capVal = captures[i].second;
      if (captureByRef[i]) {
        auto *taggedCap = builder->createBitCast(capVal, capVal->getType(),
                                                 "cap.track", expr.getLoc());
        bool isMutated = false;
        if (expr.getBody()) {
          std::string buffer;
          llvm::raw_string_ostream ss(buffer);
          expr.getBody()->dump(ss);
          ss.flush();
          std::string name = captures[i].first;
          size_t pos = 0;
          while ((pos = buffer.find("Identifier (" + name + ")", pos)) !=
                 std::string::npos) {
            size_t newline = buffer.rfind('\n', pos);
            if (newline != std::string::npos && newline > 0) {
              size_t prevNewline = buffer.rfind('\n', newline - 1);
              size_t start =
                  prevNewline == std::string::npos ? 0 : prevNewline + 1;
              std::string prevLine = buffer.substr(start, newline - start);
              if (prevLine.find("Op: =") != std::string::npos ||
                  prevLine.find("Op: ++") != std::string::npos ||
                  prevLine.find("Op: --") != std::string::npos ||
                  prevLine.find("AddressOf") != std::string::npos) {
                isMutated = true;
                break;
              }
            }
            pos++;
          }
        }

        if (isMutated || expr.getCaptureMode() == hir::CaptureMode::Mut) {
          taggedCap->setBorrowKind(mir::BorrowKind::Mut);
          isClosureMut = true;
        } else {
          taggedCap->setBorrowKind(mir::BorrowKind::View);
        }
        packedEnv.push_back(taggedCap);
      } else {
        packedEnv.push_back(savedCapVals[i]);
      }
    }

    MIRFunction *func = builder->getInsertBlock()->getParent();
    std::string safeName = func->getUniqueName("closure.val");
    lastExprValue = builder->createMakeClosure(
        fnPtr, std::move(packedEnv), fatPtrStructTy, safeName, expr.getLoc());
    if (isClosureMut) {
      lastExprValue->setBorrowKind(mir::BorrowKind::Mut);
    } else {
      lastExprValue->setBorrowKind(mir::BorrowKind::View);
    }
  }

  void visitThreadExpr(const hir::HIRThreadExpr &expr) override {
    bool oldEscape = inEscapeContext;
    inEscapeContext = true;
    visit(expr.getTask());
    inEscapeContext = oldEscape;

    MIRValue *closureVal = lastExprValue;
    if (!closureVal)
      return;

    const hir::HIRType *retTy = expr.getType();
    if (!retTy) {
      retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    MIRFunction *func = builder->getInsertBlock()->getParent();
    std::string callName = (retTy->getKind() == hir::TypeKind::Void)
                               ? ""
                               : func->getUniqueName("thread.handle");

    lastExprValue = builder->createSpawn(closureVal, expr.getThreadKind(),
                                         retTy, callName, expr.getLoc());
  }

  void visitThisExpr(const hir::HIRThisExpr &expr) override {
    if (symbolMap.count("this")) {
      auto *ptr = symbolMap["this"];
      lastExprValue = builder->createLoad(ptr, "this.val", expr.getLoc());
    } else if (!currFunc->getRawArguments().empty()) {
      lastExprValue = currFunc->getRawArguments()[0];
    } else {
      lastExprValue = nullptr;
    }
  }

  void visitSizeOfExpr(const hir::HIRSizeOfExpr &expr) override {
    const hir::HIRType *targetTy = stripMemoryModifiers(expr.getTargetType());
    if (!targetTy) {
      targetTy = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    }

    auto *usizeTy =
        const_cast<hir::HIRModule *>(hirModule)->getIntType(64, false);
    auto *i32Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    auto *i8Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();

    // 1. Recursive Alignment Resolution Helper
    auto getMaxAlignment = [&](auto &self, const hir::HIRType *ty) -> uint64_t {
      if (!ty)
        return 0;
      const hir::HIRType *stripped = stripMemoryModifiers(ty);
      uint64_t maxAlign = 0;

      std::string cName = stripped->toString();
      while (!cName.empty() &&
             (cName[0] == '*' || cName[0] == '&' || cName[0] == ' ')) {
        cName = cName.substr(1);
      }
      if (cName.find("struct.") == 0)
        cName = cName.substr(7);
      if (cName.find("class.") == 0)
        cName = cName.substr(6);
      if (cName.find("union.") == 0)
        cName = cName.substr(6);

      const hir::HIRClass *matchedCls = nullptr;
      for (const auto *cls : hirModule->getClasses()) {
        if (cls->getName() == cName) {
          matchedCls = cls;
          break;
        }
      }

      const hir::HIRType *underlying = stripped;
      if (matchedCls) {
        maxAlign = matchedCls->getAlignment();
        underlying = matchedCls->getType();
      }

      if (auto *st = llvm::dyn_cast_or_null<hir::StructType>(underlying)) {
        for (const auto *fieldTy : st->getFields()) {
          uint64_t fieldAlign = self(self, fieldTy);
          if (fieldAlign > maxAlign)
            maxAlign = fieldAlign;
        }
      } else if (auto *ut =
                     llvm::dyn_cast_or_null<hir::UnionType>(underlying)) {
        for (const auto *fieldTy : ut->getFields()) {
          uint64_t fieldAlign = self(self, fieldTy);
          if (fieldAlign > maxAlign)
            maxAlign = fieldAlign;
        }
      } else if (auto *arr =
                     llvm::dyn_cast_or_null<hir::ArrayType>(underlying)) {
        maxAlign = std::max(maxAlign, self(self, arr->getElementType()));
      }

      return maxAlign;
    };

    // Helper: Safely calculate Size via dynamic GEP trick
    auto emitSizeOf = [&](const hir::HIRType *ty) -> MIRValue * {
      auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
      MIRValue *baseAlloc =
          builder->createAlloca(ty, "sizeof.base", expr.getLoc());
      MIRValue *baseInt = builder->insert(
          std::make_unique<CastInst>(Opcode::PtrToInt, baseAlloc, usizeTy,
                                     "sizeof.base.int", expr.getLoc()));
      MIRValue *gep =
          builder->createGEP(baseAlloc, std::vector<MIRValue *>{one}, ty,
                             "sizeof.gep", expr.getLoc());
      MIRValue *gepInt = builder->insert(std::make_unique<CastInst>(
          Opcode::PtrToInt, gep, usizeTy, "sizeof.gep.int", expr.getLoc()));
      return builder->createSub(gepInt, baseInt, "sizeof.val", expr.getLoc());
    };

    // Helper: Safely calculate Alignment via dynamic GEP trick or explicit
    // metadata
    auto emitAlignOf = [&](const hir::HIRType *ty) -> MIRValue * {
      uint64_t customAlign = getMaxAlignment(getMaxAlignment, ty);
      if (customAlign > 0) {
        return mirModule->getOrInsertConstant<ConstantInt>(customAlign,
                                                           usizeTy);
      }

      std::string alignName = ty->toString();
      std::replace(alignName.begin(), alignName.end(), ' ', '_');
      std::replace(alignName.begin(), alignName.end(), '*', 'p');
      std::replace(alignName.begin(), alignName.end(), '<', '_');
      std::replace(alignName.begin(), alignName.end(), '>', '_');

      auto *structTy = const_cast<hir::HIRModule *>(hirModule)->getStructType(
          "__moksha_alignof_" + alignName, {i8Ty, ty});

      auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
      auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);

      MIRValue *baseAlloc =
          builder->createAlloca(structTy, "alignof.base", expr.getLoc());
      MIRValue *baseInt = builder->insert(
          std::make_unique<CastInst>(Opcode::PtrToInt, baseAlloc, usizeTy,
                                     "alignof.base.int", expr.getLoc()));

      MIRValue *gep =
          builder->createGEP(baseAlloc, std::vector<MIRValue *>{zero, one},
                             structTy, "alignof.gep", expr.getLoc());
      MIRValue *gepInt = builder->insert(std::make_unique<CastInst>(
          Opcode::PtrToInt, gep, usizeTy, "alignof.gep.int", expr.getLoc()));

      return builder->createSub(gepInt, baseInt, "alignof.val", expr.getLoc());
    };

    // Helper: Emulate 'select' using control flow and a Phi node
    auto emitSelect = [&](MIRValue *cond, MIRValue *trueVal, MIRValue *falseVal,
                          const hir::HIRType *resTy,
                          const std::string &name) -> MIRValue * {
      MIRBlock *currentBlock = builder->getInsertBlock();
      MIRBlock *trueBlock = newBlock(name + ".true");
      MIRBlock *mergeBlock = newBlock(name + ".merge");

      builder->createCondBr(cond, trueBlock, mergeBlock);

      builder->setInsertPoint(trueBlock);
      builder->createBr(mergeBlock);

      builder->setInsertPoint(mergeBlock);
      auto *phi = builder->createPhi(resTy, name, expr.getLoc());
      phi->addIncoming(trueVal, trueBlock);
      phi->addIncoming(falseVal, currentBlock);
      return phi;
    };

    // 2. Resolve targetType to Underlying Type
    const hir::HIRType *actualTy = targetTy;
    std::string className = targetTy->toString();
    while (!className.empty() && (className[0] == '*' || className[0] == '&' ||
                                  className[0] == ' ')) {
      className = className.substr(1);
    }
    if (className.find("struct.") == 0)
      className = className.substr(7);
    if (className.find("class.") == 0)
      className = className.substr(6);
    if (className.find("union.") == 0)
      className = className.substr(6);

    for (const auto *cls : hirModule->getClasses()) {
      if (cls->getName() == className) {
        actualTy = cls->getType();
        break;
      }
    }

    // 3. Union Calculation Branch
    if (auto *ut = llvm::dyn_cast_or_null<hir::UnionType>(actualTy)) {
      MIRValue *maxSize =
          mirModule->getOrInsertConstant<ConstantInt>(0, usizeTy);
      MIRValue *maxAlign =
          mirModule->getOrInsertConstant<ConstantInt>(1, usizeTy);

      for (const auto *fieldTy : ut->getFields()) {
        MIRValue *fieldSize = emitSizeOf(fieldTy);

        MIRValue *cmpSize =
            builder->createICmp(CompareInst::Predicate::UGT, fieldSize, maxSize,
                                boolTy, "union.size.cmp", expr.getLoc());
        maxSize =
            emitSelect(cmpSize, fieldSize, maxSize, usizeTy, "union.size.max");

        MIRValue *fieldAlign = emitAlignOf(fieldTy);
        MIRValue *cmpAlign = builder->createICmp(
            CompareInst::Predicate::UGT, fieldAlign, maxAlign, boolTy,
            "union.align.cmp", expr.getLoc());
        maxAlign = emitSelect(cmpAlign, fieldAlign, maxAlign, usizeTy,
                              "union.align.max");
      }

      MIRValue *oneUsize =
          mirModule->getOrInsertConstant<ConstantInt>(1, usizeTy);
      MIRValue *alignMinusOne =
          builder->createSub(maxAlign, oneUsize, "align.m1", expr.getLoc());
      MIRValue *added =
          builder->createAdd(maxSize, alignMinusOne, "size.add", expr.getLoc());

      MIRValue *allOnes =
          mirModule->getOrInsertConstant<ConstantInt>(~0ULL, usizeTy);
      MIRValue *invAlign = builder->insert(std::make_unique<BinaryInst>(
          Opcode::Xor, alignMinusOne, allOnes, "align.inv", expr.getLoc()));

      lastExprValue = builder->insert(std::make_unique<BinaryInst>(
          Opcode::And, added, invAlign, "union.padded.size", expr.getLoc()));
    } else {
      // 4. Standard Primitive/Struct Branch
      MIRValue *rawSize = emitSizeOf(targetTy);

      // Apply explicit alignment padding if the struct has an align(N)
      // attribute
      uint64_t explicitAlign = getMaxAlignment(getMaxAlignment, targetTy);

      if (explicitAlign > 0) {
        MIRValue *alignVal =
            mirModule->getOrInsertConstant<ConstantInt>(explicitAlign, usizeTy);
        MIRValue *oneUsize =
            mirModule->getOrInsertConstant<ConstantInt>(1, usizeTy);
        MIRValue *alignMinusOne =
            builder->createSub(alignVal, oneUsize, "align.m1", expr.getLoc());
        MIRValue *added = builder->createAdd(rawSize, alignMinusOne, "size.add",
                                             expr.getLoc());

        MIRValue *allOnes =
            mirModule->getOrInsertConstant<ConstantInt>(~0ULL, usizeTy);
        MIRValue *invAlign = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Xor, alignMinusOne, allOnes, "align.inv", expr.getLoc()));

        lastExprValue = builder->insert(std::make_unique<BinaryInst>(
            Opcode::And, added, invAlign, "padded.size", expr.getLoc()));
      } else {
        lastExprValue = rawSize;
      }
    }
  }

  void visitSharedExpr(const hir::HIRSharedExpr &expr) override {
    expr.getExpr()->accept(*this);
    MIRValue *operand = lastExprValue;
    if (!operand)
      return;

    if (operand->getType()->getKind() == hir::TypeKind::Pointer) {
      lastExprValue = builder->createBitCast(operand, expr.getType(),
                                             "shared.alias", expr.getLoc());
    } else {
      lastExprValue = boxValue(operand, expr.getExpr()->getType(),
                               expr.getType(), expr.getLoc());
    }

    applyBorrowKind(lastExprValue, expr.getType());
  }

  void visitAwaitExpr(const hir::HIRAwaitExpr &expr) override {
    visit(expr.getExpr());
    MIRValue *promiseVal = lastExprValue;
    if (!promiseVal)
      return;

    if (auto *ptrTy =
            llvm::dyn_cast_or_null<hir::PointerType>(promiseVal->getType())) {
      if (ptrTy->getPointee()->getKind() == hir::TypeKind::Promise) {
        promiseVal =
            builder->createLoad(promiseVal, "await.prom.load", expr.getLoc());
      }
    }

    const hir::HIRType *retTy = expr.getType();
    if (!retTy) {
      retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    lastExprValue = builder->insert(std::make_unique<AwaitInst>(
        promiseVal, retTy, "await.result", expr.getLoc()));
  }

  void visitSuperExpr(const hir::HIRSuperExpr &expr) override {
    if (symbolMap.count("this")) {
      auto *ptr = symbolMap["this"];
      lastExprValue = builder->createLoad(ptr, "super.this.val", expr.getLoc());
    } else if (currFunc && !currFunc->getRawArguments().empty()) {
      lastExprValue = currFunc->getRawArguments()[0];
    } else {
      lastExprValue = nullptr;
    }
  }

  void visitDerefExpr(const hir::HIRDerefExpr &expr) override {
    bool savedLValueContext = isLValueContext;
    isLValueContext = false;
    visit(expr.getPointer());
    isLValueContext = savedLValueContext;

    MIRValue *ptrVal = lastExprValue;
    if (!ptrVal)
      return;

    if (isLValueContext) {
      lastExprValue = builder->createBitCast(
          ptrVal, ptrVal->getType(), "deref.lvalue.cast", expr.getLoc());
      return;
    }

    if (isWeakMemory(ptrVal->getType())) {
      lastExprValue = builder->createLoadWeak(ptrVal, expr.getType(),
                                              "deref.weak", expr.getLoc());
    } else {
      auto *loadInst = builder->createLoad(ptrVal, "deref.val", expr.getLoc());
      if (isVolatilePointer(ptrVal))
        loadInst->setVolatile(true);
      lastExprValue = loadInst;
    }
  }

  void visitAddressOfExpr(const hir::HIRAddressOfExpr &expr) override {
    const hir::HIRExpr *operand = expr.getOperand();
    while (auto *castExpr = llvm::dyn_cast_or_null<hir::HIRCastExpr>(operand)) {
      operand = castExpr->getExpr();
    }

    MIRValue *ptr = nullptr;
    if (auto *ident = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(operand)) {
      std::string name = ident->getName();
      if (symbolMap.count(name)) {
        ptr = symbolMap[name];
      } else if (auto *global = mirModule->getGlobal(name)) {
        ptr = global;
      }
    } else {
      ptr = evaluateAsLValue(operand);
    }

    if (ptr) {
      const hir::HIRType *expectedTy = expr.getType();
      if (expectedTy && ptr->getType() != expectedTy) {
        ptr = builder->createBitCast(ptr, expectedTy, "", expr.getLoc());
      }
    }
    lastExprValue = ptr;
  }

  void
  visitTemplateStringExpr(const hir::HIRTemplateStringExpr &expr) override {
    auto *stringTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();
    auto *i32Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);

    std::vector<MIRValue *> callArgs;
    size_t numParts = expr.getParts().size();
    callArgs.push_back(
        mirModule->getOrInsertConstant<ConstantInt>(numParts, i32Ty));

    for (const auto &part : expr.getParts()) {
      visit(part.get());
      MIRValue *val = lastExprValue;
      MIRValue *strVal = coerceToString(val, expr.getLoc());
      builder->insert(std::make_unique<ARCInst>(Opcode::Retain, strVal, nullptr,
                                                expr.getLoc()));
      callArgs.push_back(strVal);
    }

    std::string joinName = "__moksha_template_join_strs";
    MIRFunction *joinFunc = mirModule->getFunction(joinName);
    if (!joinFunc) {
      auto fn =
          std::make_unique<MIRFunction>(stringTy, joinName, Linkage::External);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 0));
      fn->setVariadic(true);
      joinFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    lastExprValue = builder->createCall(joinFunc, callArgs, stringTy,
                                        "interp.str", true, expr.getLoc());
  }

  void visitInputExpr(const hir::HIRInputExpr &expr) override {
    std::string funcName = "__moksha_input";
    MIRFunction *inputFunc = mirModule->getFunction(funcName);
    auto *strTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();

    auto *i8Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
    auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        i8Ty, hir::Ownership::None);

    if (!inputFunc) {
      auto fn =
          std::make_unique<MIRFunction>(i8PtrTy, funcName, Linkage::External);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), strTy, 0));
      inputFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    std::vector<MIRValue *> args;
    visit(expr.getPrompt());
    args.push_back(lastExprValue);

    MIRValue *rawInputRes = builder->createCall(
        inputFunc, std::move(args), i8PtrTy, "input.raw", false, expr.getLoc());

    std::string allocName = "__moksha_cstr_to_string";
    ensureBuiltinMIR(allocName);
    MIRFunction *cstrToStrFunc = mirModule->getFunction(allocName);
    if (!cstrToStrFunc) {
      auto fn =
          std::make_unique<MIRFunction>(strTy, allocName, Linkage::External);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i8PtrTy, 0));
      cstrToStrFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    lastExprValue = builder->createCall(cstrToStrFunc, {rawInputRes}, strTy,
                                        "input.res", false, expr.getLoc());
  }

  void visitArrayLiteral(const hir::HIRArrayLiteral &expr) override {
    const hir::HIRType *rawArrayTy = expr.getType();

    if (expectedLambdaReturnType && expr.getElements().empty()) {
      auto kind = expectedLambdaReturnType->getKind();
      if (kind == hir::TypeKind::Slice || kind == hir::TypeKind::Array ||
          kind == hir::TypeKind::Nullable || kind == hir::TypeKind::Any) {
        rawArrayTy = expectedLambdaReturnType;
      }
    }

    const hir::HIRType *rawElemTy = nullptr;

    const hir::HIRType *checkTy = stripMemoryModifiers(rawArrayTy);
    if (auto *nullTy = llvm::dyn_cast_or_null<hir::HIRNullableType>(checkTy)) {
      checkTy = nullTy->getInner();
    }
    checkTy = stripMemoryModifiers(checkTy);

    bool isSlice = false;
    if (auto *sliceTyInfo = llvm::dyn_cast_or_null<hir::SliceType>(checkTy)) {
      rawElemTy = sliceTyInfo->getElementType();
      isSlice = true;
    } else if (auto *arrTyInfo =
                   llvm::dyn_cast_or_null<hir::ArrayType>(checkTy)) {
      rawElemTy = arrTyInfo->getElementType();
    } else if (!expr.getElements().empty()) {
      rawElemTy = expr.getElements()[0]->getType();
    } else {
      rawElemTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();
    }

    const hir::HIRType *elemTy = rawElemTy;

    if (!elemTy) {
      lastExprValue =
          mirModule->getOrInsertConstant<ConstantNull>(resolveType(rawArrayTy));
      return;
    }

    auto *i32Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);

    MIRValue *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
    MIRValue *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);

    const hir::HIRType *physicalElemTy = elemTy;
    if (!physicalElemTy) {
      physicalElemTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
          hir::Ownership::None);
    }

    auto *i64Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
    MIRValue *sizeofI64 = nullptr;
    MIRValue *sizeofInt = nullptr;

    MIRValue *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
        const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            physicalElemTy, hir::Ownership::None));
    MIRValue *sizeGep = builder->createGEP(nullPtr, {one}, physicalElemTy,
                                           "sizeof.gep", expr.getLoc());
    sizeofInt = builder->insert(std::make_unique<CastInst>(
        Opcode::PtrToInt, sizeGep, i32Ty, "sizeof.int", expr.getLoc()));
    sizeofI64 = builder->insert(std::make_unique<CastInst>(
        Opcode::ZExt, sizeofInt, i64Ty, "sizeof.i64", expr.getLoc()));

    struct EvaluatedElement {
      bool isSpread;
      MIRValue *value;
      MIRValue *length;
      const hir::HIRType *type;
    };
    std::vector<EvaluatedElement> evaluatedElements;

    MIRValue *totalNumElementsVal = zero;

    for (size_t i = 0; i < expr.getElements().size(); ++i) {
      auto *elementExpr = expr.getElements()[i].get();

      if (auto *spread =
              llvm::dyn_cast_or_null<hir::HIRSpreadExpr>(elementExpr)) {
        visit(spread->getIterable());
        MIRValue *sourceVal = lastExprValue;
        const hir::HIRType *sourceTy = spread->getIterable()->getType();
        MIRValue *spreadLenVal = nullptr;

        if (auto *arrTyInfo =
                llvm::dyn_cast_or_null<hir::ArrayType>(sourceTy)) {
          spreadLenVal = mirModule->getOrInsertConstant<ConstantInt>(
              arrTyInfo->getSize(), i32Ty);

          if (isSlice) {
            MIRValue *spreadLenI64 = builder->insert(
                std::make_unique<CastInst>(Opcode::ZExt, spreadLenVal, i64Ty,
                                           "spread.len.i64", expr.getLoc()));

            ensureBuiltinMIR("moksha_rt_array_alloc");
            MIRFunction *allocFunc =
                mirModule->getFunction("moksha_rt_array_alloc");
            if (!allocFunc) {
              auto fn = std::make_unique<MIRFunction>(
                  voidPtrTy, "moksha_rt_array_alloc", Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), i64Ty, 0));
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), i64Ty, 1));
              allocFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }
            MIRValue *tempSlice = builder->createCall(
                allocFunc, {sizeofI64, spreadLenI64}, voidPtrTy,
                "spread.temp.slice", false, expr.getLoc());

            ensureBuiltinMIR("moksha_rt_array_resize");
            MIRFunction *resizeFunc =
                mirModule->getFunction("moksha_rt_array_resize");
            if (!resizeFunc) {
              auto fn = std::make_unique<MIRFunction>(
                  voidTy, "moksha_rt_array_resize", Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), i32Ty, 1));
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
              resizeFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }
            builder->createCall(resizeFunc,
                                {tempSlice, spreadLenVal, sizeofI64}, voidTy,
                                "", false, expr.getLoc());

            ensureBuiltinMIR("moksha_rt_array_data");
            MIRFunction *dataFunc =
                mirModule->getFunction("moksha_rt_array_data");
            if (!dataFunc) {
              auto fn = std::make_unique<MIRFunction>(
                  voidPtrTy, "moksha_rt_array_data", Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              dataFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }
            MIRValue *destData =
                builder->createCall(dataFunc, {tempSlice}, voidPtrTy,
                                    "spread.dest.data", false, expr.getLoc());

            ensureBuiltinMIR("__moksha_array_copy");
            MIRFunction *memcpyFunc =
                mirModule->getFunction("__moksha_array_copy");
            if (!memcpyFunc) {
              auto fn = std::make_unique<MIRFunction>(
                  voidTy, "__moksha_array_copy", Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
              memcpyFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }
            MIRValue *srcVoidPtr = builder->createBitCast(
                sourceVal, voidPtrTy, "spread.src.void", expr.getLoc());
            MIRValue *bytesToCopy =
                builder->insert(std::make_unique<BinaryInst>(
                    Opcode::Mul, sizeofI64, spreadLenI64, "spread.bytes",
                    expr.getLoc()));

            builder->createCall(memcpyFunc, {destData, srcVoidPtr, bytesToCopy},
                                voidTy, "", false, expr.getLoc());

            sourceVal = tempSlice;
          }

        } else if (sourceTy->getKind() == hir::TypeKind::Slice) {
          ensureBuiltinMIR("moksha_rt_array_length");
          MIRFunction *lenFunc =
              mirModule->getFunction("moksha_rt_array_length");
          if (!lenFunc) {
            auto fn = std::make_unique<MIRFunction>(
                i32Ty, "moksha_rt_array_length", Linkage::External);
            fn->addArgument(
                std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
            lenFunc = fn.get();
            mirModule->addFunction(std::move(fn));
          }
          MIRValue *voidCol = builder->createBitCast(
              sourceVal, voidPtrTy, "spread.col.cast", expr.getLoc());
          spreadLenVal =
              builder->createCall(lenFunc, {voidCol}, i32Ty,
                                  "slice.len.extract", false, expr.getLoc());
        } else {
          spreadLenVal = one;
        }

        evaluatedElements.push_back({true, sourceVal, spreadLenVal, sourceTy});
        totalNumElementsVal = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Add, totalNumElementsVal, spreadLenVal, "total.add",
            expr.getLoc()));
      } else {
        visit(elementExpr);
        MIRValue *elemVal = lastExprValue;

        if (elemVal && elemVal->getType() &&
            elemVal->getType()->getKind() == hir::TypeKind::Pointer) {
          auto *ptrTy =
              static_cast<const hir::PointerType *>(elemVal->getType());
          if (ptrTy->getPointee()->getKind() == hir::TypeKind::Array) {
            elemVal =
                builder->createLoad(elemVal, "nested.arr.load", expr.getLoc());
          }
        }

        evaluatedElements.push_back({false, elemVal, one, elemTy});
        totalNumElementsVal = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Add, totalNumElementsVal, one, "total.add", expr.getLoc()));
      }
    }

    if (isSlice) {
      auto *i64Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
      MIRValue *capI64 = builder->insert(std::make_unique<CastInst>(
          Opcode::ZExt, totalNumElementsVal, i64Ty, "cap.i64", expr.getLoc()));
      ensureBuiltinMIR("moksha_rt_array_alloc");
      MIRFunction *allocFunc = mirModule->getFunction("moksha_rt_array_alloc");
      if (!allocFunc) {
        auto fn = std::make_unique<MIRFunction>(
            voidPtrTy, "moksha_rt_array_alloc", Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 1));
        allocFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      MIRValue *slicePtr =
          builder->createCall(allocFunc, {sizeofI64, capI64}, voidPtrTy,
                              "slice.alloc", false, expr.getLoc());
      ensureBuiltinMIR("moksha_rt_array_push");
      MIRFunction *pushFunc = mirModule->getFunction("moksha_rt_array_push");
      if (!pushFunc) {
        auto fn = std::make_unique<MIRFunction>(voidTy, "moksha_rt_array_push",
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
        pushFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      ensureBuiltinMIR("moksha_rt_array_extend");
      MIRFunction *extendFunc =
          mirModule->getFunction("moksha_rt_array_extend");
      if (!extendFunc) {
        auto fn = std::make_unique<MIRFunction>(
            voidTy, "moksha_rt_array_extend", Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
        extendFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      for (const auto &evalEl : evaluatedElements) {
        if (evalEl.isSpread) {
          MIRValue *srcVoidPtr = evalEl.value;
          if (srcVoidPtr->getType() != voidPtrTy)
            srcVoidPtr = builder->createBitCast(
                srcVoidPtr, voidPtrTy, "spread.src.cast", expr.getLoc());
          builder->createCall(extendFunc, {slicePtr, srcVoidPtr, sizeofI64},
                              voidTy, "", false, expr.getLoc());
          if (evalEl.type->getKind() == hir::TypeKind::Array) {
            builder->insert(std::make_unique<ARCInst>(
                Opcode::Release, srcVoidPtr, nullptr, expr.getLoc()));
          }
        } else {
          MIRValue *elemVal = evalEl.value;
          if (elemVal->getType() != elemTy) {
            if (llvm::dyn_cast_or_null<ConstantNull>(elemVal)) {
              elemVal = mirModule->getOrInsertConstant<ConstantNull>(elemTy);
            } else if (elemTy->getKind() == hir::TypeKind::Any ||
                       (elemTy->getKind() == hir::TypeKind::Pointer &&
                        (static_cast<const hir::PointerType *>(elemTy)
                                 ->getOwnership() == hir::Ownership::Shared ||
                         static_cast<const hir::PointerType *>(elemTy)
                                 ->getOwnership() == hir::Ownership::Owned))) {
              elemVal =
                  boxValue(elemVal, elemVal->getType(), elemTy, expr.getLoc());
            } else {
              elemVal = builder->createBitCast(elemVal, elemTy, "elem.cast",
                                               expr.getLoc());
            }
          }

          MIRValue *valPtr =
              builder->createAlloca(elemTy, "push.val.spill", expr.getLoc());
          builder->insert(
              std::make_unique<StoreInst>(elemVal, valPtr, expr.getLoc()));
          MIRValue *valVoidPtr = builder->createBitCast(
              valPtr, voidPtrTy, "push.val.void", expr.getLoc());
          builder->createCall(pushFunc, {slicePtr, valVoidPtr, sizeofI64},
                              voidTy, "", false, expr.getLoc());
        }
      }

      if (slicePtr->getType() != rawArrayTy) {
        lastExprValue = builder->createBitCast(slicePtr, rawArrayTy,
                                               "slice.cast", expr.getLoc());
      } else {
        lastExprValue = slicePtr;
      }
      applyBorrowKind(lastExprValue, rawArrayTy);
      return;
    }

    MIRValue *rawAlloc = nullptr;
    MIRValue *fullArrayPtr = nullptr;
    MIRValue *arrayElemPtr = nullptr;
    const hir::HIRType *actualArrayTy = nullptr;

    uint64_t fixedSize = expr.getElements().size();
    actualArrayTy = const_cast<hir::HIRModule *>(hirModule)->getArrayType(
        elemTy, fixedSize);

    auto *actualArrayPtrTy =
        const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            actualArrayTy, hir::Ownership::None);

    rawAlloc = builder->insert(std::make_unique<AllocaInst>(
        actualArrayPtrTy, actualArrayTy, "array.raw.stack", expr.getLoc(), 0));

    fullArrayPtr = rawAlloc;
    auto *elemPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        elemTy, hir::Ownership::None);
    arrayElemPtr = builder->createBitCast(fullArrayPtr, elemPtrTy,
                                          "array.elem.ptr", expr.getLoc());

    MIRValue *dynIndex = zero;
    std::string memcpyName = "__moksha_array_copy";
    ensureBuiltinMIR(memcpyName);
    MIRFunction *memcpyFunc = mirModule->getFunction(memcpyName);
    if (!memcpyFunc) {
      auto fn =
          std::make_unique<MIRFunction>(voidTy, memcpyName, Linkage::External);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
      auto *i64Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
      memcpyFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }
    const hir::HIRType *exactVoidPtrTy =
        memcpyFunc->getRawArguments()[0]->getType();

    for (const auto &evalEl : evaluatedElements) {
      if (evalEl.isSpread) {
        MIRValue *destElemPtr = builder->createGEP(
            arrayElemPtr, {dynIndex}, elemTy, "spread.dest.ptr", expr.getLoc());
        MIRValue *destVoidPtr = builder->createBitCast(
            destElemPtr, exactVoidPtrTy, "dest.void", expr.getLoc());

        MIRValue *srcDataPtr = nullptr;
        if (evalEl.type->getKind() == hir::TypeKind::Slice) {
          auto *ePtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  elemTy, hir::Ownership::None);

          ensureBuiltinMIR("moksha_rt_array_data");
          MIRFunction *dataFunc =
              mirModule->getFunction("moksha_rt_array_data");
          if (!dataFunc) {
            auto fn = std::make_unique<MIRFunction>(
                voidPtrTy, "moksha_rt_array_data", Linkage::External);
            fn->addArgument(
                std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
            dataFunc = fn.get();
            mirModule->addFunction(std::move(fn));
          }
          MIRValue *voidCol = builder->createBitCast(
              evalEl.value, voidPtrTy, "spread.col.cast", expr.getLoc());
          MIRValue *rawData =
              builder->createCall(dataFunc, {voidCol}, voidPtrTy,
                                  "spread.data.raw", false, expr.getLoc());
          srcDataPtr = builder->createBitCast(
              rawData, ePtrTy, "spread.slice.ptr", expr.getLoc());
        } else {
          srcDataPtr = evalEl.value;
        }

        MIRValue *srcVoidPtr = builder->createBitCast(
            srcDataPtr, exactVoidPtrTy, "src.void", expr.getLoc());
        MIRValue *evalLenI64 = builder->insert(std::make_unique<CastInst>(
            Opcode::ZExt, evalEl.length, i64Ty, "eval.len.i64", expr.getLoc()));
        MIRValue *bytesToCopy = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Mul, sizeofI64, evalLenI64, "spread.bytes", expr.getLoc()));

        builder->createCall(memcpyFunc, {destVoidPtr, srcVoidPtr, bytesToCopy},
                            voidTy, "", false, expr.getLoc());
        dynIndex = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Add, dynIndex, evalEl.length, "idx.next", expr.getLoc()));
      } else {
        MIRValue *elemVal = evalEl.value;
        if (elemVal->getType() != elemTy) {
          if (llvm::dyn_cast_or_null<ConstantNull>(elemVal)) {
            elemVal = mirModule->getOrInsertConstant<ConstantNull>(elemTy);
          } else if (elemTy->getKind() == hir::TypeKind::Any ||
                     (elemTy->getKind() == hir::TypeKind::Pointer &&
                      (static_cast<const hir::PointerType *>(elemTy)
                               ->getOwnership() == hir::Ownership::Shared ||
                       static_cast<const hir::PointerType *>(elemTy)
                               ->getOwnership() == hir::Ownership::Owned))) {
            elemVal =
                boxValue(elemVal, elemVal->getType(), elemTy, expr.getLoc());
          } else {
            elemVal = builder->createBitCast(elemVal, elemTy, "elem.cast",
                                             expr.getLoc());
          }
        }

        MIRValue *elemPtr = builder->createGEP(arrayElemPtr, {dynIndex}, elemTy,
                                               "elem.ptr", expr.getLoc());
        builder->insert(
            std::make_unique<StoreInst>(elemVal, elemPtr, expr.getLoc()));
        dynIndex = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Add, dynIndex, one, "idx.next", expr.getLoc()));
      }
    }

    lastExprValue = fullArrayPtr;
  }

  void visitMapLiteral(const hir::HIRMapLiteral &expr) override {
    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);
    auto *anyTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();
    std::string newName = "moksha_rt_map_new";
    ensureBuiltinMIR(newName);
    MIRFunction *mapNew = mirModule->getFunction(newName);
    if (!mapNew) {
      auto fn =
          std::make_unique<MIRFunction>(voidPtrTy, newName, Linkage::External);
      mapNew = fn.get();
      mirModule->addFunction(std::move(fn));
    }
    MIRValue *mapVal = builder->createCall(mapNew, {}, voidPtrTy, "map.new",
                                           false, expr.getLoc());

    const hir::HIRType *abiAnyTy = getABICoercedType(anyTy, true);
    std::string insertName = "__moksha_map_insert";
    ensureBuiltinMIR(insertName);
    MIRFunction *mapInsert = mirModule->getFunction(insertName);

    if (!mapInsert) {
      auto fn =
          std::make_unique<MIRFunction>(voidTy, insertName, Linkage::External);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), abiAnyTy, 1));
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), abiAnyTy, 2));
      mapInsert = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    for (auto &pair : expr.getEntries()) {
      visit(pair.first.get());
      MIRValue *k = lastExprValue;
      if (k->getType() != anyTy && k->getType() != abiAnyTy) {
        k = boxValue(k, k->getType(), anyTy, expr.getLoc());
      }
      if (k->getType() == anyTy && abiAnyTy != anyTy) {
        auto *spill = builder->createAlloca(anyTy, "k.spill", expr.getLoc());
        builder->insert(std::make_unique<StoreInst>(k, spill, expr.getLoc()));
        k = spill;
      }

      const hir::HIRType *expectedKeyTy =
          mapInsert->getRawArguments()[1]->getType();
      if (k->getType() != expectedKeyTy) {
        k = builder->createBitCast(k, expectedKeyTy, "k.cast", expr.getLoc());
      }

      visit(pair.second.get());
      MIRValue *v = lastExprValue;
      if (v->getType() != anyTy && v->getType() != abiAnyTy) {
        v = boxValue(v, v->getType(), anyTy, expr.getLoc());
      }
      if (v->getType() == anyTy && abiAnyTy != anyTy) {
        auto *spill = builder->createAlloca(anyTy, "v.spill", expr.getLoc());
        builder->insert(std::make_unique<StoreInst>(v, spill, expr.getLoc()));
        v = spill;
      }

      const hir::HIRType *expectedValTy =
          mapInsert->getRawArguments()[2]->getType();
      if (v->getType() != expectedValTy) {
        v = builder->createBitCast(v, expectedValTy, "v.cast", expr.getLoc());
      }

      builder->createCall(mapInsert, {mapVal, k, v}, voidTy, "", false,
                          expr.getLoc());
    }
    lastExprValue = builder->createBitCast(mapVal, expr.getType(),
                                           "map.cast.final", expr.getLoc());
  }

  void visitIntegerLiteral(const hir::HIRIntegerLiteral &expr) override {
    const hir::HIRType *ty = expr.getType();
    if (!ty)
      ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    lastExprValue =
        mirModule->getOrInsertConstant<ConstantInt>(expr.getValue(), ty);
  }

  void visitFloatLiteral(const hir::HIRFloatLiteral &expr) override {
    const hir::HIRType *ty = expr.getType();
    if (!ty)
      ty = const_cast<hir::HIRModule *>(hirModule)->getFloatType(32);

    if (ty->getKind() == hir::TypeKind::Decimal) {
      lastExprValue = mirModule->getOrInsertConstant<ConstantDecimal>(
          std::to_string(expr.getValue()), ty);
    } else {
      lastExprValue =
          mirModule->getOrInsertConstant<ConstantFloat>(expr.getValue(), ty);
    }
  }

  void visitDecimalLiteral(const hir::HIRDecimalLiteral &expr) override {
    lastExprValue = builder->getModule()->getOrInsertConstant<ConstantDecimal>(
        expr.getValue(), expr.getType());
  }

  void visitBoolLiteral(const hir::HIRBoolLiteral &expr) override {
    const hir::HIRType *ty = expr.getType();
    if (!ty)
      ty = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
    lastExprValue =
        mirModule->getOrInsertConstant<ConstantBool>(expr.getValue(), ty);
  }

  void visitStringLiteral(const hir::HIRStringLiteral &expr) override {
    const hir::HIRType *ty =
        const_cast<hir::HIRModule *>(hirModule)->getStringType();

    auto *i8Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
    auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        i8Ty, hir::Ownership::None);

    std::string unescaped = unescapeString(expr.getValue());
    MIRValue *rawConst =
        mirModule->getOrInsertConstant<ConstantString>(unescaped, i8PtrTy);

    std::string allocName = "__moksha_cstr_to_string";
    ensureBuiltinMIR(allocName);
    MIRFunction *cstrToStrFunc = mirModule->getFunction(allocName);

    if (!cstrToStrFunc) {
      auto fn = std::make_unique<MIRFunction>(ty, allocName, Linkage::External);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i8PtrTy, 0));
      cstrToStrFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }
    lastExprValue = builder->createCall(cstrToStrFunc, {rawConst}, ty,
                                        "str.lit.heap", false, expr.getLoc());
  }

  void visitNullLiteral(const hir::HIRNullLiteral &expr) override {
    lastExprValue =
        mirModule->getOrInsertConstant<ConstantNull>(expr.getType());
  }

  void visitSpreadExpr(const hir::HIRSpreadExpr &expr) override {
    visit(expr.getIterable());
  }

  void visitIdentifierExpr(const hir::HIRIdentifierExpr &expr) override {
    if (!builder->getInsertBlock())
      return;
    std::string name = expr.getName();

    if (symbolMap.count(name)) {
      auto *ptr = symbolMap[name];

      if (isLValueContext) {
        lastExprValue = ptr;
        return;
      }

      const hir::HIRType *loadTy = expr.getType();
      if (auto *ptrTy =
              llvm::dyn_cast_or_null<hir::PointerType>(ptr->getType())) {
        loadTy = ptrTy->getPointee();
      }

      if (isWeakMemory(ptr->getType())) {
        lastExprValue =
            builder->createLoadWeak(ptr, loadTy, name + ".weak", expr.getLoc());
      } else {
        auto *loadInst = builder->createLoad(ptr, name + ".val", expr.getLoc());
        if (isVolatilePointer(ptr))
          loadInst->setVolatile(true);
        lastExprValue = loadInst;
      }
    } else if (MIRGlobal *global = mirModule->getGlobal(name)) {
      MIRValue *ptr = global;
      if (isLValueContext) {
        lastExprValue = ptr;
        return;
      }
      auto *loadInst = builder->createLoad(ptr, name + ".val", expr.getLoc());
      if (global->isVolatile())
        loadInst->setVolatile(true);
      applyBorrowKind(loadInst, expr.getType());
      lastExprValue = loadInst;
    } else if (name == "PI" || name == "E" || name == "TAU" || name == "INF" ||
               name == "NAN") {
      auto *f64Ty = const_cast<hir::HIRModule *>(hirModule)->getFloatType(64);
      double val = 0.0;

      if (name == "PI")
        val = 3.14159265358979323846;
      else if (name == "E")
        val = 2.71828182845904523536;
      else if (name == "TAU")
        val = 6.28318530717958647692;
      else if (name == "INF")
        val = std::numeric_limits<double>::infinity();
      else if (name == "NAN")
        val = std::numeric_limits<double>::quiet_NaN();

      lastExprValue = mirModule->getOrInsertConstant<ConstantFloat>(val, f64Ty);
    } else if (name == "READ" || name == "WRITE" || name == "APPEND" ||
               name == "BINARY" || name == "CREATE" || name == "TRUNCATE") {
      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      int32_t val = 0;

      // Assign bitmask values to the constants
      if (name == "READ")
        val = 1; // 1 << 0
      else if (name == "WRITE")
        val = 2; // 1 << 1
      else if (name == "APPEND")
        val = 4; // 1 << 2
      else if (name == "BINARY")
        val = 8; // 1 << 3
      else if (name == "CREATE")
        val = 16; // 1 << 4
      else if (name == "TRUNCATE")
        val = 32; // 1 << 5

      lastExprValue = mirModule->getOrInsertConstant<ConstantInt>(val, i32Ty);
    } else if (MIRFunction *func = mirModule->getFunction(name)) {
      lastExprValue = func;
    } else {
      llvm::errs()
          << "\n[FATAL MIR ERROR] Undefined reference to '" << name
          << "'.\nThe variable is neither a tracked local nor a global.\n";
      exit(1);
    }

    if (lastExprValue && lastExprValue->getType() && expr.getType()) {
      if (llvm::isa<MIRFunction>(lastExprValue)) {
        return;
      }
      const hir::HIRType *expectedAstTy = expr.getType();
      if (lastExprValue->getType()->getKind() == hir::TypeKind::Pointer &&
          (expectedAstTy->getKind() == hir::TypeKind::Struct ||
           expectedAstTy->getKind() == hir::TypeKind::Any ||
           expectedAstTy->getKind() == hir::TypeKind::Closure ||
           expectedAstTy->getKind() == hir::TypeKind::String ||
           expectedAstTy->getKind() == hir::TypeKind::Map ||
           expectedAstTy->getKind() == hir::TypeKind::Slice)) {
        expectedAstTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            expectedAstTy, hir::Ownership::Shared);
      }

      if (lastExprValue->getType()->toString() != expectedAstTy->toString()) {
        if (expectedAstTy->getKind() == hir::TypeKind::Any ||
            (expectedAstTy->getKind() == hir::TypeKind::Pointer &&
             (static_cast<const hir::PointerType *>(expectedAstTy)
                      ->getOwnership() == hir::Ownership::Shared ||
              static_cast<const hir::PointerType *>(expectedAstTy)
                      ->getOwnership() == hir::Ownership::Owned))) {
          lastExprValue = boxValue(lastExprValue, lastExprValue->getType(),
                                   expectedAstTy, expr.getLoc());
        } else if (lastExprValue->getType()->getKind() == hir::TypeKind::Any) {
          lastExprValue = unboxValue(lastExprValue, lastExprValue->getType(),
                                     expectedAstTy, expr.getLoc());
        } else {
          lastExprValue = builder->createBitCast(lastExprValue, expectedAstTy,
                                                 "opt.cast", expr.getLoc());
        }
      }
    }
  }

  void visitCallExpr(const hir::HIRCallExpr &expr) override {
    MIRValue *callee = nullptr;
    std::string calleeName = "";
    std::vector<MIRValue *> args;
    bool needsZeroPoison = false;

    const hir::HIRType *callRetTy = expr.getType();
    if (!callRetTy)
      callRetTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    const hir::HIRExpr *rawCallee = expr.getCallee();
    while (auto *castExpr =
               llvm::dyn_cast_or_null<hir::HIRCastExpr>(rawCallee)) {
      rawCallee = castExpr->getExpr();
    }

    bool isNamespaceCall = false;
    std::string namespaceFuncName = "";
    if (auto *memberExpr =
            llvm::dyn_cast_or_null<hir::HIRMemberExpr>(rawCallee)) {
      if (auto *ident = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(
              memberExpr->getObject())) {
        std::string baseName = ident->getName();
        if (symbolMap.find(baseName) == symbolMap.end() &&
            !mirModule->getGlobal(baseName)) {
          bool isClass = false;
          for (const auto *cls : hirModule->getClasses()) {
            if (cls->getName() == baseName) {
              isClass = true;
              break;
            }
          }

          if (!isClass) {
            isNamespaceCall = true;
            namespaceFuncName = baseName + "." + memberExpr->getMemberName();
          }
        }
      }
    }

    // Direct Identifier (Casts, Builtins, Normal Functions) & Namespace Calls
    if (isNamespaceCall ||
        llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(rawCallee)) {

      if (isNamespaceCall) {
        calleeName = namespaceFuncName;
      } else {
        calleeName =
            static_cast<const hir::HIRIdentifierExpr *>(rawCallee)->getName();
      }

      if (calleeName == "alignof" && expr.getArgs().size() == 1) {
        const hir::HIRExpr *argExpr = expr.getArgs()[0].get();
        while (auto *castExpr =
                   llvm::dyn_cast_or_null<hir::HIRCastExpr>(argExpr)) {
          argExpr = castExpr->getExpr();
        }
        const hir::HIRType *targetTy = stripMemoryModifiers(argExpr->getType());
        auto *usizeTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, false);

        // Sync recursive alignment resolution from sizeof
        auto getStructAlignment = [&](auto &self,
                                      const hir::HIRType *ty) -> uint64_t {
          if (!ty)
            return 0;
          const hir::HIRType *stripped = stripMemoryModifiers(ty);
          uint64_t maxAlign = 0;

          std::string cName = stripped->toString();
          while (!cName.empty() &&
                 (cName[0] == '*' || cName[0] == '&' || cName[0] == ' ')) {
            cName = cName.substr(1);
          }
          if (cName.find("struct.") == 0)
            cName = cName.substr(7);
          if (cName.find("class.") == 0)
            cName = cName.substr(6);
          if (cName.find("union.") == 0)
            cName = cName.substr(6);

          const hir::HIRClass *matchedCls = nullptr;
          for (const auto *cls : hirModule->getClasses()) {
            if (cls->getName() == cName) {
              matchedCls = cls;
              break;
            }
          }

          const hir::HIRType *underlying = stripped;
          if (matchedCls) {
            maxAlign = matchedCls->getAlignment();
            underlying = matchedCls->getType();
          }

          if (auto *st = llvm::dyn_cast_or_null<hir::StructType>(underlying)) {
            for (const auto *fieldTy : st->getFields()) {
              uint64_t fieldAlign = self(self, fieldTy);
              if (fieldAlign > maxAlign)
                maxAlign = fieldAlign;
            }
          } else if (auto *ut =
                         llvm::dyn_cast_or_null<hir::UnionType>(underlying)) {
            for (const auto *fieldTy : ut->getFields()) {
              uint64_t fieldAlign = self(self, fieldTy);
              if (fieldAlign > maxAlign)
                maxAlign = fieldAlign;
            }
          } else if (auto *arr =
                         llvm::dyn_cast_or_null<hir::ArrayType>(underlying)) {
            maxAlign = std::max(maxAlign, self(self, arr->getElementType()));
          }

          return maxAlign;
        };

        uint64_t explicitAlign =
            getStructAlignment(getStructAlignment, targetTy);

        if (explicitAlign > 0) {
          lastExprValue = mirModule->getOrInsertConstant<ConstantInt>(
              explicitAlign, usizeTy);
          return;
        }

        // 3. Fallback to GEP trick for natural ABI alignment
        auto *i8Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);

        std::string alignName = targetTy ? targetTy->toString() : "any";
        std::replace(alignName.begin(), alignName.end(), ' ', '_');
        std::replace(alignName.begin(), alignName.end(), '*', 'p');
        std::replace(alignName.begin(), alignName.end(), '<', '_');
        std::replace(alignName.begin(), alignName.end(), '>', '_');

        auto *structTy = const_cast<hir::HIRModule *>(hirModule)->getStructType(
            "__moksha_alignof_" + alignName, {i8Ty, targetTy});

        auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
        auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);

        MIRValue *baseAlloc =
            builder->createAlloca(structTy, "alignof.base", expr.getLoc());
        MIRValue *baseInt = builder->insert(
            std::make_unique<CastInst>(Opcode::PtrToInt, baseAlloc, usizeTy,
                                       "alignof.base.int", expr.getLoc()));

        MIRValue *gep =
            builder->createGEP(baseAlloc, std::vector<MIRValue *>{zero, one},
                               structTy, "alignof.gep", expr.getLoc());
        MIRValue *gepInt = builder->insert(std::make_unique<CastInst>(
            Opcode::PtrToInt, gep, usizeTy, "alignof.gep.int", expr.getLoc()));

        lastExprValue =
            builder->createSub(gepInt, baseInt, "alignof.val", expr.getLoc());
        return;
      }

      if (calleeName == "offsetof" && expr.getArgs().size() == 2) {
        const hir::HIRExpr *argExpr = expr.getArgs()[0].get();
        while (auto *castExpr =
                   llvm::dyn_cast_or_null<hir::HIRCastExpr>(argExpr)) {
          argExpr = castExpr->getExpr();
        }
        const hir::HIRType *targetTy = stripMemoryModifiers(argExpr->getType());

        std::string fieldName = "";
        if (auto *id = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(
                expr.getArgs()[1].get())) {
          fieldName = id->getName();
        }

        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
        auto *usizeTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, false);

        if (auto *st = llvm::dyn_cast_or_null<hir::StructType>(targetTy)) {
          int idx = st->getFieldIndex(fieldName);
          if (idx >= 0) {
            auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    st, hir::Ownership::None));
            auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
            auto *idxVal =
                mirModule->getOrInsertConstant<ConstantInt>(idx, i32Ty);

            MIRValue *gep = builder->createGEP(
                nullPtr, std::vector<MIRValue *>{zero, idxVal}, st,
                "offsetof.gep", expr.getLoc());
            lastExprValue = builder->insert(std::make_unique<CastInst>(
                Opcode::PtrToInt, gep, usizeTy, "offsetof.int", expr.getLoc()));
            return;
          }
        }

        lastExprValue = mirModule->getOrInsertConstant<ConstantInt>(0, usizeTy);
        return;
      }

      std::string originalName = calleeName;
      ensureBuiltinMIR(originalName);
      for (const auto *hirTarget : hirModule->getFunctions()) {
        if (hirTarget->getName() == originalName) {
          if (hirTarget->isExtern() && !hirTarget->getABI().empty()) {
            std::string abi = hirTarget->getABI();
            if (abi != "stdcall" && abi != "fastcall" && abi != "cdecl" &&
                abi != "C" && abi != "vectorcall" && abi != "sysv64" &&
                abi != "win64") {
              calleeName = abi;
            }
          }
          break;
        }
      }

      // Route calls to the C-Runtime prefixed names
      if (calleeName == "yield" || calleeName == "spawn" ||
          calleeName == "cancel" || calleeName == "select" ||
          calleeName == "timeout" || calleeName == "sleep" ||
          calleeName == "join") {

        if (calleeName == "spawn" && !expr.getArgs().empty()) {
          const hir::HIRType *argTy = expr.getArgs()[0]->getType();
          if (argTy && argTy->getKind() == hir::TypeKind::Promise) {
            calleeName = "moksha_builtin_spawn_promise";
          } else {
            calleeName = "moksha_builtin_spawn";
          }
        } else {
          calleeName = "moksha_builtin_" + calleeName;
        }
      }

      callee = mirModule->getFunction(calleeName);

      auto coerceToPointee = [&](MIRValue *val, MIRValue *ptr) -> MIRValue * {
        const hir::HIRType *expectedTy = nullptr;
        if (auto *pTy =
                llvm::dyn_cast_or_null<hir::PointerType>(ptr->getType())) {
          expectedTy = pTy->getPointee();
        } else if (auto *rTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                       ptr->getType())) {
          expectedTy = rTy->getInner();
        }

        if (expectedTy && val->getType() != expectedTy) {
          if (auto *cInt = llvm::dyn_cast_or_null<ConstantInt>(val)) {
            return mirModule->getOrInsertConstant<ConstantInt>(cInt->getValue(),
                                                               expectedTy);
          } else if (auto *cFloat =
                         llvm::dyn_cast_or_null<ConstantFloat>(val)) {
            return mirModule->getOrInsertConstant<ConstantFloat>(
                cFloat->getValue(), expectedTy);
          }
          return builder->createBitCast(val, expectedTy, "atomic.cast",
                                        expr.getLoc());
        }
        return val;
      };

      if (calleeName == "atomic_add") {
        visit(expr.getArgs()[0].get());
        MIRValue *ptr = lastExprValue;
        visit(expr.getArgs()[1].get());
        MIRValue *val = coerceToPointee(lastExprValue, ptr);

        lastExprValue = builder->insert(std::make_unique<AtomicRMWInst>(
            AtomicOp::Add, ptr, val, MemoryOrder::SeqCst, expr.getLoc()));
        return;
      } else if (calleeName == "atomic_load") {
        visit(expr.getArgs()[0].get());
        MIRValue *ptr = lastExprValue;
        lastExprValue = builder->insert(std::make_unique<AtomicLoadInst>(
            ptr, MemoryOrder::SeqCst, expr.getLoc()));
        return;
      } else if (calleeName == "atomic_store") {
        visit(expr.getArgs()[0].get());
        MIRValue *ptr = lastExprValue;
        visit(expr.getArgs()[1].get());
        MIRValue *val = coerceToPointee(lastExprValue, ptr);

        lastExprValue = builder->insert(std::make_unique<AtomicStoreInst>(
            val, ptr, MemoryOrder::SeqCst, expr.getLoc()));
        return;
      } else if (calleeName == "atomic_cas") {
        visit(expr.getArgs()[0].get());
        MIRValue *ptr = lastExprValue;
        visit(expr.getArgs()[1].get());
        MIRValue *expected = coerceToPointee(lastExprValue, ptr);
        visit(expr.getArgs()[2].get());
        MIRValue *desired = coerceToPointee(lastExprValue, ptr);

        lastExprValue = builder->insert(std::make_unique<AtomicCmpXchgInst>(
            ptr, expected, desired, MemoryOrder::SeqCst, MemoryOrder::SeqCst,
            expr.getLoc()));
        return;
      } else if (calleeName == "atomic_thread_fence") {
        MemoryOrder order = MemoryOrder::SeqCst;
        if (!expr.getArgs().empty()) {
          if (auto *strLit = llvm::dyn_cast_or_null<hir::HIRStringLiteral>(
                  expr.getArgs()[0].get())) {
            if (strLit->getValue() == "acquire")
              order = MemoryOrder::Acquire;
            else if (strLit->getValue() == "release")
              order = MemoryOrder::Release;
          }
        }
        lastExprValue =
            builder->insert(std::make_unique<FenceInst>(order, expr.getLoc()));
        return;
      }

      IntrinsicID intrinID = mirModule->lookupIntrinsic(calleeName);

      if (intrinID != IntrinsicID::None) {
        // Helper to cast atomic operands safely
        auto coerceToPointee = [&](MIRValue *val, MIRValue *ptr) -> MIRValue * {
          const hir::HIRType *expectedTy = nullptr;
          if (auto *pTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(ptr->getType())) {
            expectedTy = pTy->getPointee();
          } else if (auto *rTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                         ptr->getType())) {
            expectedTy = rTy->getInner();
          }

          if (expectedTy && val->getType() != expectedTy) {
            if (auto *cInt = llvm::dyn_cast_or_null<ConstantInt>(val)) {
              return mirModule->getOrInsertConstant<ConstantInt>(
                  cInt->getValue(), expectedTy);
            }
            return builder->createBitCast(val, expectedTy, "atomic.cast",
                                          expr.getLoc());
          }
          return val;
        };

        switch (intrinID) {
        case IntrinsicID::AtomicAdd: {
          visit(expr.getArgs()[0].get());
          MIRValue *ptr = lastExprValue;
          visit(expr.getArgs()[1].get());
          MIRValue *val = coerceToPointee(lastExprValue, ptr);
          lastExprValue = builder->insert(std::make_unique<AtomicRMWInst>(
              AtomicOp::Add, ptr, val, MemoryOrder::SeqCst, expr.getLoc()));
          return;
        }
        case IntrinsicID::AtomicLoad: {
          visit(expr.getArgs()[0].get());
          lastExprValue = builder->insert(std::make_unique<AtomicLoadInst>(
              lastExprValue, MemoryOrder::SeqCst, expr.getLoc()));
          return;
        }
        case IntrinsicID::AtomicStore: {
          visit(expr.getArgs()[0].get());
          MIRValue *ptr = lastExprValue;
          visit(expr.getArgs()[1].get());
          MIRValue *val = coerceToPointee(lastExprValue, ptr);
          lastExprValue = builder->insert(std::make_unique<AtomicStoreInst>(
              val, ptr, MemoryOrder::SeqCst, expr.getLoc()));
          return;
        }
        case IntrinsicID::AtomicCAS: {
          visit(expr.getArgs()[0].get());
          MIRValue *ptr = lastExprValue;
          visit(expr.getArgs()[1].get());
          MIRValue *expected = coerceToPointee(lastExprValue, ptr);
          visit(expr.getArgs()[2].get());
          MIRValue *desired = coerceToPointee(lastExprValue, ptr);
          lastExprValue = builder->insert(std::make_unique<AtomicCmpXchgInst>(
              ptr, expected, desired, MemoryOrder::SeqCst, MemoryOrder::SeqCst,
              expr.getLoc()));
          return;
        }
        case IntrinsicID::AtomicFenceAcquire:
          lastExprValue = builder->insert(
              std::make_unique<FenceInst>(MemoryOrder::Acquire, expr.getLoc()));
          return;
        case IntrinsicID::AtomicFenceRelease:
          lastExprValue = builder->insert(
              std::make_unique<FenceInst>(MemoryOrder::Release, expr.getLoc()));
          return;
        case IntrinsicID::AtomicFenceSeqCst:
          lastExprValue = builder->insert(
              std::make_unique<FenceInst>(MemoryOrder::SeqCst, expr.getLoc()));
          return;
        case IntrinsicID::AtomicThreadFence: {
          MemoryOrder order = MemoryOrder::SeqCst;
          if (!expr.getArgs().empty()) {
            if (auto *strLit = llvm::dyn_cast_or_null<hir::HIRStringLiteral>(
                    expr.getArgs()[0].get())) {
              if (strLit->getValue() == "acquire")
                order = MemoryOrder::Acquire;
              else if (strLit->getValue() == "release")
                order = MemoryOrder::Release;
            }
          }
          lastExprValue = builder->insert(
              std::make_unique<FenceInst>(order, expr.getLoc()));
          return;
        }
        default:
          break;
        }

        // Route Standard Function Call Intrinsics (bswap, clz, etc.)
        callee = mirModule->getFunction(calleeName);
        if (!callee) {
          IntrinsicID actualIntrin = mirModule->lookupIntrinsic(calleeName);
          const hir::HIRType *retTy = nullptr;
          if (actualIntrin == IntrinsicID::Trap) {
            retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
          } else if (actualIntrin == IntrinsicID::Bswap) {
            retTy = expr.getArgs().empty()
                        ? const_cast<hir::HIRModule *>(hirModule)->getIntType(
                              32, false)
                        : expr.getArgs()[0]->getType();
          } else {
            retTy =
                const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
          }

          auto intrin = std::make_unique<MIRFunction>(retTy, calleeName,
                                                      Linkage::External);
          if (actualIntrin != IntrinsicID::Trap) {
            const hir::HIRType *argTy =
                expr.getArgs().empty()
                    ? const_cast<hir::HIRModule *>(hirModule)->getIntType(32,
                                                                          false)
                    : expr.getArgs()[0]->getType();
            intrin->addArgument(
                std::make_unique<MIRArgument>(intrin.get(), argTy, 0));
            if (needsZeroPoison) {
              auto *boolTy =
                  const_cast<hir::HIRModule *>(hirModule)->getBoolType();
              intrin->addArgument(
                  std::make_unique<MIRArgument>(intrin.get(), boolTy, 1));
            }
          }

          callee = intrin.get();
          mirModule->addFunction(std::move(intrin));
        }
      }

      // Functional Cast Interceptor
      const hir::HIRType *targetTy = nullptr;

      // Signed Integers
      if (calleeName == "char")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
      else if (calleeName == "short")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(16, true);
      else if (calleeName == "int")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      else if (calleeName == "long")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
      else if (calleeName == "isize")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true, true);

      // Unsigned Integers
      else if (calleeName == "uchar" || calleeName == "unsigned_char")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(8, false);
      else if (calleeName == "ushort" || calleeName == "unsigned_short")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(16, false);
      else if (calleeName == "uint" || calleeName == "unsigned_int")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, false);
      else if (calleeName == "ulong" || calleeName == "unsigned_long")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, false);
      else if (calleeName == "usize")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getIntType(
            64, false, true);

      // Floating Point & Decimals
      else if (calleeName == "quarter")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(8);
      else if (calleeName == "half")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(16);
      else if (calleeName == "float")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(32);
      else if (calleeName == "double")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(64);
      else if (calleeName == "decimal")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getDecimalType(10, 2);

      // Standard Primitives
      else if (calleeName == "bool")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
      else if (calleeName == "string")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();

      // Execute the Cast
      if (targetTy) {
        if (!expr.getArgs().empty()) {
          visit(expr.getArgs()[0].get());
          MIRValue *valToCast = lastExprValue;
          if (targetTy->getKind() == hir::TypeKind::String) {
            lastExprValue = coerceToString(valToCast, expr.getLoc());
          } else {
            if (valToCast->getType()->getKind() == hir::TypeKind::Any) {
              valToCast = unboxValue(valToCast, valToCast->getType(), targetTy,
                                     expr.getLoc());
            }
            if (valToCast->getType() != targetTy) {
              lastExprValue = coerceValue(valToCast, targetTy, expr.getLoc());
            } else {
              lastExprValue = valToCast;
            }
          }
          return;
        }
      }

      if (calleeName == "bswap16")
        calleeName = "llvm.bswap.i16";
      else if (calleeName == "bswap32")
        calleeName = "llvm.bswap.i32";
      else if (calleeName == "bswap64")
        calleeName = "llvm.bswap.i64";
      else if (calleeName == "ctpop" || calleeName == "popcount")
        calleeName = "llvm.ctpop.i32";
      else if (calleeName == "clz" || calleeName == "ctz") {
        calleeName = (calleeName == "clz") ? "llvm.ctlz.i32" : "llvm.cttz.i32";
        needsZeroPoison = true;
      } else if (calleeName == "trap") {
        calleeName = "llvm.trap";
      }

      // Standard Identifier Resolution
      callee = mirModule->getFunction(calleeName);
      if (!callee && symbolMap.count(calleeName)) {
        MIRValue *ptr = symbolMap[calleeName];
        auto *loadInst =
            builder->createLoad(ptr, calleeName + ".val", expr.getLoc());
        if (isVolatilePointer(ptr)) {
          loadInst->setVolatile(true);
        }
        loadInst->setBorrowKind(mir::BorrowKind::View);
        callee = loadInst;
      } else if (!callee && mirModule->getGlobal(calleeName)) {
        MIRValue *ptr = mirModule->getGlobal(calleeName);
        auto *loadInst =
            builder->createLoad(ptr, calleeName + ".val", expr.getLoc());
        if (isVolatilePointer(ptr)) {
          loadInst->setVolatile(true);
        }
        loadInst->setBorrowKind(mir::BorrowKind::View);
        callee = loadInst;
      }

      // Builtin / Generic Discovery & Type Deduction
      if (!callee && !mirModule->getGlobal(calleeName)) {
        // Intercept Free-Floating Generic Functions
        const hir::HIRFunction *hirTarget = hirModule->getFunction(calleeName);
        bool isGenericFunc = false;

        if (hirTarget) {
          for (const auto &p : hirTarget->getParams()) {
            if (p.type->toString().find("T") != std::string::npos) {
              isGenericFunc = true;
              break;
            }
          }
        }

        if (isGenericFunc) {
          std::vector<const hir::HIRType *> argTys;
          for (const auto &arg : expr.getArgs()) {
            const hir::HIRType *aTy = arg->getType();
            if (!aTy) {
              if (llvm::isa<hir::HIRIntegerLiteral>(arg.get()))
                aTy = const_cast<hir::HIRModule *>(hirModule)->getIntType(32,
                                                                          true);
              else if (llvm::isa<hir::HIRFloatLiteral>(arg.get()))
                aTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(32);
              else if (llvm::isa<hir::HIRBoolLiteral>(arg.get()))
                aTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
              else if (llvm::isa<hir::HIRStringLiteral>(arg.get()))
                aTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();
            }
            argTys.push_back(aTy);
          }

          const hir::HIRType *resolvedRetTy = expr.getType();
          if (!resolvedRetTy ||
              resolvedRetTy->getKind() == hir::TypeKind::Void) {
            resolvedRetTy =
                const_cast<hir::HIRModule *>(hirModule)->getVoidType();
          }

          std::string mangledName = mangleName(calleeName, argTys);
          std::string retStr = resolvedRetTy->toString();
          std::replace(retStr.begin(), retStr.end(), '*', 'p');
          mangledName += "_ret_" + retStr;

          if (instantiatedGenerics.insert(mangledName).second) {
            MonomorphizationTask task;
            task.genericClass = nullptr;
            task.genericFunc = hirTarget;
            task.typeArgs = argTys;
            monoQueue.push(task);
          }

          calleeName = mangledName;
        }

        ensureBuiltinMIR(calleeName);
        callee = mirModule->getFunction(calleeName);

        if (!callee) {
          IntrinsicID actualIntrin = mirModule->lookupIntrinsic(calleeName);
          const hir::HIRType *retTy = expr.getType();

          bool isIntercepted = false;
          if (calleeName == "push" || calleeName == "pop" ||
              calleeName == "length" || calleeName == "at" ||
              calleeName == "is_empty" || calleeName == "copy" ||
              calleeName == "slice" || calleeName == "contains" ||
              calleeName == "index" || calleeName == "fill" ||
              calleeName == "reverse" || calleeName == "sort" ||
              calleeName == "clone" || calleeName == "insert" ||
              calleeName == "remove" || calleeName == "clear" ||
              calleeName == "capacity" || calleeName == "resize" ||
              calleeName == "extend" || calleeName == "has" ||
              calleeName == "substring" || calleeName == "starts_with" ||
              calleeName == "ends_with" || calleeName == "to_upper" ||
              calleeName == "to_lower" || calleeName == "trim" ||
              calleeName == "replace" || calleeName == "split" ||
              calleeName == "is_digit" || calleeName == "is_alpha" ||
              calleeName == "is_whitespace" || calleeName == "join") {
            isIntercepted = true;
          }
          if (calleeName.find("length_") == 0 ||
              calleeName.find("push_") == 0 || calleeName.find("pop_") == 0 ||
              calleeName.find("at_") == 0) {
            isIntercepted = true;
          }

          if (!isIntercepted) {
            if (actualIntrin == IntrinsicID::Trap) {
              retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            } else if (actualIntrin == IntrinsicID::Bswap) {
              retTy = expr.getArgs().empty()
                          ? const_cast<hir::HIRModule *>(hirModule)->getIntType(
                                32, false)
                          : expr.getArgs()[0]->getType();
            } else if (actualIntrin == IntrinsicID::Ctlz ||
                       actualIntrin == IntrinsicID::Cttz ||
                       actualIntrin == IntrinsicID::Ctpop) {
              retTy =
                  const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
            } else if (!retTy || retTy->getKind() == hir::TypeKind::Void) {
              if (!expr.getArgs().empty()) {
                const hir::HIRType *arg0Ty = expr.getArgs()[0]->getType();
                if (auto *ptrTy =
                        llvm::dyn_cast_or_null<hir::PointerType>(arg0Ty)) {
                  retTy = ptrTy->getPointee();
                } else if (auto *arrTy =
                               llvm::dyn_cast_or_null<hir::ArrayType>(arg0Ty)) {
                  retTy = arrTy->getElementType();
                }
              }
              if (!retTy)
                retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            }

            auto externFunc = std::make_unique<MIRFunction>(retTy, calleeName,
                                                            Linkage::External);

            if (calleeName == "print" || calleeName == "println" ||
                calleeName == "__moksha_template_join_strs" ||
                calleeName == "moksha_builtin_join" ||
                calleeName == "moksha_builtin_select") {
              externFunc->setVariadic(true);
            }

            if (actualIntrin != IntrinsicID::Trap) {
              unsigned idx = 0;

              if (calleeName == "print" || calleeName == "println") {
                auto *anyTy =
                    const_cast<hir::HIRModule *>(hirModule)->getAnyType();
                auto *anyPtrTy =
                    const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                        anyTy, hir::Ownership::None);
                externFunc->addArgument(std::make_unique<MIRArgument>(
                    externFunc.get(), anyPtrTy, idx++));
              } else if (!externFunc->isVariadic()) {
                for (const auto &astArg : expr.getArgs()) {
                  const hir::HIRType *argTy = astArg->getType();

                  if (!argTy || argTy->getKind() == hir::TypeKind::Void) {
                    auto *voidTy =
                        const_cast<hir::HIRModule *>(hirModule)->getVoidType();
                    argTy =
                        const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                            voidTy, hir::Ownership::None);
                  }
                  argTy = getABICoercedType(argTy, true);

                  externFunc->addArgument(std::make_unique<MIRArgument>(
                      externFunc.get(), argTy, idx++));
                }
              }
              if (needsZeroPoison) {
                auto *boolTy =
                    const_cast<hir::HIRModule *>(hirModule)->getBoolType();
                externFunc->addArgument(std::make_unique<MIRArgument>(
                    externFunc.get(), boolTy, idx++));
              }
            }

            callee = externFunc.get();
            mirModule->addFunction(std::move(externFunc));
          }
        }
      }
    }
    // Method Call Interception (e.g., io.open() or Matrix.identity())
    else if (auto *memberExpr =
                 llvm::dyn_cast_or_null<hir::HIRMemberExpr>(rawCallee)) {
      MIRValue *baseObj = nullptr;
      std::string className = "";

      // Detect Static Class Method Calls natively
      if (auto *ident = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(
              memberExpr->getObject())) {
        std::string baseName = ident->getName();
        for (const auto *cls : hirModule->getClasses()) {
          if (cls->getName() == baseName) {
            className = baseName;
            break;
          }
        }
      }

      if (className.empty()) {
        baseObj = evaluateAsLValue(memberExpr->getObject());
        // Unwrap Double Pointers (L-Value to R-Value)
        if (baseObj && baseObj->getType() &&
            baseObj->getType()->getKind() == hir::TypeKind::Pointer) {
          auto *ptrTy =
              static_cast<const hir::PointerType *>(baseObj->getType());
          const auto *pointee = ptrTy->getPointee();

          bool isManaged = false;
          if (ptrTy->getOwnership() != hir::Ownership::None &&
              ptrTy->getOwnership() != hir::Ownership::Borrowed) {
            isManaged = true;
          }

          if (pointee &&
              (pointee->getKind() == hir::TypeKind::Pointer ||
               pointee->getKind() == hir::TypeKind::Reference ||
               pointee->getKind() == hir::TypeKind::Nullable || isManaged ||
               pointee->toString().find("shared") != std::string::npos ||
               pointee->toString().find("weak") != std::string::npos ||
               pointee->toString().find("Box<") != std::string::npos ||
               pointee->toString().find("Arc<") != std::string::npos)) {

            if (llvm::isa<AllocaInst>(baseObj) ||
                llvm::isa<GetElementPtrInst>(baseObj) ||
                llvm::isa<MIRGlobal>(baseObj) || llvm::isa<CastInst>(baseObj) ||
                llvm::isa<MIRArgument>(baseObj)) {
              auto *baseLoad =
                  builder->createLoad(baseObj, "base.load", expr.getLoc());
              baseLoad->setBorrowKind(mir::BorrowKind::View);
              baseObj = baseLoad;
            } else if (auto *existingLoad =
                           llvm::dyn_cast_or_null<LoadInst>(baseObj)) {
              existingLoad->setBorrowKind(mir::BorrowKind::View);
            }
          }
        }

        const hir::HIRType *baseTy = baseObj ? baseObj->getType() : nullptr;
        if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(baseTy)) {
          baseTy = ptrTy->getPointee();
        } else if (auto *refTy =
                       llvm::dyn_cast_or_null<hir::ReferenceType>(baseTy)) {
          baseTy = refTy->getInner();
        }

        if (baseTy)
          className = baseTy->toString();
      }

      // Dynamic Generic Sanitizer
      if (!className.empty() && className[0] == '*')
        className = className.substr(1);
      // Strip Nullability from Class Name
      if (!className.empty() && className.back() == '?')
        className.pop_back();
      std::replace(className.begin(), className.end(), '<', '_');
      std::replace(className.begin(), className.end(), '>', '_');
      while (!className.empty() && className.back() == '_')
        className.pop_back();

      // Strip internal prefixes generated by earlier passes
      if (className.find("struct.") == 0)
        className = className.substr(7);
      if (className.find("class.") == 0)
        className = className.substr(6);

      std::string memberName = memberExpr->getMemberName();
      calleeName = className + "." + memberName;

      std::vector<const hir::HIRType *> argTys;
      for (const auto &arg : expr.getArgs()) {
        const hir::HIRType *aTy = arg->getType();
        if (!aTy) {
          if (llvm::isa<hir::HIRIntegerLiteral>(arg.get()))
            aTy = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
          else if (llvm::isa<hir::HIRFloatLiteral>(arg.get()))
            aTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(32);
          else if (llvm::isa<hir::HIRBoolLiteral>(arg.get()))
            aTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
          else if (llvm::isa<hir::HIRStringLiteral>(arg.get()))
            aTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();
        }
        argTys.push_back(aTy);
      }

      const hir::HIRClass *targetCls = nullptr;
      for (const auto *cls : hirModule->getClasses()) {
        if (cls->getName() == className) {
          targetCls = cls;
          break;
        }
      }

      const hir::HIRFunction *bestMatch = nullptr;
      std::string bestMangledName = "";
      if (targetCls) {
        int bestScore = -1;
        for (const auto &method : targetCls->getMethods()) {
          if (method->getName() == memberName) {
            const auto &params = method->getParams();
            if (params.size() == argTys.size()) {
              int score = 0;
              bool valid = true;
              for (size_t i = 0; i < params.size(); ++i) {
                const hir::HIRType *aTy = argTys[i];
                if (!aTy) {
                  valid = false;
                  break;
                }

                if (params[i].type == aTy ||
                    params[i].type->toString() == aTy->toString()) {
                  score += 10;
                } else if (params[i].type->getKind() == aTy->getKind()) {
                  score += 5;
                } else if (params[i].type->getKind() ==
                               hir::TypeKind::Nullable ||
                           params[i].type->toString().back() == '?') {
                  score += 3;
                } else if (params[i].type->getKind() == hir::TypeKind::Any) {
                  score += 1;
                } else {
                  valid = false;
                  break;
                }
              }

              if (valid && score > bestScore) {
                bestScore = score;
                bestMatch = method.get();
              }
            }
          }
        }
      }

      if (bestMatch) {
        std::vector<const hir::HIRType *> paramTys;
        for (const auto &p : bestMatch->getParams()) {
          paramTys.push_back(p.type);
        }
        if (bestMatch->isExtern()) {
          std::string baseClass = className;
          size_t under = baseClass.find('_');
          if (under != std::string::npos) {
            baseClass = baseClass.substr(0, under);
          }
          calleeName = baseClass + "_" + memberName;
          if (baseClass == "Channel") {
            calleeName = "moksha_builtin_" + calleeName;
          }
        } else {
          calleeName = mangleName(className + "." + memberName, paramTys);
          std::string retStr = bestMatch->getReturnType()
                                   ? bestMatch->getReturnType()->toString()
                                   : "void";
          if (!retStr.empty() && retStr.back() == '?')
            retStr.pop_back();
          std::replace(retStr.begin(), retStr.end(), '*', 'p');
          for (char &c : retStr) {
            if (!isalnum(c))
              c = '_';
          }
          calleeName += "_ret_" + retStr;
        }
        callee = mirModule->getFunction(calleeName);
      } else {
        calleeName = mangleName(calleeName, argTys);
        const hir::HIRType *callRetTy = expr.getType();
        std::string retStr = callRetTy ? callRetTy->toString() : "void";
        if (!retStr.empty() && retStr.back() == '?')
          retStr.pop_back();
        std::replace(retStr.begin(), retStr.end(), '*', 'p');
        for (char &c : retStr) {
          if (!isalnum(c))
            c = '_';
        }
        calleeName += "_ret_" + retStr;
        callee = mirModule->getFunction(calleeName);
      }

      if (!callee) {
        const hir::HIRType *retTy = expr.getType();
        if (!retTy || retTy->getKind() == hir::TypeKind::Void) {
          retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        }
        auto externFunc =
            std::make_unique<MIRFunction>(retTy, calleeName, Linkage::External);
        const hir::HIRType *thisTy = nullptr;
        if (baseObj && baseObj->getType()) {
          const hir::HIRType *rawBase = baseObj->getType();
          if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(rawBase)) {
            rawBase = pTy->getPointee();
          } else if (auto *rTy =
                         llvm::dyn_cast_or_null<hir::ReferenceType>(rawBase)) {
            rawBase = rTy->getInner();
          }
          thisTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              rawBase, hir::Ownership::Borrowed);
        } else {
          thisTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        }

        auto thisArg =
            std::make_unique<MIRArgument>(externFunc.get(), thisTy, 0);
        applyBorrowKind(thisArg.get(), thisTy);
        externFunc->addArgument(std::move(thisArg));

        unsigned idx = 1;
        for (const auto &arg : expr.getArgs()) {
          const hir::HIRType *argTy = arg->getType();
          if (!argTy || argTy->getKind() == hir::TypeKind::Void) {
            auto *voidTy =
                const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            argTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);
          }
          externFunc->addArgument(
              std::make_unique<MIRArgument>(externFunc.get(), argTy, idx++));
        }
        callee = externFunc.get();
        mirModule->addFunction(std::move(externFunc));
      }

      for (const auto *cls : hirModule->getClasses()) {
        if (cls->getName() == className) {
          targetCls = cls;
          break;
        }
      }

      const hir::HIRFunction *hirMethod = nullptr;
      if (targetCls) {
        for (const auto &m : targetCls->getMethods()) {
          if (m->getName() == memberExpr->getMemberName()) {
            hirMethod = m.get();
            break;
          }
        }
      }

      MIRFunction *staticCalleeFunc =
          llvm::dyn_cast_or_null<MIRFunction>(callee);
      if (hirMethod &&
          (hirMethod->isVirtualFunc() || hirMethod->isOverrideFunc())) {
        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
        auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
        auto *idxVal = mirModule->getOrInsertConstant<ConstantInt>(
            hirMethod->getVTableIndex(), i32Ty);
        auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
        const hir::HIRType *vtableStructTy = nullptr;
        if (MIRGlobal *vtableGlobal =
                mirModule->getGlobal(className + ".vtable")) {
          vtableStructTy = vtableGlobal->getType();
          while (auto *pTy =
                     llvm::dyn_cast_or_null<hir::PointerType>(vtableStructTy)) {
            vtableStructTy = pTy->getPointee();
          }
        } else {
          vtableStructTy =
              const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        }

        auto *vptrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            vtableStructTy, hir::Ownership::None);
        auto *vptrAddr = builder->createGEP(baseObj, {zero, zero}, vptrTy,
                                            "vptr.addr", expr.getLoc());
        MIRValue *vtablePtr =
            builder->createLoad(vptrAddr, "vtable.ptr", expr.getLoc());
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *voidPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                voidTy, hir::Ownership::None);

        auto *funcPtrAddr =
            builder->createGEP(vtablePtr, {zero, one, idxVal}, voidPtrTy,
                               "vfunc.addr", expr.getLoc());
        MIRValue *rawFuncPtr =
            builder->createLoad(funcPtrAddr, "vfunc.raw", expr.getLoc());

        const hir::HIRType *expectedPtrTy = voidPtrTy;
        if (staticCalleeFunc) {
          std::vector<const hir::HIRType *> fnParams;
          for (auto &arg : staticCalleeFunc->getRawArguments())
            fnParams.push_back(arg->getType());
          auto *funcTy =
              const_cast<hir::HIRModule *>(hirModule)->getFunctionType(
                  staticCalleeFunc->getType(), fnParams);
          expectedPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  funcTy, hir::Ownership::None);
        }
        callee = builder->createBitCast(rawFuncPtr, expectedPtrTy, "vfunc.cast",
                                        expr.getLoc());
      }

      if (baseObj) {
        if (staticCalleeFunc) {
          if (!staticCalleeFunc->getRawArguments().empty()) {
            baseObj->setBorrowKind(
                staticCalleeFunc->getRawArguments()[0]->getBorrowKind());
          }
        }
        args.push_back(baseObj);
      }
    } else if (llvm::dyn_cast_or_null<hir::HIRSuperExpr>(rawCallee)) {
      std::string className = "";
      if (currFunc) {
        std::string funcName = currFunc->getName();
        size_t dotPos = funcName.find('.');
        if (dotPos != std::string::npos) {
          className = funcName.substr(0, dotPos);
        }
      }
      callee = nullptr;
      for (const auto &func : mirModule->getFunctions()) {
        std::string fnName = func->getName();
        if (fnName.find(".constructor") != std::string::npos &&
            fnName != className + ".constructor") {
          auto params = func->getRawArguments();
          if (params.size() == expr.getArgs().size() + 1) {
            bool match = true;
            for (size_t i = 0; i < expr.getArgs().size(); ++i) {
              const hir::HIRType *expected = params[i + 1]->getType();
              const hir::HIRType *actual = expr.getArgs()[i]->getType();
              if (expected && actual &&
                  expected->getKind() != actual->getKind()) {
                match = false;
                break;
              }
            }
            if (match) {
              callee = func;
              calleeName = fnName;
              break;
            }
          }
        }
      }
      if (callee && symbolMap.count("this")) {
        MIRValue *thisAddr = symbolMap["this"];
        MIRValue *loadedThis =
            builder->createLoad(thisAddr, "this.val", expr.getLoc());
        auto *calleeFunc = static_cast<MIRFunction *>(callee);
        const hir::HIRType *parentThisTy =
            calleeFunc->getRawArguments()[0]->getType();
        MIRValue *castedThis = builder->createBitCast(
            loadedThis, parentThisTy, "base.cast", expr.getLoc());

        args.push_back(castedThis);
      }
    } else {
      lastExprValue = nullptr;
      bool savedLValueContext = isLValueContext;
      isLValueContext = false;
      visit(expr.getCallee());
      isLValueContext = savedLValueContext;
      callee = lastExprValue;
      if (callee) {
        calleeName = callee->getName();
      }
    }

    // Handle Closure Unpacking (Fat Pointers)
    bool isClosure = false;
    if (expr.getCallee() && expr.getCallee()->getType()) {
      const hir::HIRType *astTy = expr.getCallee()->getType();
      while (true) {
        if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(astTy)) {
          astTy = pTy->getPointee();
        } else if (auto *nTy =
                       llvm::dyn_cast_or_null<hir::HIRNullableType>(astTy)) {
          astTy = nTy->getInner();
        } else {
          break;
        }
      }
      if (astTy && astTy->getKind() == hir::TypeKind::Closure) {
        isClosure = true;
      }
    }

    if (!isClosure && callee && callee->getKind() != mir::ValueKind::Function &&
        callee->getType()) {
      const hir::HIRType *mirTy = callee->getType();
      while (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(mirTy)) {
        mirTy = pTy->getPointee();
      }
      if (mirTy->getKind() == hir::TypeKind::Closure) {
        isClosure = true;
      } else if (auto *structTy =
                     llvm::dyn_cast_or_null<hir::StructType>(mirTy)) {
        if (structTy->toString().find("Closure.") != std::string::npos) {
          isClosure = true;
        }
      }
    }

    if (isClosure) {
      MIRValue *closurePtr = callee;
      if (!llvm::dyn_cast_or_null<hir::PointerType>(closurePtr->getType())) {
        auto *spill = builder->createAlloca(closurePtr->getType(),
                                            "closure.spill", expr.getLoc());
        spill->setBorrowKind(mir::BorrowKind::View);
        builder->insert(
            std::make_unique<StoreInst>(closurePtr, spill, expr.getLoc()));
        closurePtr = spill;
      }

      const hir::HIRType *fnPtrTy = nullptr;
      const hir::HIRType *envPtrTy = nullptr;
      const hir::FunctionType *actualFnTy = nullptr;
      const hir::HIRType *baseTy = closurePtr->getType();
      if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(baseTy)) {
        baseTy = pTy->getPointee();
      }

      if (auto *structTy = llvm::dyn_cast_or_null<hir::StructType>(baseTy)) {
        if (structTy->getFields().size() == 2) {
          fnPtrTy = structTy->getFields()[0];
          envPtrTy = structTy->getFields()[1];
          if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(fnPtrTy)) {
            actualFnTy =
                llvm::dyn_cast_or_null<hir::FunctionType>(pTy->getPointee());
          }
        }
      }

      if (!actualFnTy) {
        const hir::HIRClosureType *closTy =
            llvm::dyn_cast_or_null<hir::HIRClosureType>(baseTy);
        if (!closTy && expr.getCallee() && expr.getCallee()->getType()) {
          const hir::HIRType *astTy = expr.getCallee()->getType();
          while (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(astTy)) {
            astTy = pTy->getPointee();
          }
          closTy = llvm::dyn_cast_or_null<hir::HIRClosureType>(astTy);
        }
        if (closTy) {
          auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
          envPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              voidTy, hir::Ownership::None);

          std::vector<const hir::HIRType *> fnParams;
          fnParams.push_back(envPtrTy);
          for (auto *p : closTy->getParamTypes()) {
            fnParams.push_back(p);
          }

          const hir::HIRType *retTy = closTy->getReturnType();
          if (!retTy)
            retTy = voidTy;

          actualFnTy = llvm::cast<hir::FunctionType>(
              const_cast<hir::HIRModule *>(hirModule)->getFunctionType(
                  retTy, fnParams));
          fnPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              actualFnTy, hir::Ownership::None);

          auto *structTy =
              const_cast<hir::HIRModule *>(hirModule)->getStructType(
                  "Closure.Opaque", {fnPtrTy, envPtrTy});
          auto *structPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  structTy, hir::Ownership::None);

          closurePtr = builder->createBitCast(closurePtr, structPtrTy,
                                              "closure.cast", expr.getLoc());
        }
      }

      if (!actualFnTy) {
        diags.report(expr.getLoc(), DiagID::err_invalid_type)
            << "Cannot resolve physical closure layout";
        return;
      }

      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      MIRValue *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
      MIRValue *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
      MIRValue *fnGep = builder->createGEP(closurePtr, {zero, zero}, baseTy,
                                           "fn.gep", expr.getLoc());
      auto *fnPtrPtrTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              fnPtrTy, hir::Ownership::None);
      if (fnGep->getType() != fnPtrPtrTy) {
        fnGep = builder->createBitCast(fnGep, fnPtrPtrTy, "fn.gep.cast",
                                       expr.getLoc());
      }
      MIRValue *fn = builder->createLoad(fnGep, "fn.load", expr.getLoc());
      fn->setBorrowKind(mir::BorrowKind::View);
      MIRValue *envGep = builder->createGEP(closurePtr, {zero, one}, baseTy,
                                            "env.gep", expr.getLoc());
      auto *envPtrPtrTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              envPtrTy, hir::Ownership::None);
      if (envGep->getType() != envPtrPtrTy) {
        envGep = builder->createBitCast(envGep, envPtrPtrTy, "env.gep.cast",
                                        expr.getLoc());
      }
      MIRValue *env = builder->createLoad(envGep, "env.load", expr.getLoc());
      std::vector<MIRValue *> callArgs;
      callArgs.push_back(env);

      const auto &paramTys = actualFnTy->getParamTypes();
      for (size_t i = 0; i < expr.getArgs().size(); ++i) {
        visit(expr.getArgs()[i].get());
        MIRValue *argVal = lastExprValue;
        size_t paramIdx = i + 1;
        if (paramIdx < paramTys.size() &&
            argVal->getType() != paramTys[paramIdx]) {
          argVal = builder->createBitCast(argVal, paramTys[paramIdx],
                                          "clos.arg.cast", expr.getLoc());
        }
        callArgs.push_back(argVal);
      }

      const hir::HIRType *callRetTy = expr.getType();
      if (!callRetTy || callRetTy->getKind() == hir::TypeKind::Void) {
        callRetTy = actualFnTy->getReturnType();
      }
      if (!callRetTy) {
        callRetTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      }

      bool requiresCleanup = false;
      for (const auto &scope : scopeStack) {
        if (!scope.deferredStmts.empty() || !scope.ownedVars.empty() ||
            !scope.refCountedVars.empty()) {
          requiresCleanup = true;
          break;
        }
      }

      if (currentUnwindDest || requiresCleanup) {
        MIRBlock *normalDest = newBlock("invoke.cont");
        MIRBlock *cleanupDest = newBlock("invoke.cleanup");
        lastExprValue =
            builder->createInvoke(fn, std::move(callArgs), normalDest,
                                  cleanupDest, callRetTy, "", expr.getLoc());
        MIRValue *invokeVal = lastExprValue;
        builder->setInsertPoint(cleanupDest);
        MIRBlock *savedUnwind = currentUnwindDest;
        currentUnwindDest = nullptr;
        size_t targetDepth = tryScopeDepths.empty() ? 0 : tryScopeDepths.top();
        for (size_t i = scopeStack.size(); i > targetDepth; --i) {
          emitScopeCleanup(i - 1, expr.getLoc(), true);
        }
        currentUnwindDest = savedUnwind;
        if (currentUnwindDest) {
          builder->createBr(currentUnwindDest);
        } else {
          MIRValue *lpad = builder->createLoad(getExceptionPayloadGlobal(),
                                               "ex.cleanup", expr.getLoc());
          builder->insert(std::make_unique<ResumeInst>(lpad, expr.getLoc()));
        }
        builder->setInsertPoint(normalDest);
        lastExprValue = invokeVal;
      } else {
        lastExprValue = builder->createCall(fn, std::move(callArgs), callRetTy,
                                            "", false, expr.getLoc());
      }
      return;
    }

    auto *mirF = llvm::dyn_cast_or_null<MIRFunction>(callee);
    bool isVarArg = mirF ? mirF->isVariadic() : false;
    std::vector<const hir::HIRType *> expectedParamTys;
    if (mirF) {
      for (auto *arg : mirF->getRawArguments())
        expectedParamTys.push_back(arg->getType());
    } else if (callee && callee->getType()) {
      const hir::HIRType *cTy = callee->getType();
      if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(cTy))
        cTy = pTy->getPointee();
      if (auto *fTy = llvm::dyn_cast_or_null<hir::FunctionType>(cTy)) {
        expectedParamTys = fTy->getParamTypes();
      }
    }
    if (!args.empty() && !expectedParamTys.empty()) {
      const hir::HIRType *expectedTy = expectedParamTys[0];
      if (args[0]->getType() != expectedTy) {
        if (auto *expPtrTy =
                llvm::dyn_cast_or_null<hir::PointerType>(expectedTy)) {
          if (expPtrTy->getPointee() == args[0]->getType() ||
              expPtrTy->getPointee()->toString() ==
                  args[0]->getType()->toString()) {
            MIRValue *tempAlloca = builder->createAlloca(
                args[0]->getType(), "this.coerce", expr.getLoc());
            builder->insert(std::make_unique<StoreInst>(args[0], tempAlloca,
                                                        expr.getLoc()));
            if (tempAlloca->getType() != expectedTy) {
              args[0] = builder->createBitCast(
                  tempAlloca, expectedTy, "this.coerce.cast", expr.getLoc());
            } else {
              args[0] = tempAlloca;
            }
          } else {
            args[0] = builder->createBitCast(args[0], expectedTy, "this.cast",
                                             expr.getLoc());
          }
        } else {
          args[0] = builder->createBitCast(args[0], expectedTy, "this.cast",
                                           expr.getLoc());
        }
      }
    }

    bool oldEscape = inEscapeContext;
    if (calleeName == "moksha_builtin_spawn" ||
        calleeName == "moksha_builtin_spawn_promise" ||
        calleeName == "moksha_builtin_spawn_func" ||
        calleeName == "moksha_builtin_spawn_thread" ||
        calleeName == "moksha_builtin_spawn_weak_thread") {
      inEscapeContext = true;
    }

    size_t hiddenArgOffset = args.size();

    for (size_t i = 0; i < expr.getArgs().size(); ++i) {
      size_t paramIdx = hiddenArgOffset + i;
      const hir::HIRType *expectedArgTy = nullptr;
      if (paramIdx < expectedParamTys.size()) {
        expectedArgTy = expectedParamTys[paramIdx];
      }
      const hir::HIRType *oldExpected = expectedLambdaReturnType;
      expectedLambdaReturnType = expectedArgTy;

      MIRValue *argVal = nullptr;
      const hir::HIRType *actualArgTy = expr.getArgs()[i]->getType();

      bool forcePointer = false;
      if (i == 0 && !callee) {
        std::string n = calleeName;
        if (n == "push" || n == "pop" || n == "length" || n == "at" ||
            n == "is_empty" || n == "copy" || n == "slice" || n == "contains" ||
            n == "index" || n == "fill" || n == "reverse" || n == "sort" ||
            n == "clone" || n == "insert" || n == "remove" || n == "clear" ||
            n == "capacity" || n == "resize" || n == "extend" ||
            n.find("push_") == 0 || n.find("pop_") == 0 ||
            n.find("length_") == 0 || n.find("at_") == 0) {
          forcePointer = true;
        }
      }

      const hir::HIRType *coreArgTy = actualArgTy;
      if (coreArgTy) {
        if (auto *nullTy =
                llvm::dyn_cast_or_null<hir::HIRNullableType>(coreArgTy)) {
          coreArgTy = nullTy->getInner();
        }
      }
      if ((forcePointer || (expectedArgTy && expectedArgTy->getKind() ==
                                                 hir::TypeKind::Pointer)) &&
          coreArgTy && (coreArgTy->getKind() == hir::TypeKind::Array)) {
        const hir::HIRExpr *underlyingExpr = expr.getArgs()[i].get();
        while (auto *castExpr =
                   llvm::dyn_cast_or_null<hir::HIRCastExpr>(underlyingExpr)) {
          underlyingExpr = castExpr->getExpr();
        }

        MIRValue *arrayLVal = nullptr;
        if (llvm::isa<hir::HIRIdentifierExpr>(underlyingExpr) ||
            llvm::isa<hir::HIRMemberExpr>(underlyingExpr) ||
            llvm::isa<hir::HIRIndexExpr>(underlyingExpr)) {
          arrayLVal = evaluateAsLValue(underlyingExpr);
        }

        if (arrayLVal) {
          const hir::ArrayType *arrTy =
              llvm::cast<hir::ArrayType>(underlyingExpr->getType());
          auto *sliceTy = expr.getArgs()[i]->getType();
          argVal = builder->createAlloca(sliceTy, "lvalue.slice.spill",
                                         expr.getLoc());
          auto *i32Ty =
              const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
          auto *i64Ty =
              const_cast<hir::HIRModule *>(hirModule)->getIntType(64, false);
          auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
          auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
          MIRValue *elemPtr = builder->createGEP(arrayLVal, {zero, zero},
                                                 arrTy->getElementType(),
                                                 "array.decay", expr.getLoc());
          auto *ptrFieldTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  arrTy->getElementType(), hir::Ownership::None);
          MIRValue *slicePtrGep = builder->createGEP(
              argVal, {zero, zero}, ptrFieldTy, "slice.ptr.gep", expr.getLoc());
          auto *destPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  ptrFieldTy, hir::Ownership::None);
          if (slicePtrGep->getType() != destPtrTy) {
            slicePtrGep = builder->createBitCast(
                slicePtrGep, destPtrTy, "slice.ptr.gep.cast", expr.getLoc());
          }
          if (elemPtr->getType() != ptrFieldTy) {
            elemPtr = builder->createBitCast(elemPtr, ptrFieldTy,
                                             "array.decay.cast", expr.getLoc());
          }

          builder->insert(
              std::make_unique<StoreInst>(elemPtr, slicePtrGep, expr.getLoc()));
          MIRValue *lenVal = mirModule->getOrInsertConstant<ConstantInt>(
              arrTy->getSize(), i64Ty);
          MIRValue *sliceLenGep = builder->createGEP(
              argVal, {zero, one}, i64Ty, "slice.len.gep", expr.getLoc());
          auto *destLenPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  i64Ty, hir::Ownership::None);
          if (sliceLenGep->getType() != destLenPtrTy) {
            sliceLenGep = builder->createBitCast(
                sliceLenGep, destLenPtrTy, "slice.len.gep.cast", expr.getLoc());
          }
          builder->insert(
              std::make_unique<StoreInst>(lenVal, sliceLenGep, expr.getLoc()));
        }
        if (!argVal) {
          bool savedLValueContext = isLValueContext;
          isLValueContext = false;
          visit(expr.getArgs()[i].get());
          isLValueContext = savedLValueContext;
          MIRValue *tempVal = lastExprValue;
          argVal = builder->createAlloca(tempVal->getType(), "temp.slice.spill",
                                         expr.getLoc());
          builder->insert(
              std::make_unique<StoreInst>(tempVal, argVal, expr.getLoc()));
        }
      } else {
        bool savedLValueContext = isLValueContext;
        isLValueContext = false;
        visit(expr.getArgs()[i].get());
        isLValueContext = savedLValueContext;
        argVal = lastExprValue;
      }
      expectedLambdaReturnType = oldExpected;
      if (!argVal) {
        continue;
      }
      if (calleeName == "print" || calleeName == "println") {
        auto *anyTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();
        auto *anyPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                anyTy, hir::Ownership::None);

        bool isAlreadyPtrToAny = false;
        if (argVal->getType()->getKind() == hir::TypeKind::Pointer) {
          auto *ptrTy =
              static_cast<const hir::PointerType *>(argVal->getType());
          if (ptrTy->getPointee()->getKind() == hir::TypeKind::Any) {
            isAlreadyPtrToAny = true;
          }
        }

        if (!isAlreadyPtrToAny) {
          if (argVal->getType()->getKind() != hir::TypeKind::Any) {
            argVal = boxValue(argVal, argVal->getType(), anyTy, expr.getLoc());
          }
          auto *spill =
              builder->createAlloca(anyTy, "any.spill", expr.getLoc());
          builder->insert(
              std::make_unique<StoreInst>(argVal, spill, expr.getLoc()));
          argVal = spill;
        }

        if (argVal->getType() != anyPtrTy) {
          argVal = builder->createBitCast(argVal, anyPtrTy,
                                          "any.cast.safeguard", expr.getLoc());
        }
        expectedArgTy = anyPtrTy;
      }

      bool isExternCall = false;
      if (auto *mirF = llvm::dyn_cast_or_null<MIRFunction>(callee)) {
        isExternCall = mirF->isDeclaration();
      } else if (!calleeName.empty()) {
        if (auto *hirF = hirModule->getFunction(calleeName)) {
          isExternCall = hirF->isExtern();
        } else {
          isExternCall = true;
        }
      }

      if (isExternCall) {
        bool needsSpill = false;
        auto argKind = argVal->getType()->getKind();
        if (expectedArgTy &&
            expectedArgTy->getKind() == hir::TypeKind::Pointer) {
          auto *ptrTy = static_cast<const hir::PointerType *>(expectedArgTy);
          if (ptrTy->getPointee()->getKind() == argKind) {
            needsSpill = true;
          }
        } else if (!expectedArgTy || paramIdx >= expectedParamTys.size()) {
          if (argKind == hir::TypeKind::Any ||
              argKind == hir::TypeKind::Closure) {
            needsSpill = true;
          }
        }

        if (argKind == hir::TypeKind::Slice || argKind == hir::TypeKind::Map ||
            argKind == hir::TypeKind::String ||
            argKind == hir::TypeKind::Closure) {
          needsSpill = false;
        }

        if (needsSpill) {
          auto *spill = builder->createAlloca(argVal->getType(), "abi.spill",
                                              expr.getLoc());
          builder->insert(
              std::make_unique<StoreInst>(argVal, spill, expr.getLoc()));
          argVal = spill;
        }
      }

      if (expectedArgTy && argVal->getType() != expectedArgTy) {
        bool expectedIsPtrToAny = false;
        if (expectedArgTy->getKind() == hir::TypeKind::Pointer) {
          auto *ptrTy = static_cast<const hir::PointerType *>(expectedArgTy);
          if (ptrTy->getPointee()->getKind() == hir::TypeKind::Any) {
            expectedIsPtrToAny = true;
          }
        }

        if (expectedIsPtrToAny) {
          bool argIsPtrToAny = false;
          if (argVal->getType()->getKind() == hir::TypeKind::Pointer) {
            auto *ptrTy =
                static_cast<const hir::PointerType *>(argVal->getType());
            if (ptrTy->getPointee()->getKind() == hir::TypeKind::Any) {
              argIsPtrToAny = true;
            }
          }
          if (!argIsPtrToAny) {
            auto *anyTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();
            if (argVal->getType()->getKind() != hir::TypeKind::Any) {
              argVal =
                  boxValue(argVal, argVal->getType(), anyTy, expr.getLoc());
            }
            auto *spill =
                builder->createAlloca(anyTy, "any.spill", expr.getLoc());
            builder->insert(
                std::make_unique<StoreInst>(argVal, spill, expr.getLoc()));
            argVal = spill;

            if (argVal->getType() != expectedArgTy) {
              argVal = builder->createBitCast(argVal, expectedArgTy, "any.cast",
                                              expr.getLoc());
            }
          }
        } else if (argVal->getType()->getKind() == hir::TypeKind::Struct &&
                   expectedArgTy->getKind() == hir::TypeKind::Int) {
          argVal = coerceValue(argVal, expectedArgTy, expr.getLoc());
        } else if (expectedArgTy->getKind() == hir::TypeKind::Any ||
                   (expectedArgTy->getKind() == hir::TypeKind::Pointer &&
                    (static_cast<const hir::PointerType *>(expectedArgTy)
                             ->getOwnership() == hir::Ownership::Shared ||
                     static_cast<const hir::PointerType *>(expectedArgTy)
                             ->getOwnership() == hir::Ownership::Owned))) {
          argVal =
              boxValue(argVal, argVal->getType(), expectedArgTy, expr.getLoc());
        } else if (argVal->getType()->getKind() == hir::TypeKind::Any) {
          argVal = unboxValue(argVal, argVal->getType(), expectedArgTy,
                              expr.getLoc());
        } else {
          argVal = coerceValue(argVal, expectedArgTy, expr.getLoc());
        }
      }

      if (mirF && paramIdx < mirF->getRawArguments().size()) {
        argVal->setBorrowKind(
            mirF->getRawArguments()[paramIdx]->getBorrowKind());
      }
      args.push_back(argVal);
    }

    inEscapeContext = oldEscape;
    if ((calleeName == "print" || calleeName == "println") &&
        args.size() == hiddenArgOffset) {
      auto *strTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();
      auto *anyTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();
      auto *i8Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
      auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          i8Ty, hir::Ownership::None);

      MIRValue *rawConst =
          mirModule->getOrInsertConstant<ConstantString>("", i8PtrTy);
      std::string allocName = "__moksha_cstr_to_string";
      ensureBuiltinMIR(allocName);
      MIRFunction *cstrToStrFunc = mirModule->getFunction(allocName);

      if (!cstrToStrFunc) {
        auto fn =
            std::make_unique<MIRFunction>(strTy, allocName, Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i8PtrTy, 0));
        cstrToStrFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *emptyStr =
          builder->createCall(cstrToStrFunc, {rawConst}, strTy,
                              "empty.str.heap", false, expr.getLoc());

      MIRValue *anyEmpty = builder->insert(std::make_unique<CastInst>(
          Opcode::AnyCast, emptyStr, anyTy, "empty.any", expr.getLoc()));

      auto *spill = builder->createAlloca(anyTy, "empty.spill", expr.getLoc());
      builder->insert(
          std::make_unique<StoreInst>(anyEmpty, spill, expr.getLoc()));

      MIRValue *emptyArg = spill;
      auto *anyPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          anyTy, hir::Ownership::None);
      if (emptyArg->getType() != anyPtrTy) {
        emptyArg = builder->createBitCast(emptyArg, anyPtrTy, "empty.cast",
                                          expr.getLoc());
      }
      args.push_back(emptyArg);
    }

    if (calleeName == "print" || calleeName == "println") {
      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          voidTy, hir::Ownership::None);
      MIRValue *nullSentinel =
          mirModule->getOrInsertConstant<ConstantNull>(voidPtrTy);
      args.push_back(nullSentinel);
    }

    if (!calleeName.empty()) {
      const hir::HIRFunction *hirTarget = hirModule->getFunction(calleeName);
      if (hirTarget && args.size() < hirTarget->getParams().size()) {
        for (size_t i = args.size(); i < hirTarget->getParams().size(); ++i) {
          const auto &param = hirTarget->getParams()[i];
          if (param.getDefaultValue()) {
            bool savedLValueContext = isLValueContext;
            isLValueContext = false;
            visit(param.getDefaultValue());
            isLValueContext = savedLValueContext;
            MIRValue *defVal = lastExprValue;
            if (defVal->getType() != param.getType()) {
              defVal = builder->createBitCast(defVal, param.getType(),
                                              "defval.cast", expr.getLoc());
            }
            args.push_back(defVal);
          } else {
            diags.report(expr.getLoc(), DiagID::err_argument_count_mismatch)
                << calleeName << hirTarget->getParams().size() << args.size();
            lastExprValue = nullptr;
            return;
          }
        }
      }
    }

    if (needsZeroPoison) {
      auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
      MIRValue *falseVal =
          mirModule->getOrInsertConstant<ConstantBool>(false, boolTy);
      args.push_back(falseVal);
    }

    std::string cName = callee ? callee->getName() : calleeName;
    bool isUserDefined = false;
    if (!calleeName.empty()) {
      if (const auto *hirF = hirModule->getFunction(calleeName)) {
        if (!hirF->isExtern()) {
          isUserDefined = true;
        }
      }
    }

    if (callee && callee->getKind() == mir::ValueKind::Function) {
      if (!llvm::cast<MIRFunction>(callee)->isDeclaration()) {
        isUserDefined = true;
      }
    }

    std::string rtName = "";
    bool isArrayBuiltin = false;

    if (!isUserDefined && !expr.getArgs().empty()) {
      const hir::HIRExpr *firstArg = expr.getArgs()[0].get();
      while (auto *cast = llvm::dyn_cast_or_null<hir::HIRCastExpr>(firstArg)) {
        if (cast->getType() &&
            cast->getType()->getKind() == hir::TypeKind::Any) {
          break;
        }
        firstArg = cast->getExpr();
      }

      const hir::HIRType *colTy = firstArg->getType();
      if (colTy) {
        while (colTy) {
          if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(colTy)) {
            colTy = ptrTy->getPointee();
          } else if (auto *refTy =
                         llvm::dyn_cast_or_null<hir::ReferenceType>(colTy)) {
            colTy = refTy->getInner();
          } else if (auto *nullTy =
                         llvm::dyn_cast_or_null<hir::HIRNullableType>(colTy)) {
            colTy = nullTy->getInner();
          } else {
            break;
          }
        }

        if (colTy && (colTy->getKind() == hir::TypeKind::Array ||
                      colTy->getKind() == hir::TypeKind::Slice)) {
          isArrayBuiltin = true;
        }
      }
    }

    if (isArrayBuiltin) {
      if (cName.find("length_") == 0 || cName == "length")
        rtName = "moksha_rt_array_length";
      else if (cName.find("push_") == 0 || cName == "push")
        rtName = "moksha_rt_array_push";
      else if (cName.find("pop_") == 0 || cName == "pop")
        rtName = "moksha_rt_array_pop";
      else if (cName.find("at_") == 0 || cName == "at")
        rtName = "moksha_rt_array_at";
      else if (cName.find("is_empty_") == 0 || cName == "is_empty")
        rtName = "moksha_rt_array_is_empty";
      else if (cName.find("copy_") == 0 || cName == "copy")
        rtName = "moksha_rt_array_copy";
      else if (cName.find("slice_") == 0 || cName == "slice")
        rtName = "moksha_rt_array_slice";
      else if (cName.find("contains_") == 0 || cName == "contains")
        rtName = "moksha_rt_array_contains";
      else if (cName.find("index_") == 0 || cName == "index")
        rtName = "moksha_rt_array_index";
      else if (cName.find("fill_") == 0 || cName == "fill")
        rtName = "moksha_rt_array_fill";
      else if (cName.find("reverse_") == 0 || cName == "reverse")
        rtName = "moksha_rt_array_reverse";
      else if (cName.find("sort_") == 0 || cName == "sort")
        rtName = "moksha_rt_array_sort";
      else if (cName.find("clone_") == 0 || cName == "clone")
        rtName = "moksha_rt_array_clone";
      else if (cName.find("insert_") == 0 || cName == "insert")
        rtName = "moksha_rt_array_insert";
      else if (cName.find("remove_") == 0 || cName == "remove")
        rtName = "moksha_rt_array_remove";
      else if (cName.find("clear_") == 0 || cName == "clear")
        rtName = "moksha_rt_array_clear";
      else if (cName.find("capacity_") == 0 || cName == "capacity")
        rtName = "moksha_rt_array_capacity";
      else if (cName.find("resize_") == 0 || cName == "resize")
        rtName = "moksha_rt_array_resize";
      else if (cName.find("extend_") == 0 || cName == "extend")
        rtName = "moksha_rt_array_extend";
    }

    if (!rtName.empty()) {
      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          voidTy, hir::Ownership::None);
      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      auto *i64Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
      auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();

      // 1. Determine Return Type
      const hir::HIRType *retTy = voidTy;
      if (rtName == "moksha_rt_array_pop" || rtName == "moksha_rt_array_at" ||
          rtName == "moksha_rt_array_remove") {
        retTy = voidPtrTy; // Returns *void (raw heap pointer to the element)
      } else if (rtName == "moksha_rt_array_length" ||
                 rtName == "moksha_rt_array_index" ||
                 rtName == "moksha_rt_array_capacity") {
        retTy = i32Ty; // Returns int
      } else if (rtName == "moksha_rt_array_is_empty" ||
                 rtName == "moksha_rt_array_contains") {
        retTy = boolTy; // Returns bool
      } else if (rtName == "moksha_rt_array_slice" ||
                 rtName == "moksha_rt_array_clone") {
        retTy = voidPtrTy;
      }

      MIRFunction *rtFunc = mirModule->getFunction(rtName);
      if (!rtFunc) {
        auto fn =
            std::make_unique<MIRFunction>(retTy, rtName, Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));

        if (rtName == "moksha_rt_array_push" ||
            rtName == "moksha_rt_array_fill" ||
            rtName == "moksha_rt_array_contains" ||
            rtName == "moksha_rt_array_index") {
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
        } else if (rtName == "moksha_rt_array_at" ||
                   rtName == "moksha_rt_array_remove" ||
                   rtName == "moksha_rt_array_resize") {
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 1));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
        } else if (rtName == "moksha_rt_array_pop" ||
                   rtName == "moksha_rt_array_reverse" ||
                   rtName == "moksha_rt_array_clone" ||
                   rtName == "moksha_rt_array_sort") {
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 1));
        } else if (rtName == "moksha_rt_array_insert") {
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 1));
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 2));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 3));
        } else if (rtName == "moksha_rt_array_slice") {
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 1));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 2));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 3));
        } else if (rtName == "moksha_rt_array_copy" ||
                   rtName == "moksha_rt_array_extend") {
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1));
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i64Ty, 2));
        }

        rtFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      callee = rtFunc;
      const hir::HIRType *elemTy = nullptr;
      const hir::HIRExpr *innerArg2 = expr.getArgs()[0].get();
      while (auto *cast = llvm::dyn_cast_or_null<hir::HIRCastExpr>(innerArg2)) {
        if (cast->getType() &&
            cast->getType()->getKind() == hir::TypeKind::Any) {
          break;
        }
        innerArg2 = cast->getExpr();
      }
      const hir::HIRType *colTy2 = innerArg2->getType();
      while (colTy2) {
        if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(colTy2)) {
          colTy2 = ptrTy->getPointee();
        } else if (auto *refTy =
                       llvm::dyn_cast_or_null<hir::ReferenceType>(colTy2)) {
          colTy2 = refTy->getInner();
        } else if (auto *nullTy =
                       llvm::dyn_cast_or_null<hir::HIRNullableType>(colTy2)) {
          colTy2 = nullTy->getInner();
        } else {
          break;
        }
      }

      if (auto *sliceTy = llvm::dyn_cast_or_null<hir::SliceType>(colTy2)) {
        elemTy = sliceTy->getElementType();
      } else if (auto *arrTy = llvm::dyn_cast_or_null<hir::ArrayType>(colTy2)) {
        elemTy = arrTy->getElementType();
      }

      if (!elemTy) {
        elemTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();
      }

      MIRValue *elemSizeVal = nullptr;
      auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              elemTy, hir::Ownership::None));
      auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
      auto *sizeGep = builder->createGEP(nullPtr, {one}, elemTy, "sizeof.gep",
                                         expr.getLoc());
      elemSizeVal = builder->insert(std::make_unique<CastInst>(
          Opcode::PtrToInt, sizeGep, i64Ty, "sizeof.i64", expr.getLoc()));
      std::vector<MIRValue *> cArgs;
      MIRValue *arrArg = args[0];

      while (auto *castInst = llvm::dyn_cast_or_null<CastInst>(arrArg)) {
        if (castInst->getOpcode() == Opcode::BitCast) {
          arrArg = castInst->getValue();
        } else {
          break;
        }
      }

      if (arrArg->getType() != voidPtrTy) {
        arrArg = builder->createBitCast(arrArg, voidPtrTy, "rt.slice.cast",
                                        expr.getLoc());
      }
      cArgs.push_back(arrArg);

      if (rtName == "moksha_rt_array_push" ||
          rtName == "moksha_rt_array_fill" ||
          rtName == "moksha_rt_array_contains" ||
          rtName == "moksha_rt_array_index") {
        MIRValue *valArg = args[1];
        auto *spill = builder->createAlloca(valArg->getType(), "val.spill",
                                            expr.getLoc());
        builder->insert(
            std::make_unique<StoreInst>(valArg, spill, expr.getLoc()));
        MIRValue *valPtr = builder->createBitCast(
            spill, voidPtrTy, "val.ptr.cast", expr.getLoc());

        cArgs.push_back(valPtr);
        cArgs.push_back(elemSizeVal);
      } else if (rtName == "moksha_rt_array_at" ||
                 rtName == "moksha_rt_array_remove" ||
                 rtName == "moksha_rt_array_resize") {
        cArgs.push_back(args[1]);
        cArgs.push_back(elemSizeVal);
      } else if (rtName == "moksha_rt_array_pop" ||
                 rtName == "moksha_rt_array_reverse" ||
                 rtName == "moksha_rt_array_clone" ||
                 rtName == "moksha_rt_array_sort") {
        cArgs.push_back(elemSizeVal);
      } else if (rtName == "moksha_rt_array_insert") {
        cArgs.push_back(args[1]);
        MIRValue *valArg = args[2];

        auto *spill = builder->createAlloca(valArg->getType(), "val.spill",
                                            expr.getLoc());
        builder->insert(
            std::make_unique<StoreInst>(valArg, spill, expr.getLoc()));
        MIRValue *valPtr = builder->createBitCast(
            spill, voidPtrTy, "val.ptr.cast", expr.getLoc());

        cArgs.push_back(valPtr);
        cArgs.push_back(elemSizeVal);
      } else if (rtName == "moksha_rt_array_slice") {
        cArgs.push_back(args[1]);
        cArgs.push_back(args[2]);
        cArgs.push_back(elemSizeVal);
      } else if (rtName == "moksha_rt_array_copy" ||
                 rtName == "moksha_rt_array_extend") {
        MIRValue *srcArg = args[1];
        if (srcArg->getType() != voidPtrTy) {
          srcArg = builder->createBitCast(srcArg, voidPtrTy, "rt.src.cast",
                                          expr.getLoc());
        }
        cArgs.push_back(srcArg);
        cArgs.push_back(elemSizeVal);
      }
      args = std::move(cArgs);
    }

    bool isMapBuiltin = false;
    if (!isUserDefined && !expr.getArgs().empty()) {
      const hir::HIRExpr *firstArg = expr.getArgs()[0].get();
      while (auto *cast = llvm::dyn_cast_or_null<hir::HIRCastExpr>(firstArg)) {
        firstArg = cast->getExpr();
      }

      const hir::HIRType *colTy = firstArg->getType();
      if (colTy) {
        if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(colTy)) {
          colTy = ptrTy->getPointee();
        }
        if (auto *nullTy =
                llvm::dyn_cast_or_null<hir::HIRNullableType>(colTy)) {
          colTy = nullTy->getInner();
        }
        if (colTy && colTy->getKind() == hir::TypeKind::Map) {
          isMapBuiltin = true;
        }
      }
    }

    std::string rtMapName = "";
    if (isMapBuiltin) {
      if (cName == "length" || cName == "length_")
        rtMapName = "moksha_rt_map_length";
      else if (cName == "has" || cName == "has_")
        rtMapName = "moksha_rt_map_has";
      else if (cName == "remove" || cName == "remove_")
        rtMapName = "moksha_rt_map_remove";
      else if (cName == "clear" || cName == "clear_")
        rtMapName = "moksha_rt_map_clear";
    }

    if (!rtMapName.empty()) {
      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          voidTy, hir::Ownership::None);
      auto *anyTy = const_cast<hir::HIRModule *>(hirModule)->getAnyType();
      auto *anyPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          anyTy, hir::Ownership::None);
      auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);

      const hir::HIRType *retTy = voidTy;
      if (rtMapName == "moksha_rt_map_has")
        retTy = boolTy;
      else if (rtMapName == "moksha_rt_map_length")
        retTy = i32Ty;

      MIRFunction *rtFunc = mirModule->getFunction(rtMapName);
      if (!rtFunc) {
        auto fn =
            std::make_unique<MIRFunction>(retTy, rtMapName, Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        if (rtMapName == "moksha_rt_map_has" ||
            rtMapName == "moksha_rt_map_remove") {
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), anyPtrTy, 1));
        }
        rtFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      std::vector<MIRValue *> cArgs;
      MIRValue *mapArg = args[0];
      bool isPtrToAny = false;
      if (auto *pTy =
              llvm::dyn_cast_or_null<hir::PointerType>(mapArg->getType())) {
        if (pTy->getPointee()->getKind() == hir::TypeKind::Any) {
          isPtrToAny = true;
        }
      }

      if (isPtrToAny) {
        MIRValue *loadedAny =
            builder->createLoad(mapArg, "map.any.load", expr.getLoc());
        mapArg = builder->insert(std::make_unique<ExtractValueInst>(
            loadedAny, 0, voidPtrTy, "map.unboxed.ptr", expr.getLoc()));
      } else if (mapArg->getType()->getKind() == hir::TypeKind::Any) {
        mapArg = builder->insert(std::make_unique<ExtractValueInst>(
            mapArg, 0, voidPtrTy, "map.unboxed.ptr", expr.getLoc()));
      }

      if (mapArg->getType() != voidPtrTy) {
        mapArg = builder->createBitCast(mapArg, voidPtrTy, "rt.map.cast",
                                        expr.getLoc());
      }

      cArgs.push_back(mapArg);
      if (rtMapName == "moksha_rt_map_has" ||
          rtMapName == "moksha_rt_map_remove") {
        MIRValue *keyArg = args[1];
        if (expr.getArgs()[1]->getType()->getKind() != hir::TypeKind::Any) {
          keyArg = boxValue(keyArg, expr.getArgs()[1]->getType(), anyTy,
                            expr.getLoc());
        }
        if (keyArg->getType()->getKind() != hir::TypeKind::Pointer) {
          auto *spill =
              builder->createAlloca(anyTy, "map.key.spill", expr.getLoc());
          builder->insert(
              std::make_unique<StoreInst>(keyArg, spill, expr.getLoc()));
          keyArg = spill;
        }
        if (keyArg->getType() != anyPtrTy) {
          keyArg = builder->createBitCast(keyArg, anyPtrTy, "map.key.cast",
                                          expr.getLoc());
        }
        cArgs.push_back(keyArg);
      }
      args = std::move(cArgs);
      callee = rtFunc;
      callRetTy = retTy;
    }

    std::string rtStringName = "";
    if (!isArrayBuiltin && !isMapBuiltin && !isUserDefined) {
      if (cName.find("substring") == 0)
        rtStringName = "moksha_string_substring";
      else if (cName.find("contains") == 0)
        rtStringName = "moksha_string_contains";
      else if (cName.find("index") == 0)
        rtStringName = "moksha_string_index";
      else if (cName.find("starts_with") == 0)
        rtStringName = "moksha_string_starts_with";
      else if (cName.find("ends_with") == 0)
        rtStringName = "moksha_string_ends_with";
      else if (cName.find("slice") == 0)
        rtStringName = "moksha_string_slice";
      else if (cName.find("to_upper") == 0)
        rtStringName = "moksha_string_to_upper";
      else if (cName.find("to_lower") == 0)
        rtStringName = "moksha_string_to_lower";
      else if (cName.find("trim") == 0)
        rtStringName = "moksha_string_trim";
      else if (cName.find("replace") == 0)
        rtStringName = "moksha_string_replace";
      else if (cName.find("split") == 0)
        rtStringName = "moksha_string_split";
      else if (cName.find("is_digit") == 0)
        rtStringName = "moksha_string_is_digit";
      else if (cName.find("is_alpha") == 0)
        rtStringName = "moksha_string_is_alpha";
      else if (cName.find("is_whitespace") == 0)
        rtStringName = "moksha_string_is_whitespace";
      else if (cName == "length" || cName.find("length_") == 0) {
        bool isAnyArg = false;
        if (!expr.getArgs().empty()) {
          const hir::HIRExpr *firstArg = expr.getArgs()[0].get();
          const hir::HIRType *argTy = firstArg->getType();
          while (argTy) {
            if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(argTy)) {
              argTy = ptrTy->getPointee();
            } else if (auto *nullTy =
                           llvm::dyn_cast_or_null<hir::HIRNullableType>(
                               argTy)) {
              argTy = nullTy->getInner();
            } else {
              break;
            }
          }

          if (argTy && argTy->getKind() == hir::TypeKind::Any) {
            isAnyArg = true;
          }
        }

        if (isAnyArg) {
          rtStringName = "moksha_rt_any_len";
        } else {
          rtStringName = "moksha_rt_string_len";
        }
      } else if (cName == "at" || cName.find("at_") == 0)
        rtStringName = "moksha_rt_string_char_at";
    }

    if (cName == "join" || cName == "moksha_builtin_join") {
      bool isStrJoin = false;
      if (!expr.getArgs().empty()) {
        const hir::HIRType *firstArgTy = expr.getArgs()[0]->getType();
        while (firstArgTy) {
          if (auto *ptrTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(firstArgTy)) {
            firstArgTy = ptrTy->getPointee();
          } else if (auto *refTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                         firstArgTy)) {
            firstArgTy = refTy->getInner();
          } else if (auto *nullTy =
                         llvm::dyn_cast_or_null<hir::HIRNullableType>(
                             firstArgTy)) {
            firstArgTy = nullTy->getInner();
          } else {
            break;
          }
        }

        if (firstArgTy && (firstArgTy->getKind() == hir::TypeKind::Slice ||
                           firstArgTy->getKind() == hir::TypeKind::Array)) {
          isStrJoin = true;
        }
      }
      if (isStrJoin) {
        rtStringName = "moksha_string_join";
      }
    }

    if (!rtStringName.empty()) {
      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          voidTy, hir::Ownership::None);

      if (rtStringName == "moksha_string_join" && !args.empty()) {
        MIRValue *arrArg = args[0];
        if (arrArg->getType() != voidPtrTy) {
          arrArg = builder->createBitCast(arrArg, voidPtrTy, "join.arr.cast",
                                          expr.getLoc());
        }
        args[0] = arrArg;
      }

      MIRFunction *rtFunc = mirModule->getFunction(rtStringName);
      if (!rtFunc) {
        const hir::HIRType *retTy = expr.getType();
        if (!retTy)
          retTy = voidTy;
        if (rtStringName == "moksha_string_split") {
          retTy = voidPtrTy;
        }

        auto fn = std::make_unique<MIRFunction>(retTy, rtStringName,
                                                Linkage::External);
        for (size_t k = 0; k < args.size(); k++) {
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), args[k]->getType(), k));
        }
        rtFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      callee = rtFunc;
    }

    std::string rtMathName = "";
    if (!isArrayBuiltin && !isMapBuiltin && rtStringName.empty() &&
        !isUserDefined) {
      static const std::unordered_map<std::string, std::string> llvmMath = {
          {"sqrt", "llvm.sqrt.f64"},   {"sin", "llvm.sin.f64"},
          {"cos", "llvm.cos.f64"},     {"exp", "llvm.exp.f64"},
          {"log", "llvm.log.f64"},     {"log10", "llvm.log10.f64"},
          {"log2", "llvm.log2.f64"},   {"floor", "llvm.floor.f64"},
          {"ceil", "llvm.ceil.f64"},   {"trunc", "llvm.trunc.f64"},
          {"round", "llvm.round.f64"}, {"abs", "llvm.fabs.f64"}};

      if (llvmMath.count(cName)) {
        rtMathName = llvmMath.at(cName);
      } else if (cName == "tan" || cName == "asin" || cName == "acos" ||
                 cName == "atan" || cName == "atan2" || cName == "cbrt" ||
                 cName == "hypot" || cName == "fmod" || cName == "mod" ||
                 cName == "random" || cName == "randint" || cName == "seed" ||
                 cName == "lerp" || cName == "clamp" || cName == "isPowerOf2" ||
                 cName == "isnan" || cName == "isinf" || cName == "isfinite" ||
                 cName == "min" || cName == "max" || cName == "sign" ||
                 cName == "is_close") {
        rtMathName = "moksha_rt_math_" + cName;
      }
    }

    if (!rtMathName.empty()) {
      auto *f64Ty = const_cast<hir::HIRModule *>(hirModule)->getFloatType(64);
      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      auto *boolTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      const hir::HIRType *retTy = f64Ty;
      if (cName == "isPowerOf2" || cName == "isnan" || cName == "isinf" ||
          cName == "isfinite" || cName == "is_close") {
        retTy = boolTy;
      } else if (cName == "randint")
        retTy = i32Ty;
      else if (cName == "seed")
        retTy = voidTy;

      MIRFunction *rtFunc = mirModule->getFunction(rtMathName);
      if (!rtFunc) {
        auto fn =
            std::make_unique<MIRFunction>(retTy, rtMathName, Linkage::External);
        for (size_t k = 0; k < args.size(); k++) {
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), args[k]->getType(), k));
        }
        rtFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      callee = rtFunc;
      callRetTy = retTy;
    }

    std::string rtFileName = "";
    if (!isArrayBuiltin && !isMapBuiltin && rtStringName.empty() &&
        rtMathName.empty() && !isUserDefined) {
      static const std::unordered_map<std::string, std::string> fileBuiltins = {
          {"open", "moksha_file_open"},
          {"close", "moksha_file_close"},
          {"read", "moksha_file_read"},
          {"write", "moksha_file_write"},
          {"readText", "moksha_file_readText"},
          {"writeText", "moksha_file_writeText"},
          {"appendText", "moksha_file_appendText"},
          {"readBytes", "moksha_file_readBytes"},
          {"writeBytes", "moksha_file_writeBytes"},
          {"appendBytes", "moksha_file_appendBytes"},
          {"readJson", "moksha_file_readJson"},
          {"writeJson", "moksha_file_writeJson"},
          {"readYaml", "moksha_file_readYaml"},
          {"writeYaml", "moksha_file_writeYaml"},
          {"readCsv", "moksha_file_readCsv"},
          {"writeCsv", "moksha_file_writeCsv"},
          {"openPdf", "moksha_file_openPdf"},
          {"createPdf", "moksha_file_createPdf"},
          {"extractText", "moksha_file_extractText"},
          {"writePdfText", "moksha_file_writePdfText"},
          {"savePdf", "moksha_file_savePdf"},
          {"readLine", "moksha_file_readLine"},
          {"writeLine", "moksha_file_writeLine"},
          {"readLines", "moksha_file_readLines"},
          {"seek", "moksha_file_seek"},
          {"tell", "moksha_file_tell"},
          {"flush", "moksha_file_flush"},
          {"eof", "moksha_file_eof"},
          {"size", "moksha_file_size"},
          {"truncate", "moksha_file_truncate"},
          {"exists", "moksha_file_exists"},
          {"isFile", "moksha_file_isFile"},
          {"isDir", "moksha_file_isDir"},
          {"createDir", "moksha_file_createDir"},
          {"remove", "moksha_file_remove"},
          {"removeDir", "moksha_file_removeDir"},
          {"copy", "moksha_file_copy"},
          {"move_file", "moksha_file_move"},
          {"listDir", "moksha_file_listDir"}};

      if (fileBuiltins.count(cName)) {
        rtFileName = fileBuiltins.at(cName);
      }
    }

    if (!rtFileName.empty()) {
      MIRFunction *rtFunc = mirModule->getFunction(rtFileName);
      if (!rtFunc) {
        const hir::HIRType *retTy = expr.getType();
        if (!retTy)
          retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto fn =
            std::make_unique<MIRFunction>(retTy, rtFileName, Linkage::External);
        for (size_t k = 0; k < args.size(); k++) {
          fn->addArgument(
              std::make_unique<MIRArgument>(fn.get(), args[k]->getType(), k));
        }
        rtFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      callee = rtFunc;
    }

    if (!callee) {
      diags.report(expr.getLoc(), DiagID::err_invalid_type)
          << "Failed to resolve function call: " << cName;
      lastExprValue = nullptr;
      return;
    }

    if (auto *mirTargetF = llvm::dyn_cast_or_null<MIRFunction>(callee)) {
      callRetTy = mirTargetF->getType();
    } else if (!callRetTy || callRetTy->getKind() == hir::TypeKind::Void) {
      callRetTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    std::string callName = "";
    bool requiresCleanup = false;
    for (const auto &scope : scopeStack) {
      if (!scope.deferredStmts.empty() || !scope.ownedVars.empty() ||
          !scope.refCountedVars.empty()) {
        requiresCleanup = true;
        break;
      }
    }

    bool isLLVMIntrinsic = false;
    if (callee) {
      if (auto *mirTargetF = llvm::dyn_cast_or_null<MIRFunction>(callee)) {
        if (mirTargetF->getName().find("llvm.") == 0) {
          isLLVMIntrinsic = true;
        }
      }
    }

    if (!isLLVMIntrinsic && (currentUnwindDest || requiresCleanup)) {
      MIRBlock *normalDest = newBlock("invoke.cont");
      MIRBlock *cleanupDest = newBlock("invoke.cleanup");

      lastExprValue =
          builder->createInvoke(callee, std::move(args), normalDest,
                                cleanupDest, callRetTy, "", expr.getLoc());

      MIRValue *invokeVal = lastExprValue;
      builder->setInsertPoint(cleanupDest);
      auto *i8Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(8, true);
      auto *i8PtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          i8Ty, hir::Ownership::None);
      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      auto *lpadType = const_cast<hir::HIRModule *>(hirModule)->getStructType(
          "eh_result", {i8PtrTy, i32Ty});
      auto *cleanupLpad =
          builder->createLandingPad(lpadType, "cleanup.lpad", expr.getLoc());

      MIRBlock *savedUnwind = currentUnwindDest;
      MIRBlock *savedUnwindBody = currentUnwindBody;
      currentUnwindDest = nullptr;
      currentUnwindBody = nullptr;

      size_t targetDepth = tryScopeDepths.empty() ? 0 : tryScopeDepths.top();
      for (size_t i = scopeStack.size(); i > targetDepth; --i) {
        emitScopeCleanup(i - 1, expr.getLoc(), true);
      }
      currentUnwindDest = savedUnwind;
      currentUnwindBody = savedUnwindBody;

      if (currentUnwindBody) {
        builder->createBr(currentUnwindBody);
      } else if (currentUnwindDest) {
        builder->createBr(currentUnwindDest);
      } else {
        builder->insert(
            std::make_unique<ResumeInst>(cleanupLpad, expr.getLoc()));
      }

      builder->setInsertPoint(normalDest);
      lastExprValue = invokeVal;
    } else {
      if (!args.empty() && callee) {
        if (auto *calleeFunc = llvm::dyn_cast_or_null<MIRFunction>(callee)) {
          auto params = calleeFunc->getRawArguments();
          if (!params.empty() && params[0]->getName() == "this") {
            MIRValue *thisArg = args[0];
            MIRValue *traced = thisArg;

            while (auto *c = llvm::dyn_cast_or_null<CastInst>(traced)) {
              traced = c->getValue();
            }

            if (auto *alloca = llvm::dyn_cast_or_null<AllocaInst>(traced)) {
              if (alloca->getName().find("coerce") != std::string::npos) {
                MIRValue *heapPtr =
                    builder->createLoad(alloca, "this.heap.ptr", expr.getLoc());
                args[0] =
                    builder->createBitCast(heapPtr, thisArg->getType(),
                                           "this.hack.cast", expr.getLoc());
              }
            }
          }
        }
      }
      lastExprValue = builder->createCall(callee, std::move(args), callRetTy,
                                          callName, isVarArg, expr.getLoc());
    }

    applyBorrowKind(lastExprValue, callRetTy);
    const hir::HIRType *expectedAstTy = expr.getType();
    if (lastExprValue && expectedAstTy &&
        lastExprValue->getType() != expectedAstTy) {
      if (lastExprValue->getType()->getKind() == hir::TypeKind::Pointer &&
          expectedAstTy->getKind() != hir::TypeKind::Pointer) {
        bool isASTRefType = false;
        auto ak = expectedAstTy->getKind();
        if (ak == hir::TypeKind::String || ak == hir::TypeKind::Slice ||
            ak == hir::TypeKind::Map || ak == hir::TypeKind::Any ||
            ak == hir::TypeKind::Closure || ak == hir::TypeKind::Promise ||
            ak == hir::TypeKind::Array) {
          isASTRefType = true;
        }

        if (!isASTRefType) {
          MIRValue *wrapperPtr = lastExprValue;
          auto *targetPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  expectedAstTy, hir::Ownership::None);
          if (wrapperPtr->getType() != targetPtrTy) {
            wrapperPtr = builder->createBitCast(wrapperPtr, targetPtrTy,
                                                "ret.ptr.cast", expr.getLoc());
          }
          lastExprValue =
              builder->createLoad(wrapperPtr, "abi.ret.load", expr.getLoc());

          bool isInteriorPtr = (rtName == "moksha_rt_array_at" ||
                                rtName == "moksha_rt_array_pop" ||
                                rtName == "moksha_rt_array_remove" ||
                                rtMapName == "moksha_rt_map_get_val_at" ||
                                rtMapName == "moksha_rt_map_get_key_at" ||
                                calleeName.find("at_") == 0);
          if (!isInteriorPtr) {
            std::string freeName = "moksha_mem_free";
            ensureBuiltinMIR(freeName);
            MIRFunction *freeFunc = mirModule->getFunction(freeName);
            auto *voidTy =
                const_cast<hir::HIRModule *>(hirModule)->getVoidType();
            auto *voidPtrTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    voidTy, hir::Ownership::None);

            if (!freeFunc) {
              auto fn = std::make_unique<MIRFunction>(voidTy, freeName,
                                                      Linkage::External);
              fn->addArgument(
                  std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
              freeFunc = fn.get();
              mirModule->addFunction(std::move(fn));
            }

            MIRValue *castToVoid = builder->createBitCast(
                wrapperPtr, voidPtrTy, "free.cast", expr.getLoc());
            builder->insert(std::make_unique<CallInst>(
                freeFunc, std::vector<MIRValue *>{castToVoid}, voidTy, "",
                false, expr.getLoc()));
          }
        }
      }

      if (lastExprValue->getType()->toString() != expectedAstTy->toString()) {
        if (expectedAstTy->getKind() == hir::TypeKind::Any ||
            (expectedAstTy->getKind() == hir::TypeKind::Pointer &&
             (static_cast<const hir::PointerType *>(expectedAstTy)
                      ->getOwnership() == hir::Ownership::Shared ||
              static_cast<const hir::PointerType *>(expectedAstTy)
                      ->getOwnership() == hir::Ownership::Owned))) {
          lastExprValue = boxValue(lastExprValue, lastExprValue->getType(),
                                   expectedAstTy, expr.getLoc());
        } else if (lastExprValue->getType()->getKind() == hir::TypeKind::Any) {
          lastExprValue = unboxValue(lastExprValue, lastExprValue->getType(),
                                     expectedAstTy, expr.getLoc());
        } else if (lastExprValue->getType()->getKind() == hir::TypeKind::Int &&
                   expectedAstTy->getKind() == hir::TypeKind::Float) {
          lastExprValue = builder->createIntToFloat(
              lastExprValue, expectedAstTy, "prom.cast", expr.getLoc());
        } else {
          bool skipCast = false;
          auto expectedKind = expectedAstTy->getKind();
          auto actualKind = lastExprValue->getType()->getKind();

          if (actualKind == hir::TypeKind::Struct ||
              actualKind == hir::TypeKind::Array ||
              actualKind == hir::TypeKind::Slice ||
              actualKind == hir::TypeKind::Map ||
              actualKind == hir::TypeKind::Nullable ||
              expectedKind == hir::TypeKind::Struct ||
              expectedKind == hir::TypeKind::Array ||
              expectedKind == hir::TypeKind::Slice ||
              expectedKind == hir::TypeKind::Map ||
              expectedKind == hir::TypeKind::Nullable) {
            skipCast = true;
          }

          if (actualKind == hir::TypeKind::Pointer) {
            if (expectedKind == hir::TypeKind::Struct ||
                expectedKind == hir::TypeKind::Array ||
                expectedKind == hir::TypeKind::Map) {
              skipCast = true;
            }
          }

          if (!skipCast) {
            lastExprValue = builder->createBitCast(lastExprValue, expectedAstTy,
                                                   "opt.cast", expr.getLoc());
          }
        }
      }
    }
  }

  void visit(const hir::HIRStmt *stmt) {
    if (stmt)
      stmt->accept(*this);
  }
  void visit(const hir::HIRExpr *expr) {
    if (expr)
      expr->accept(*this);
  }
};
} // namespace

std::unique_ptr<MIRModule> LowerHIRToMIR(const hir::HIRModule *hirModule,
                                         DiagnosticEngine &diags) {
  return HIRToMIRConverter(hirModule, diags).run();
}

} // namespace mir
} // namespace moksha
