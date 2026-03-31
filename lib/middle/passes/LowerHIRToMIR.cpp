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
  std::stack<MIRValue *> currentExceptionSlots;
  unsigned lambdaCounter = 0;
  const hir::HIRType *expectedLambdaReturnType = nullptr;
  std::unordered_map<std::string, uint64_t> enumVariantValues;

  // --- [NEW] MONOMORPHIZATION QUEUE STATE ---
  struct MonomorphizationTask {
    const hir::HIRClass *genericClass = nullptr;
    const hir::HIRFunction *genericFunc = nullptr;
    std::vector<const hir::HIRType *> typeArgs;
  };
  std::queue<MonomorphizationTask> monoQueue;
  std::unordered_set<std::string> instantiatedGenerics;
  std::unordered_map<std::string, const hir::HIRType *> currentTypeEnv;

  // Substitutes generic 'T' with concrete types during lowering
  const hir::HIRType *resolveType(const hir::HIRType *t) {
    if (!t)
      return nullptr;
    std::string tName = t->toString();
    if (currentTypeEnv.count(tName)) {
      return currentTypeEnv[tName];
    }
    // Unwrap pointers safely
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(t)) {
      const hir::HIRType *resolved = resolveType(ptrTy->getPointee());
      if (resolved != ptrTy->getPointee()) {
        return const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            resolved, ptrTy->getOwnership());
      }
    }
    return t;
  }

  MIRValue *evaluateAsLValue(const hir::HIRExpr *expr) {
    if (!expr)
      return nullptr;

    visit(expr);
    MIRValue *val = lastExprValue;

    if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(val)) {
      auto &insts = builder->getInsertBlock()->getInstructionsMut();
      if (!insts.empty() && insts.back().get() == loadInst) {
        insts.pop_back();
      }
      return loadInst->getPointer();
    } else if (auto *loadWeak = llvm::dyn_cast_or_null<LoadWeakInst>(val)) {
      auto &insts = builder->getInsertBlock()->getInstructionsMut();
      if (!insts.empty() && insts.back().get() == loadWeak) {
        insts.pop_back();
      }
      return loadWeak->getPointer();
    }

    return val; // It was already an L-Value (like a GEP)
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
    // 1. If it's already in the MIR module, we're done.
    if (mirModule->getFunction(name))
      return;

    // 2. Search the HIR Module's function list for this builtin.
    // BuiltinRegistry already populated these during the Sema phase.
    for (const auto *hirFunc : hirModule->getFunctions()) {
      if (hirFunc->getName() == name && hirFunc->isExtern()) {
        createFunctionDecl(hirFunc); // Reuse your existing decl logic
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

        // Deep Inference: Extract 'T' from the lambda signature so the inner
        // body can use it!
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
        std::replace(retStr.begin(), retStr.end(), '*', 'p');
        mangledName += "_ret_" + retStr;

        if (!mirModule->getFunction(mangledName)) {
          createFunctionDecl(task.genericFunc, mangledName, nullptr);
        }

        // Emit the body using the populated currentTypeEnv mapping
        lowerFunction(*task.genericFunc, mangledName, nullptr);

        currentTypeEnv.clear();
        continue; // Move to next task
      }

      // Map generic parameters to concrete types based on constructor params
      for (const auto &method : task.genericClass->getMethods()) {
        if (method->getName() == "constructor") {
          for (size_t i = 0;
               i < method->getParams().size() && i < task.typeArgs.size();
               ++i) {
            std::string pName = method->getParams()[i].type->toString();
            if (pName.length() == 1 || pName == "T") { // Generic param heuristc
              currentTypeEnv[pName] = task.typeArgs[i];
            }
          }
        }
      }

      // Fallback for standard single-parameter <T>
      if (currentTypeEnv.empty() && task.typeArgs.size() == 1) {
        currentTypeEnv["T"] = task.typeArgs[0];
      }

      const hir::HIRType *thisTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              task.genericClass->getType(), hir::Ownership::Borrowed);

      // [FIX] Iterate through ALL methods, including the destructor, and
      // generate them!
      for (const auto &method : task.genericClass->getMethods()) {
        if (!method->isExtern()) {
          std::vector<const hir::HIRType *> pTys;
          for (const auto &p : method->getParams()) {
            pTys.push_back(resolveType(p.type));
          }

          // [FIX] Inject the concrete class type into the prefix (e.g.,
          // "*Container<i32>" -> "Container_i32")
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
            std::replace(retStr.begin(), retStr.end(), '*', 'p');
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

    // 1. Process Functions
    for (const auto *func : hirModule->getFunctions()) {
      createFunctionDecl(func);
    }

    // Process Class Method Declarations
    for (const auto *cls : hirModule->getClasses()) {
      const hir::HIRType *thisTy = cls->getType();
      for (const auto &method : cls->getMethods()) {
        std::vector<const hir::HIRType *> pTys;
        for (const auto &p : method->getParams())
          pTys.push_back(p.type);

        std::string mangledName =
            mangleName(cls->getName() + "." + method->getName(), pTys);
        std::string retStr = method->getReturnType()
                                 ? method->getReturnType()->toString()
                                 : "void";
        std::replace(retStr.begin(), retStr.end(), '*', 'p');
        mangledName += "_ret_" + retStr;

        createFunctionDecl(method.get(), mangledName, thisTy);
      }
      generateVTable(cls);
    }

    // 2. Process Globals
    builder->setInsertPoint(initBlock); // Ensure we start in entry
    currFunc = initFunc;
    for (const auto &stmt : hirModule->getGlobals()) {
      createGlobalDecl(stmt);
    }

    // ✅ SAVE THE EXIT BLOCK AFTER GLOBALS ARE INITIALIZED!
    MIRBlock *initExitBlock = builder->getInsertBlock();

    // 3. Lower Bodies
    for (const auto *func : hirModule->getFunctions()) {
      if (!func->isExtern()) {
        lowerFunction(*func);
      }
    }

    // Lower Class Method Bodies (Skipping raw uninstantiated templates)
    for (const auto *cls : hirModule->getClasses()) {
      const hir::HIRType *thisTy = cls->getType();
      for (const auto &method : cls->getMethods()) {
        if (!method->isExtern()) {
          bool isGenericTemplate = false;
          for (const auto &p : method->getParams()) {
            if (p.type->toString() == "T" || p.type->toString().length() == 1)
              isGenericTemplate = true;
          }
          if (isGenericTemplate)
            continue; // Skip raw templates!

          std::vector<const hir::HIRType *> pTys;
          for (const auto &p : method->getParams())
            pTys.push_back(p.type);

          std::string mangledName =
              mangleName(cls->getName() + "." + method->getName(), pTys);
          std::string retStr = method->getReturnType()
                                   ? method->getReturnType()->toString()
                                   : "void";
          std::replace(retStr.begin(), retStr.end(), '*', 'p');
          mangledName += "_ret_" + retStr;

          lowerFunction(*method.get(), mangledName, thisTy);
        }
      }
    }

    // --- [NEW] PROCESS QUEUE BEFORE EXIT ---
    processMonomorphizationQueue();

    // ---------------------------------------------------------
    // 4. Terminate the Init Function
    // ---------------------------------------------------------
    builder->setInsertPoint(initExitBlock);
    builder->insert(std::make_unique<ReturnInst>(nullptr, SourceLocation{}));
    initFunc->numberUnnamedValues();
    builder->clearInsertPoint();

    // ---------------------------------------------------------
    // 5. __moksha_module_destroy Generation
    // ---------------------------------------------------------
    auto destroyF = std::make_unique<MIRFunction>(
        const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
        "__moksha_module_destroy", Linkage::External);
    MIRFunction *destroyFunc = destroyF.get();
    mirModule->addFunction(std::move(destroyF));

    auto db = std::make_unique<MIRBlock>("entry", destroyFunc);
    builder->setInsertPoint(db.get());
    destroyFunc->addBlock(std::move(db));
    currFunc = destroyFunc;

    // [FIX] Safely iterate globals and call destructors ONLY if they actually
    // exist!
    for (const auto &global : hirModule->getGlobals()) {
      if (auto *varDecl = llvm::dyn_cast<hir::HIRVarDeclStmt>(global)) {
        std::string baseName;
        const hir::HIRType *actualType = varDecl->getType();

        if (auto *ptrTy =
                llvm::dyn_cast_or_null<hir::PointerType>(actualType)) {
          actualType = ptrTy->getPointee();
        }

        if (auto *stTy = llvm::dyn_cast_or_null<hir::StructType>(actualType)) {
          baseName = stTy->getName().str();

          // [FIX] Sanitize instead of stripping!
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

          // Only call the destructor if the compiler explicitly generated one!
          if (MIRFunction *dtorFunc = mirModule->getFunction(dtorName)) {
            MIRGlobal *gVar = mirModule->getGlobal(varDecl->getName());
            if (gVar) {
              MIRValue *argVal = gVar;

              // Safely align types without crossing Module boundaries
              const hir::HIRType *expectedTy =
                  dtorFunc->getRawArguments().empty()
                      ? nullptr
                      : dtorFunc->getRawArguments()[0]->getType();
              if (expectedTy && argVal->getType() != expectedTy) {
                argVal = builder->createBitCast(argVal, expectedTy, "gvar.cast",
                                                varDecl->getLoc());
              }

              argVal->setBorrowKind(mir::BorrowKind::View);
              builder->createCall(
                  dtorFunc, {argVal},
                  const_cast<hir::HIRModule *>(hirModule)->getVoidType(), "",
                  false, varDecl->getLoc());
            }
          }
        }
      }
    }

    builder->insert(std::make_unique<ReturnInst>(nullptr, SourceLocation{}));
    destroyFunc->numberUnnamedValues();
    builder->clearInsertPoint();

    return std::move(mirModule);
  }

  // --- Lexical Scope Tracker ---
  struct LexicalScope {
    std::vector<MIRValue *> refCountedVars;
    std::vector<MIRValue *> ownedVars;
    std::vector<const hir::HIRStmt *> deferredStmts;
  };
  std::vector<LexicalScope> scopeStack;

  void emitScopeCleanup(LexicalScope scope, SourceLocation loc) {
    // 1. Run Deferred Statements (LIFO: last deferred, first executed)
    for (auto it = scope.deferredStmts.rbegin();
         it != scope.deferredStmts.rend(); ++it) {
      visit(*it);
    }

    // [NEW] Helper to process both Owned and Shared drops
    auto processDrops = [&](const std::vector<MIRValue *> &vars, bool isARC) {
      for (auto it = vars.rbegin(); it != vars.rend(); ++it) {
        MIRValue *allocaPtr = *it;

        const hir::HIRType *ptrTy = allocaPtr->getType();
        const hir::HIRType *valTy = nullptr;
        if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(ptrTy)) {
          valTy = pTy->getPointee();
        }

        if (valTy) {
          std::string typeName = valTy->toString();

          if (typeName.find("shared ") == 0)
            typeName = typeName.substr(7);
          if (typeName.find("owned ") == 0)
            typeName = typeName.substr(6);
          if (typeName.find("weak ") == 0)
            typeName = typeName.substr(5);

          // Unwrap "Arc<File>" or "Box<File>" to just "File"
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
            if (endPos != std::string::npos) {
              typeName = typeName.substr(0, endPos);
            }
          }

          // Clean up pointer prefixes
          while (!typeName.empty() &&
                 (typeName[0] == '&' || typeName[0] == '*' ||
                  typeName[0] == ' ')) {
            typeName = typeName.substr(1);
          }

          // Strip "struct " or "class " from the inner type name
          if (typeName.find("struct ") == 0)
            typeName = typeName.substr(7);
          if (typeName.find("class ") == 0)
            typeName = typeName.substr(6);

          // Move the LoadInst BEFORE the destructor check!
          auto *loaded = builder->insert(
              std::make_unique<LoadInst>(allocaPtr, "cleanup_val", loc));

          std::string dropName = typeName + ".destructor_ret_void";

          if (MIRFunction *dropFunc = mirModule->getFunction(dropName)) {
            // Pass the LOADED value to the destructor, not the allocaPtr
            MIRValue *argVal = loaded;

            if (!dropFunc->getRawArguments().empty()) {
              const hir::HIRType *expectedTy =
                  dropFunc->getRawArguments()[0]->getType();
              if (argVal->getType() != expectedTy) {
                argVal = builder->createBitCast(argVal, expectedTy, "drop.cast",
                                                loc);
              }
            }

            // Emit the destructor call
            builder->insert(std::make_unique<CallInst>(
                dropFunc, std::vector<MIRValue *>{argVal}, dropFunc->getType(),
                "", false, loc));
          }

          if (isARC) {
            // Emit ARC Release for shared objects
            builder->insert(
                std::make_unique<ARCInst>(Opcode::Release, loaded, loc));
          } else {
            bool isClosureType =
                (valTy->getKind() == hir::TypeKind::Closure ||
                 valTy->toString().find("closure") != std::string::npos);

            if (valTy->toString().find("Box<") != std::string::npos ||
                valTy->getKind() == hir::TypeKind::Pointer || isClosureType) {
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

              MIRValue *castToVoid = nullptr;

              // Safely extract the env_ptr from the Closure Fat Pointer
              if (isClosureType) {
                // GUARANTEE strict type matching by reading the parameter type
                // directly from the function
                auto *expectedTy =
                    freeFunc->getRawArguments().empty()
                        ? const_cast<hir::HIRModule *>(hirModule)
                              ->getPointerType(
                                  const_cast<hir::HIRModule *>(hirModule)
                                      ->getVoidType(),
                                  hir::Ownership::None)
                        : freeFunc->getRawArguments()[0]->getType();

                // The environment pointer is at index 1 of the closure struct
                castToVoid = builder->insert(std::make_unique<ExtractValueInst>(
                    loaded, 1, expectedTy, "env.free.ptr", loc));
              } else {
                // Standard Box/Pointer behavior
                if (!freeFunc->getRawArguments().empty()) {
                  castToVoid = builder->createBitCast(
                      loaded, freeFunc->getRawArguments()[0]->getType(),
                      "free.cast", loc);
                } else {
                  castToVoid = loaded;
                }
              }

              builder->insert(std::make_unique<CallInst>(
                  freeFunc, std::vector<MIRValue *>{castToVoid},
                  freeFunc->getType(), "", false, loc));
            }
          }
        }
      }
    };

    // 2. Drop and Free Unique/Owned Variables (LIFO)
    processDrops(scope.ownedVars, false);

    // 3. Release ARC Variables (LIFO)
    processDrops(scope.refCountedVars, true);
  }

  void emitAllDrops(SourceLocation loc) {
    for (size_t i = scopeStack.size(); i > 0; --i) {
      emitScopeCleanup(scopeStack[i - 1], loc);
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
  MIRValue *currentAsyncPromise = nullptr;
  std::stack<MIRBlock *> loopCondBlocks;
  std::stack<MIRBlock *> loopMergeBlocks;

  std::unordered_set<MIRValue *> volatileVars;

  // Traces a pointer back to its origin to determine if it points to volatile
  // memory
  bool isVolatilePointer(MIRValue *ptr) {
    if (!ptr)
      return false;

    // 1. Did it come from a volatile global? (e.g., SERIAL_STATUS_REG)
    if (auto *g = llvm::dyn_cast_or_null<MIRGlobal>(ptr))
      return g->isVolatile();

    // 2. Is it a tracked volatile local variable?
    if (volatileVars.count(ptr))
      return true;

    // 3. Follow casts backwards
    if (auto *cast = llvm::dyn_cast_or_null<CastInst>(ptr)) {
      if (llvm::dyn_cast_or_null<ConstantInt>(cast->getValue()))
        return true;
      return isVolatilePointer(cast->getValue());
    }

    // 4. Follow loads and GEPs backwards
    if (auto *load = llvm::dyn_cast_or_null<LoadInst>(ptr))
      return isVolatilePointer(load->getPointer());
    if (auto *gep = llvm::dyn_cast_or_null<GetElementPtrInst>(ptr))
      return isVolatilePointer(gep->getPointer());

    return false;
  }

  bool isIdentifierUsed(const hir::HIRStmt *stmt, const std::string &name) {
    if (!stmt)
      return false;

    // Capture the HIR dump into a string buffer
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

    // [NEW] O(1) strongly-typed MIR tagging! No more string matching!
    if (auto *ptrTy = llvm::dyn_cast<hir::PointerType>(hirType)) {
      if (ptrTy->isMut())
        mirVal->setBorrowKind(mir::BorrowKind::Mut);
      else if (ptrTy->isView())
        mirVal->setBorrowKind(mir::BorrowKind::View);
      else if (ptrTy->isLock())
        mirVal->setBorrowKind(mir::BorrowKind::Lock);
    } else if (auto *refTy = llvm::dyn_cast<hir::ReferenceType>(hirType)) {
      if (refTy->isMut())
        mirVal->setBorrowKind(mir::BorrowKind::Mut);
      else if (refTy->isView())
        mirVal->setBorrowKind(mir::BorrowKind::View);
      else if (refTy->isLock())
        mirVal->setBorrowKind(mir::BorrowKind::Lock);
    }
  }

  // --- [NEW] Helper to detect weak references in memory ---
  bool isWeakMemory(const hir::HIRType *ty) const {
    if (!ty)
      return false;

    // Peel off the pointer layer if we are looking at an Alloca/GEP address
    if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(ty)) {
      ty = ptrTy->getPointee();
    }

    if (!ty)
      return false;
    if (ty->getKind() == hir::TypeKind::Weak)
      return true;

    // Check string representation to catch Nullable<Weak<T>> wrappers
    return ty->toString().find("weak ") != std::string::npos;
  }

  void generateVTable(const hir::HIRClass *cls) {
    if (!cls->hasVTable())
      return;

    std::string vtableName = cls->getName() + ".vtable";
    if (mirModule->getGlobal(vtableName))
      return; // Already generated

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

        MIRFunction *mirF = mirModule->getFunction(mangledName);
        if (!mirF) {
          createFunctionDecl(m.get(), mangledName, thisTy);
          mirF = mirModule->getFunction(mangledName);
        }

        // [NEW] Use ConstantBitCast to uniformize the function pointer
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

    // [NEW] Use ConstantStruct to wrap RTTI + Func Array
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
    if (ty->getKind() == hir::TypeKind::Bool)
      return val;

    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();

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

  // Helper function to evaluate string escapes down to raw bytes
  std::string unescapeString(const std::string &in) {
    std::string out;
    for (size_t i = 0; i < in.length(); ++i) {
      if (in[i] == '\\' && i + 1 < in.length()) {
        i++; // Skip the backslash
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
          out += in[i]; // Unknown escape, just pass it through
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
    // Request a safe, unique name from the parent function
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

    // [FIX 1] Map HIR Weak flag to MIR Linkage
    Linkage linkage = hirFunc->isWeak() ? Linkage::Weak : Linkage::External;

    auto mirFunc = std::make_unique<MIRFunction>(
        resolveType(hirFunc->getReturnType()), mirName, linkage);

    // [FIX 2] Map HIR ABI to MIR Calling Conventions
    std::string abi = hirFunc->getABI();
    if (abi == "stdcall")
      mirFunc->setCallingConv(CallingConv::StdCall);
    else if (abi == "fastcall")
      mirFunc->setCallingConv(CallingConv::FastCall);

    // Interrupts override standard calling conventions
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

    // Inject the implicit 'this' pointer as the first parameter
    if (thisType) {
      auto arg = std::make_unique<MIRArgument>(mirFunc.get(),
                                               resolveType(thisType), idx++);
      arg->setName("this"); // Name the 'this' parameter explicitly
      applyBorrowKind(arg.get(), thisType);
      mirFunc->addArgument(std::move(arg));
    }

    // [FIX 3] Preserve parameter names to fix the `*u8 %` bug
    for (const auto &p : hirFunc->getParams()) {
      auto arg = std::make_unique<MIRArgument>(mirFunc.get(),
                                               resolveType(p.getType()), idx++);
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
      const hir::HIRType *actualType = varDecl->getType();

      if (initVal && initVal->getType() && initVal->getType() != actualType) {
        // Automatically promote literals to match the expected type
        if (auto *cInt = llvm::dyn_cast_or_null<ConstantInt>(initVal)) {
          valToStore = mirModule->getOrInsertConstant<ConstantInt>(
              cInt->getValue(), actualType);
        } else if (auto *cFloat =
                       llvm::dyn_cast_or_null<ConstantFloat>(initVal)) {
          valToStore = mirModule->getOrInsertConstant<ConstantFloat>(
              cFloat->getValue(), actualType);
        } else if (llvm::dyn_cast_or_null<ConstantNull>(initVal)) {
          valToStore = mirModule->getOrInsertConstant<ConstantNull>(actualType);
        } else {
          // Fallback: It's a dynamic value, emit a BitCast
          bool isDestStruct = actualType->getKind() == hir::TypeKind::Struct;
          bool isSrcPtr =
              initVal->getType()->getKind() == hir::TypeKind::Pointer ||
              initVal->getType()->getKind() == hir::TypeKind::Reference;

          if (isDestStruct && isSrcPtr) {
            auto *rawRefTy =
                const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                    actualType, hir::Ownership::Borrowed);
            auto *castPtr = builder->createBitCast(
                initVal, rawRefTy, "init.raw_ptr", varDecl->getLoc());
            valToStore = builder->insert(std::make_unique<LoadInst>(
                castPtr, "init.load", varDecl->getLoc()));
          } else {
            valToStore = builder->createBitCast(initVal, actualType,
                                                "init.cast", varDecl->getLoc());
          }
        }
      }

      Linkage linkage =
          varDecl->isStaticVar() ? Linkage::Internal : Linkage::External;

      bool isConstant = false;
      if (actualType && actualType->isImmutable()) {
        isConstant = true;
      }

      auto *global =
          builder->createGlobal(mirModule.get(), varDecl->getName(), actualType,
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
          builder->insert(std::make_unique<StoreInst>(valToStore, global,
                                                      varDecl->getLoc()));
        }
      } else {
        global->setExtern(varDecl->isExternVar());
      }
    }
  }

  // --- Visitor Implementations ---
  void visitFunction(const hir::HIRFunction &func) override {
    lowerFunction(func);
  }

  void lowerFunction(const hir::HIRFunction &func,
                     std::string overrideName = "",
                     const hir::HIRType *thisType = nullptr) {
    scopeStack.clear();
    scopeStack.push_back({});

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

    // [FIX] Explicitly map the implicit 'this' pointer if it exists
    if (thisType && argIdx < mirArgs.size()) {
      MIRArgument *thisArg = mirArgs[argIdx].get();
      auto *alloca =
          builder->createAlloca(thisArg->getType(), "this.addr", func.getLoc());
      builder->insert(
          std::make_unique<StoreInst>(thisArg, alloca, func.getLoc()));
      symbolMap["this"] = alloca;
      argIdx++;
    }

    // [FIX] Safely map the rest of the explicit parameters
    for (size_t i = 0; i < hirParams.size(); ++i) {
      if (argIdx >= mirArgs.size())
        break; // Safety check

      MIRArgument *arg = mirArgs[argIdx].get();
      auto *alloca = builder->createAlloca(resolveType(arg->getType()),
                                           hirParams[i].getName() + ".addr",
                                           hirParams[i].getLoc());

      builder->insert(
          std::make_unique<StoreInst>(arg, alloca, hirParams[i].getLoc()));
      symbolMap[hirParams[i].getName()] = alloca;
      argIdx++;
    }

    if (func.getBody()) {
      visit(func.getBody());
    }

    if (!getTerminator(builder->getInsertBlock())) {
      // --- NEW: Base Destructor Chaining ---
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
            // Clean up prefix spaces/symbols
            std::string pName = parentTy->toString();
            while (!pName.empty() && !isalnum(pName[0]))
              pName = pName.substr(1);

            std::string parentDtorName = pName + ".destructor";
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

      emitScopeCleanup(scopeStack.back(), func.getLoc());

      if (currFunc->isNoReturn()) {
        builder->insert(std::make_unique<UnreachableInst>(func.getLoc()));
      } else {
        // [FIX] Implicit Default Return for Non-Void Functions
        const hir::HIRType *expectedTy = currFunc->getType();
        if (expectedTy && expectedTy->getKind() != hir::TypeKind::Void) {
          MIRValue *defVal = nullptr;
          if (expectedTy->getKind() == hir::TypeKind::Int)
            defVal = mirModule->getOrInsertConstant<ConstantInt>(0, expectedTy);
          else if (expectedTy->getKind() == hir::TypeKind::Float ||
                   expectedTy->getKind() == hir::TypeKind::Decimal)
            defVal =
                mirModule->getOrInsertConstant<ConstantFloat>(0.0, expectedTy);
          else if (expectedTy->getKind() == hir::TypeKind::Bool)
            defVal =
                mirModule->getOrInsertConstant<ConstantBool>(false, expectedTy);
          else
            defVal = mirModule->getOrInsertConstant<ConstantNull>(expectedTy);

          builder->insert(std::make_unique<ReturnInst>(defVal, func.getLoc()));
        } else {
          builder->insert(std::make_unique<ReturnInst>(nullptr, func.getLoc()));
        }
      }
    }

    scopeStack.pop_back();
    currFunc->numberUnnamedValues();
  }

  void visitBlockStmt(const hir::BlockStmt &stmt) override {
    auto oldSymbolMap = symbolMap;
    scopeStack.push_back({}); // Push new scope boundary

    for (const auto &s : stmt.getStatements()) {
      visit(s.get());
      if (builder->getInsertBlock() && getTerminator(builder->getInsertBlock()))
        break;
    }

    // Clean up variables/defers when naturally leaving the block
    if (builder->getInsertBlock() &&
        !getTerminator(builder->getInsertBlock())) {
      emitScopeCleanup(scopeStack.back(), stmt.getLoc());
    }

    scopeStack.pop_back(); // Pop scope boundary
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

      visit(stmt.getReturnValue());

      expectedLambdaReturnType = oldExpected;
      retVal = lastExprValue;

      if (retVal && currFunc) {
        const hir::HIRType *expectedTy = currFunc->getType();
        if (expectedTy && expectedTy->getKind() != hir::TypeKind::Void &&
            retVal->getType() != expectedTy) {
          if (llvm::dyn_cast_or_null<ConstantNull>(retVal)) {
            retVal = mirModule->getOrInsertConstant<ConstantNull>(expectedTy);
          } else {
            retVal = builder->createBitCast(retVal, expectedTy, "ret.cast",
                                            stmt.getLoc());
          }
        }
      }
    }

    // [NEW] Identify the alloca we are returning so we don't free it!
    MIRValue *retOriginAlloca = nullptr;
    if (retVal) {
      if (auto *load = llvm::dyn_cast_or_null<LoadInst>(retVal)) {
        retOriginAlloca =
            load->getPointer(); // This is the alloca being returned
      }
    }

    // Iterating by index and copying the scope prevents vector reallocation
    // crashes.
    for (size_t i = scopeStack.size(); i > 0; --i) {
      LexicalScope scopeCopy = scopeStack[i - 1];

      // Protect the returned variable from being dropped
      if (retOriginAlloca) {
        auto &owned = scopeCopy.ownedVars;
        owned.erase(std::remove(owned.begin(), owned.end(), retOriginAlloca),
                    owned.end());

        auto &shared = scopeCopy.refCountedVars;
        shared.erase(std::remove(shared.begin(), shared.end(), retOriginAlloca),
                     shared.end());
      }

      emitScopeCleanup(scopeCopy, stmt.getLoc());
    }

    builder->insert(std::make_unique<ReturnInst>(retVal, stmt.getLoc()));
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
    if (!getTerminator(builder->getInsertBlock()))
      builder->createBr(mergeBlock);

    builder->setInsertPoint(elseBlock);
    if (stmt.getElseBranch())
      visit(stmt.getElseBranch());
    if (!getTerminator(builder->getInsertBlock()))
      builder->createBr(mergeBlock);

    builder->setInsertPoint(mergeBlock);
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

    if (!getTerminator(builder->getInsertBlock()))
      builder->createBr(condBlock);

    builder->setInsertPoint(mergeBlock);
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
  }

  void visitForInStmt(const hir::ForInStmt &stmt) override {
    auto oldSymbolMap = symbolMap;
    // 1. Evaluate the array/collection we are iterating over
    visit(stmt.getCollection());
    MIRValue *collection = lastExprValue;

    // 2. Allocate the loop variable (e.g., 'x')
    MIRValue *loopVar = nullptr;
    if (auto *varDecl = stmt.getVariable()) {
      loopVar = builder->createAlloca(varDecl->getType(), varDecl->getName(),
                                      varDecl->getLoc());
      symbolMap[varDecl->getName()] = loopVar;
    }

    // 3. Allocate a hidden index counter and initialize it to 0
    auto *intType =
        const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    MIRValue *indexAlloca =
        builder->createAlloca(intType, "forin.idx", stmt.getLoc());
    MIRValue *zero = mirModule->getOrInsertConstant<ConstantInt>(0, intType);
    builder->insert(
        std::make_unique<StoreInst>(zero, indexAlloca, stmt.getLoc()));

    // 4. Create the CFG Blocks
    MIRBlock *condBlock = newBlock("forin.cond");
    MIRBlock *bodyBlock = newBlock("forin.body");
    MIRBlock *incBlock = newBlock("forin.inc");
    MIRBlock *endBlock = newBlock("forin.end");

    builder->createBr(condBlock);

    // --- Condition Block ---
    builder->setInsertPoint(condBlock);
    MIRValue *currentIndex = builder->insert(
        std::make_unique<LoadInst>(indexAlloca, "idx.load", stmt.getLoc()));

    MIRValue *lengthVal = nullptr;
    if (collection->getType()->getKind() == hir::TypeKind::Slice) {
      // Extract the dynamic length from the Slice Fat Pointer (Index 1)
      lengthVal = builder->insert(std::make_unique<ExtractValueInst>(
          collection, 1, intType, "slice.len", stmt.getLoc()));
    } else if (auto *arrTy = llvm::dyn_cast_or_null<hir::ArrayType>(
                   collection->getType())) {
      // Fixed size array
      lengthVal = mirModule->getOrInsertConstant<ConstantInt>(arrTy->getSize(),
                                                              intType);
    } else {
      // Fallback
      lengthVal = mirModule->getOrInsertConstant<ConstantInt>(0, intType);
    }

    // Compare: currentIndex < lengthVal
    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();
    MIRValue *loopCond =
        builder->createICmp(CompareInst::Predicate::LT, currentIndex, lengthVal,
                            boolTy, "forin.cmp", stmt.getLoc());

    builder->createCondBr(loopCond, bodyBlock, endBlock);

    // --- Body Block ---
    builder->setInsertPoint(bodyBlock);

    // [FIX] Auto-extract the item from the array into 'x'
    if (collection && loopVar) {
      // Pass arguments in the correct order, and wrap the index in a
      // std::vector
      auto *gep = builder->createGEP(collection, {currentIndex},
                                     stmt.getVariable()->getType(), "elem.ptr",
                                     stmt.getLoc());

      auto *loadedElem = builder->insert(
          std::make_unique<LoadInst>(gep, "elem.val", stmt.getLoc()));

      builder->insert(
          std::make_unique<StoreInst>(loadedElem, loopVar, stmt.getLoc()));
    }

    if (stmt.getBody()) {
      continueScopeDepths.push(scopeStack.size());
      breakScopeDepths.push(scopeStack.size());

      visit(stmt.getBody());

      continueScopeDepths.pop();
      breakScopeDepths.pop();
    }

    if (!getTerminator(builder->getInsertBlock())) {
      builder->createBr(incBlock);
    }

    // --- Increment Block ---
    builder->setInsertPoint(incBlock);

    // Increment the hidden counter
    MIRValue *loadedIdx = builder->insert(
        std::make_unique<LoadInst>(indexAlloca, "idx.inc.load", stmt.getLoc()));
    MIRValue *oneVal = mirModule->getOrInsertConstant<ConstantInt>(1, intType);
    MIRValue *nextIdx =
        builder->createAdd(loadedIdx, oneVal, "idx.add", stmt.getLoc());
    builder->insert(
        std::make_unique<StoreInst>(nextIdx, indexAlloca, stmt.getLoc()));

    builder->createBr(condBlock);

    // --- End Block ---
    builder->setInsertPoint(endBlock);
    symbolMap = oldSymbolMap;
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

    // 1. Identify the Default case from the cases vector
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

    // 2. Map blocks to their constants and execution bodies
    // [FIX] Track massive ranges separately to prevent OOM
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
        continue; // Skip the default case in this loop

      MIRBlock *caseBlock = newBlock("switch.case");
      CaseMapping mapping;
      mapping.block = caseBlock;
      mapping.body = &c.getBody();

      for (const auto &valExpr : c.getValues()) {
        // Intercept Range Expressions
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

              // [FIX] Threshold check! If the range is > 64 elements, track it
              // dynamically. Otherwise, safely unroll it into the dense jump
              // table.
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

        // Standard single-value case
        visit(valExpr.get());
        if (lastExprValue) {
          MIRValue *caseVal = lastExprValue;

          // Unwrap any Cast instructions injected by implicit AST conversions
          if (auto *castInst = llvm::dyn_cast_or_null<CastInst>(caseVal)) {
            caseVal = castInst->getValue();
          }
          // Intercept Enum Variants loaded as Globals
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

    // 3. Create CFG Routing (Dynamic Ranges FIRST, then Discrete Switch)
    MIRBlock *switchHeaderBlock = newBlock("switch.header");
    MIRBlock *currentBlock = builder->getInsertBlock();
    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();

    // --- [FIX] Emit bounds-checking blocks for large ranges ---
    for (const auto &m : mappings) {
      for (const auto &range : m.rangeValues) {
        MIRBlock *nextCheckBlock = newBlock("switch.next_range");

        builder->setInsertPoint(currentBlock);
        auto *startConst = mirModule->getOrInsertConstant<ConstantInt>(
            range.start, condVal->getType());
        auto *endConst = mirModule->getOrInsertConstant<ConstantInt>(
            range.end, condVal->getType());

        // Evaluate: if (condVal >= start && condVal <= end)
        MIRValue *ge =
            builder->createICmp(CompareInst::Predicate::GE, condVal, startConst,
                                boolTy, "range.ge", stmt.getLoc());
        MIRValue *le =
            builder->createICmp(CompareInst::Predicate::LE, condVal, endConst,
                                boolTy, "range.le", stmt.getLoc());
        MIRValue *inRange = builder->insert(std::make_unique<BinaryInst>(
            Opcode::And, ge, le, "range.and", stmt.getLoc()));

        builder->createCondBr(inRange, m.block, nextCheckBlock);

        // Update CFG Edges
        currentBlock->addSuccessor(m.block);
        m.block->addPredecessor(currentBlock);
        currentBlock->addSuccessor(nextCheckBlock);
        nextCheckBlock->addPredecessor(currentBlock);

        currentBlock = nextCheckBlock;
      }
    }

    // Fallthrough: If no dynamic ranges matched, jump to the discrete
    // jump-table
    builder->setInsertPoint(currentBlock);
    builder->createBr(switchHeaderBlock);
    currentBlock->addSuccessor(switchHeaderBlock);
    switchHeaderBlock->addPredecessor(currentBlock);

    // --- Emit the standard Switch Instruction ---
    builder->setInsertPoint(switchHeaderBlock);
    auto *switchInst =
        builder->createSwitch(condVal, defaultBlock, stmt.getLoc());
    for (const auto &m : mappings) {
      for (MIRValue *v : m.constValues) {
        switchInst->addCase(v, m.block);
        builder->getInsertBlock()->addSuccessor(m.block);
        m.block->addPredecessor(builder->getInsertBlock());
      }
    }

    loopMergeBlocks.push(mergeBlock);
    breakScopeDepths.push(scopeStack.size());

    // 4. Emit the actual code for each Case block
    for (size_t i = 0; i < mappings.size(); ++i) {
      const auto &m = mappings[i];
      builder->setInsertPoint(m.block);

      // Determine if the case is empty to decide between Fallthrough and
      // Implicit Break
      bool isEmpty = true;
      if (m.body) {
        if (auto *block = llvm::dyn_cast_or_null<hir::BlockStmt>(m.body)) {
          isEmpty = block->getStatements().empty();
        } else {
          // If it's a single statement (not a block), it's not empty
          isEmpty = false;
        }
        visit(m.body);
      }

      if (!getTerminator(builder->getInsertBlock())) {
        // [FIX] Conditional Fallthrough Logic:
        if (isEmpty) {
          // EMPTY CASE: Branch to the next available block (Fallthrough)
          if (i < mappings.size() - 1) {
            builder->createBr(mappings[i + 1].block);
          } else if (actualDefaultBlock) {
            builder->createBr(actualDefaultBlock);
          } else {
            builder->createBr(mergeBlock);
          }
        } else {
          // HAS STATEMENTS: Implicit Break
          builder->createBr(mergeBlock);
        }
      }
    }

    // 5. Emit the actual code for the Default block
    if (actualDefaultBlock && defaultASTCase) {
      builder->setInsertPoint(actualDefaultBlock);
      visit(&defaultASTCase->getBody());

      // Default case always performs an implicit break to switch.end
      if (!getTerminator(builder->getInsertBlock())) {
        builder->createBr(mergeBlock);
      }
    }

    loopMergeBlocks.pop();
    breakScopeDepths.pop();
    builder->setInsertPoint(mergeBlock);
  }

  void visitBreakStmt(const hir::BreakStmt &stmt) override {
    if (!loopMergeBlocks.empty() && !breakScopeDepths.empty()) {
      size_t targetDepth = breakScopeDepths.top();
      // Clean up scopes down to the loop's outer boundary
      for (size_t i = scopeStack.size(); i > targetDepth; --i) {
        emitScopeCleanup(scopeStack[i - 1], stmt.getLoc());
      }
      builder->createBr(loopMergeBlocks.top());
    }
  }

  void visitContinueStmt(const hir::ContinueStmt &stmt) override {
    if (!loopCondBlocks.empty() && !continueScopeDepths.empty()) {
      size_t targetDepth = continueScopeDepths.top();
      // Clean up scopes down to the loop's outer boundary
      for (size_t i = scopeStack.size(); i > targetDepth; --i) {
        emitScopeCleanup(scopeStack[i - 1], stmt.getLoc());
      }
      builder->createBr(loopCondBlocks.top());
    }
  }

  void visitDeferStmt(const hir::DeferStmt &stmt) override {
    if (!scopeStack.empty()) {
      scopeStack.back().deferredStmts.push_back(stmt.getDeferredStmt());
    }
  }

  void visitLockStmt(const hir::LockStmt &stmt) override {
    // 1. Evaluate the mutex target
    MIRValue *mutexPtr = nullptr;

    if (stmt.getMutex()) {
      // 1. Evaluate the expression normally
      visit(stmt.getMutex());
      mutexPtr = lastExprValue;

      // 2. Are we holding a memory address (L-Value)?
      if (stmt.getMutex()->getValueCategory() == hir::ValueCategory::LValue) {

        if (mutexPtr &&
            mutexPtr->getType()->getKind() == hir::TypeKind::Pointer) {
          auto *ptrTy =
              static_cast<const hir::PointerType *>(mutexPtr->getType());

          // 3. If the allocated data is ITSELF a pointer (e.g., `lock i32* p`),
          //    we hold `**i32`. We MUST load it to get the `*i32` target!
          if (ptrTy->getPointee()->getKind() == hir::TypeKind::Pointer ||
              ptrTy->getPointee()->getKind() == hir::TypeKind::Reference) {

            mutexPtr =
                builder->createLoad(mutexPtr, "mutex.load", stmt.getLoc());
          }
        }
      }
    }

    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);

    // 2. Emit the lock acquisition
    if (mutexPtr) {
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

    // 3. Evaluate the synchronized block
    if (stmt.getBody()) {
      visit(stmt.getBody());
    }

    // 4. Emit the lock release
    if (mutexPtr) {
      MIRValue *castMutex = builder->createBitCast(
          mutexPtr, voidPtrTy, "mutex.unlock.cast", stmt.getLoc());

      MIRFunction *unlockFunc = mirModule->getFunction("__moksha_unlock");
      if (!unlockFunc) {
        auto fn = std::make_unique<MIRFunction>(voidTy, "__moksha_unlock",
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0));
        unlockFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      builder->createCall(unlockFunc, {castMutex}, voidTy, "", false,
                          stmt.getLoc());
    }
  }

  void visitExprStmt(const hir::ExprStmt &stmt) override {
    if (stmt.getExpr())
      visit(stmt.getExpr());
  }

  void visitTryCatchStmt(const hir::TryCatchStmt &stmt) override {
    auto oldSymbolMap = symbolMap;

    // [FIX 2a] Allocate the exception slot up front and push to stack
    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);
    auto *exSlot = builder->createAlloca(voidPtrTy, "ex.slot", stmt.getLoc());
    currentExceptionSlots.push(exSlot);

    MIRBlock *tryBodyBlock = newBlock("try.body");
    MIRBlock *cleanupBlock = newBlock("try.cleanup");
    MIRBlock *catchBlock = newBlock("catch");
    MIRBlock *contBlock = newBlock("try.cont");

    builder->createBr(tryBodyBlock);

    // --- Try Body ---
    builder->setInsertPoint(tryBodyBlock);

    MIRBlock *oldUnwind = currentUnwindDest;
    currentUnwindDest = cleanupBlock;

    if (stmt.getTryBody()) {
      tryScopeDepths.push(scopeStack.size());
      visit(stmt.getTryBody());
      tryScopeDepths.pop();
    }

    currentUnwindDest = oldUnwind;

    if (!getTerminator(builder->getInsertBlock())) {
      builder->createBr(contBlock);
    }

    // --- Cleanup Pad Block ---
    builder->setInsertPoint(cleanupBlock);
    builder->createBr(catchBlock);

    // --- Catch Block ---
    builder->setInsertPoint(catchBlock);

    const hir::HIRType *catchType = nullptr;
    if (stmt.getCatchVar()) {
      catchType = stmt.getCatchVar()->getType();
    }

    if (!catchType) {
      catchType = voidPtrTy;
    }

    // [FIX 2b] Load from the slot instead of creating a second landingpad!
    MIRValue *lpad = builder->createLoad(exSlot, "ex.val", stmt.getLoc());

    if (stmt.getCatchVar()) {
      auto *catchIdent =
          llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(stmt.getCatchVar());
      std::string catchName = catchIdent ? catchIdent->getName() : "catch_var";

      auto *alloca = builder->createAlloca(catchType, catchName,
                                           stmt.getCatchVar()->getLoc());
      symbolMap[catchName] = alloca;

      // Cast if the catch type isn't a raw void*
      MIRValue *valToStore = lpad;
      if (lpad->getType() != catchType) {
        valToStore =
            builder->createBitCast(lpad, catchType, "ex.cast", stmt.getLoc());
      }
      builder->insert(
          std::make_unique<StoreInst>(valToStore, alloca, stmt.getLoc()));
    }

    if (stmt.getCatchBody()) {
      visit(stmt.getCatchBody());
    }

    if (!getTerminator(builder->getInsertBlock())) {
      builder->createBr(contBlock);
    }

    builder->setInsertPoint(contBlock);
    if (stmt.getFinallyBody()) {
      visit(stmt.getFinallyBody());
    }

    // [FIX 2c] Clean up the stack
    currentExceptionSlots.pop();
    symbolMap = oldSymbolMap;
  }

  void visitThrowStmt(const hir::HIRThrowStmt &stmt) override {
    visit(stmt.getExpr());
    MIRValue *exVal = lastExprValue;

    // Prevent Infinite Recursion!
    MIRBlock *savedUnwind = currentUnwindDest;
    currentUnwindDest = nullptr;

    size_t targetDepth = tryScopeDepths.empty() ? 0 : tryScopeDepths.top();
    for (size_t i = scopeStack.size(); i > targetDepth; --i) {
      emitScopeCleanup(scopeStack[i - 1], stmt.getLoc());
    }

    currentUnwindDest = savedUnwind; // Restore unwind dest

    if (currentUnwindDest) {
      builder->insert(
          std::make_unique<ThrowInst>(exVal, currentUnwindDest, stmt.getLoc()));
    } else {
      builder->insert(
          std::make_unique<ThrowInst>(exVal, nullptr, stmt.getLoc()));
    }
  }

  void visitAsmStmt(const hir::HIRAsmStmt &stmt) override {
    builder->createInlineAsm(stmt.getAssemblyStr(), stmt.getConstraints(), {},
                             nullptr, stmt.getLoc());
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

    // Unwrap Reference Initializers
    if (varDecl->getType() &&
        varDecl->getType()->getKind() == hir::TypeKind::Reference) {
      if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(initVal)) {
        // We want the memory address (pointer), not the loaded value!
        initVal = loadInst->getPointer();
      }
    }

    MIRValue *valToStore = initVal;
    const hir::HIRType *actualType = resolveType(varDecl->getType());

    if (!actualType) {
      if (initVal && initVal->getType()) {
        actualType = initVal->getType();
      } else {
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        actualType = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            voidTy, hir::Ownership::None);
      }
    }

    // ConstantNull can properly enter the coercion block!
    if (initVal && initVal->getType() != actualType) {

      // CONSTANT COERCION
      if (auto *cInt = llvm::dyn_cast_or_null<ConstantInt>(initVal)) {
        valToStore = mirModule->getOrInsertConstant<ConstantInt>(
            cInt->getValue(), actualType);
      } else if (auto *cFloat =
                     llvm::dyn_cast_or_null<ConstantFloat>(initVal)) {
        valToStore = mirModule->getOrInsertConstant<ConstantFloat>(
            cFloat->getValue(), actualType);
      } else if (llvm::dyn_cast_or_null<ConstantNull>(initVal)) {
        // [FIX] Coerce the Null pointer to the expected Variable Type
        valToStore = mirModule->getOrInsertConstant<ConstantNull>(actualType);
      } else {
        bool isDestStruct = actualType->getKind() == hir::TypeKind::Struct;

        // Guard against null types for dynamic values just in case
        bool isSrcPtr =
            initVal->getType() &&
            (initVal->getType()->getKind() == hir::TypeKind::Pointer ||
             initVal->getType()->getKind() == hir::TypeKind::Reference);

        if (isSrcPtr && actualType->getKind() != hir::TypeKind::Pointer &&
            actualType->getKind() != hir::TypeKind::Reference) {
          auto *rawRefTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  actualType, isDestStruct ? hir::Ownership::Borrowed
                                           : hir::Ownership::None);
          auto *castPtr = builder->createBitCast(
              initVal, rawRefTy, "init.raw_ptr", varDecl->getLoc());
          valToStore = builder->insert(std::make_unique<LoadInst>(
              castPtr, "init.load", varDecl->getLoc()));
        } else {
          valToStore = builder->createBitCast(initVal, actualType, "init.cast",
                                              varDecl->getLoc());
          applyBorrowKind(valToStore, actualType);
        }
      }
    }

    auto *alloca = builder->createAlloca(actualType, varDecl->getName(),
                                         varDecl->getLoc());
    applyBorrowKind(alloca, actualType);
    if (stmt.isVolatileVar()) {
      volatileVars.insert(alloca);
    }
    symbolMap[varDecl->getName()] = alloca;

    if (initVal) {
      // Use StoreWeak for weak variable initializations
      if (isWeakMemory(actualType)) {
        builder->createStoreWeak(valToStore, alloca, varDecl->getLoc());
      } else {
        builder->insert(
            std::make_unique<StoreInst>(valToStore, alloca, varDecl->getLoc()));
        if (isVolatilePointer(valToStore)) {
          volatileVars.insert(alloca);
        }
      }
    }

    if (!scopeStack.empty() && actualType) {
      bool isShared = false;
      bool isOwned = false;

      // 1. Check explicit pointer ownership first!
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(actualType)) {
        if (ptrTy->getOwnership() == hir::Ownership::Shared) {
          isShared = true;
        } else if (ptrTy->getOwnership() == hir::Ownership::Owned ||
                   ptrTy->getOwnership() == hir::Ownership::None) {
          // If it is a standard class/struct allocated via `new`, we own it
          if (ptrTy->getPointee() &&
              ptrTy->getPointee()->getKind() == hir::TypeKind::Struct) {
            isOwned = true;
          }
        }
      } else if (llvm::dyn_cast_or_null<hir::ReferenceType>(actualType)) {
        // References are always borrowed, do nothing.
      }
      // 2. Only string match if it is an actual value type!
      else {
        std::string tyStr = actualType->toString();
        if (tyStr.find("Arc<") != std::string::npos ||
            tyStr.find("shared ") != std::string::npos) {
          isShared = true;
        } else if (tyStr.find("Box<") != std::string::npos ||
                   tyStr.find("owned ") != std::string::npos) {
          isOwned = true;
        } else if (actualType->getKind() == hir::TypeKind::Struct ||
                   actualType->getKind() == hir::TypeKind::Closure ||
                   tyStr.find("closure") != std::string::npos) {
          isOwned = true; // Value structs on the stack
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
            // Fallback for complex LHS like `array[0].bitfield`
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

          // 1. Resolve types correctly via hirModule
          auto *i32Ty =
              const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
          auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
          auto *idx = mirModule->getOrInsertConstant<ConstantInt>(
              memberInfo.index, i32Ty);

          // 2. Explicit Vector to resolve C++ template deduction ambiguity
          std::vector<MIRValue *> gepIndices = {zero, idx};

          // 3. GEP to the specific container field
          MIRValue *containerPtr = builder->createGEP(
              objPtr, gepIndices, memExpr->getType(), "bf.gep", expr.getLoc());

          // 4. Load the old container
          MIRValue *oldVal =
              builder->createLoad(containerPtr, "bf.load", expr.getLoc());

          // 5. Calculate Masks dynamically from AST Info
          uint64_t fieldMask = (1ULL << memberInfo.bitWidth) - 1;
          uint64_t clearMask = ~(fieldMask << memberInfo.bitOffset);

          // 6. Clear old bits
          auto *clearConst = mirModule->getOrInsertConstant<ConstantInt>(
              clearMask, oldVal->getType());
          MIRValue *cleared =
              builder->createAnd(oldVal, clearConst, "bf.clear", expr.getLoc());

          // 7. Cast and Shift the new value safely
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

          // 8. Combine and Store
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
      } else if (auto *deref =
                     llvm::dyn_cast_or_null<hir::HIRDerefExpr>(expr.getLHS())) {
        visit(deref->getPointer());
        lhsPtr = lastExprValue;
      } else {
        lhsPtr = evaluateAsLValue(expr.getLHS());
      }

      const hir::HIRType *oldExpected = expectedLambdaReturnType;
      if (lhsPtr && lhsPtr->getType()) {
        if (auto *pTy =
                llvm::dyn_cast_or_null<hir::PointerType>(lhsPtr->getType())) {
          expectedLambdaReturnType = pTy->getPointee();
        }
      }

      visit(expr.getRHS());
      expectedLambdaReturnType = oldExpected;

      MIRValue *rhs = lastExprValue;

      if (lhsPtr && rhs) {
        const hir::HIRType *expectedTy = lhsPtr->getType();
        if (auto *ptrTy =
                llvm::dyn_cast_or_null<hir::PointerType>(expectedTy)) {
          expectedTy = ptrTy->getPointee();
        }

        if (expectedTy && rhs->getType()) {
          if (auto *pTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(expectedTy)) {
            if (pTy->getOwnership() != hir::Ownership::None &&
                pTy->getPointee() == rhs->getType()) {
              lhsPtr = builder->insert(std::make_unique<LoadInst>(
                  lhsPtr, "assign.deref", expr.getLoc()));
              expectedTy = pTy->getPointee();
            }
          } else if (auto *rTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                         expectedTy)) {
            if (rTy->getInner() == rhs->getType()) {
              lhsPtr = builder->insert(std::make_unique<LoadInst>(
                  lhsPtr, "assign.deref", expr.getLoc()));
              expectedTy = rTy->getInner();
            }
          }
        }

        if (expectedTy && rhs->getType() && expectedTy != rhs->getType()) {
          if (auto *cInt = llvm::dyn_cast_or_null<ConstantInt>(rhs)) {
            rhs = mirModule->getOrInsertConstant<ConstantInt>(cInt->getValue(),
                                                              expectedTy);
          } else if (auto *cFloat =
                         llvm::dyn_cast_or_null<ConstantFloat>(rhs)) {
            rhs = mirModule->getOrInsertConstant<ConstantFloat>(
                cFloat->getValue(), expectedTy);
          } else {
            bool isDestStruct = expectedTy->getKind() == hir::TypeKind::Struct;
            bool isSrcPtr =
                rhs->getType()->getKind() == hir::TypeKind::Pointer ||
                rhs->getType()->getKind() == hir::TypeKind::Reference;

            if (isSrcPtr && expectedTy->getKind() != hir::TypeKind::Pointer &&
                expectedTy->getKind() != hir::TypeKind::Reference) {
              auto *rawRefTy =
                  const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                      expectedTy, isDestStruct ? hir::Ownership::Borrowed
                                               : hir::Ownership::None);
              auto *castPtr = builder->createBitCast(
                  rhs, rawRefTy, "assign.raw_ptr", expr.getLoc());
              rhs = builder->insert(std::make_unique<LoadInst>(
                  castPtr, "assign.load", expr.getLoc()));
            } else {
              if (rhs->getType() != expectedTy) {
                rhs = builder->createBitCast(rhs, expectedTy, "assign.coerce",
                                             expr.getLoc());
                applyBorrowKind(rhs, expectedTy);
              }
            }
          }
        }

        if (isWeakMemory(expectedTy)) {
          builder->createStoreWeak(rhs, lhsPtr, expr.getLoc());
        } else {
          auto *storeInst = builder->insert(
              std::make_unique<StoreInst>(rhs, lhsPtr, expr.getLoc()));
          if (isVolatilePointer(lhsPtr)) {
            storeInst->setVolatile(true);
          }
          if (isVolatilePointer(rhs)) {
            volatileVars.insert(lhsPtr);
          }
        }
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

        MIRBlock *lhsBlock = builder->getInsertBlock();
        MIRBlock *rhsBlock = newBlock("nullcoal.rhs");
        MIRBlock *mergeBlock = newBlock("nullcoal.end");

        // 1. Check if LHS is not null
        auto *nullConst =
            mirModule->getOrInsertConstant<ConstantNull>(lhsVal->getType());
        const hir::HIRType *boolTy =
            const_cast<hir::HIRModule *>(hirModule)->getBoolType();
        MIRValue *isNotNull =
            builder->createICmp(CompareInst::Predicate::NE, lhsVal, nullConst,
                                boolTy, "notnull", expr.getLoc());

        // 2. Branch: If not null, skip to merge. If null, go to RHS.
        builder->createCondBr(isNotNull, mergeBlock, rhsBlock);

        // 3. Evaluate RHS
        builder->setInsertPoint(rhsBlock);
        visit(expr.getRHS());
        MIRValue *rhsVal = lastExprValue;
        MIRBlock *rhsEndBlock = builder->getInsertBlock();
        builder->createBr(mergeBlock);

        // 4. Merge Block & Phi Node
        builder->setInsertPoint(mergeBlock);
        const hir::HIRType *resTy = expr.getType();
        if (!resTy || resTy->getKind() == hir::TypeKind::Void) {
          resTy = rhsVal->getType();
        }

        // Unwrap LHS from Nullable to underlying type
        MIRValue *lhsCast = lhsVal;
        if (lhsVal->getType() != resTy) {
          lhsCast = builder->createBitCast(lhsVal, resTy, "unwrap.cast",
                                           expr.getLoc());
        }

        // Ensure RHS matches the exact result type
        MIRValue *rhsCast = rhsVal;
        if (rhsVal->getType() != resTy) {
          rhsCast =
              builder->createBitCast(rhsVal, resTy, "rhs.cast", expr.getLoc());
        }

        auto *phi = builder->createPhi(resTy, "nullcoal.phi", expr.getLoc());
        phi->addIncoming(lhsCast, lhsBlock);
        phi->addIncoming(rhsCast, rhsEndBlock);

        lastExprValue = phi;
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

        // [FIX] Hardcode the Phi and Constant types to be strictly Bool!
        auto *phi = builder->createPhi(boolTy, "land.phi", expr.getLoc());
        phi->addIncoming(
            mirModule->getOrInsertConstant<ConstantBool>(false, boolTy),
            lhsBlock);
        phi->addIncoming(rhsVal, rhsEndBlock);

        lastExprValue = phi;
        return;
      } else {
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
        phi->addIncoming(
            mirModule->getOrInsertConstant<ConstantBool>(true, boolTy),
            lhsBlock);
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

    // Implicit Dereference for Managed Pointers
    auto autoDeref = [&](MIRValue *val) -> MIRValue * {
      if (!val || !val->getType())
        return val;
      const hir::HIRType *ty = val->getType();
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(ty)) {
        // Dereference Shared/Owned pointers, unless they are structs
        if (ptrTy->getOwnership() != hir::Ownership::None &&
            ptrTy->getPointee()->getKind() != hir::TypeKind::Struct) {
          return builder->insert(
              std::make_unique<LoadInst>(val, "auto.deref", expr.getLoc()));
        }
      } else if (auto *refTy = llvm::dyn_cast_or_null<hir::ReferenceType>(ty)) {
        if (refTy->getInner()->getKind() != hir::TypeKind::Struct) {
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
          // Ensure 'this' argument is passed by pointer
          MIRValue *thisArg = lhs;
          if (lhs->getType() &&
              lhs->getType()->getKind() != hir::TypeKind::Pointer) {
            thisArg =
                builder->createAlloca(lhs->getType(), "op.this", expr.getLoc());
            builder->createStore(lhs, thisArg, expr.getLoc());
          }

          // Bitcast to match the exact ownership semantics (None -> Borrowed)
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

    // POINTER ARITHMETIC INTERCEPTOR
    bool lhsIsPtr = lhs->getType() &&
                    (lhs->getType()->getKind() == hir::TypeKind::Pointer ||
                     lhs->getType()->getKind() == hir::TypeKind::Reference);
    bool rhsIsPtr = rhs->getType() &&
                    (rhs->getType()->getKind() == hir::TypeKind::Pointer ||
                     rhs->getType()->getKind() == hir::TypeKind::Reference);

    if (lhsIsPtr || rhsIsPtr) {
      if (expr.getOp() == hir::BinaryOp::Add) {
        // Pointer + Integer
        MIRValue *ptr = lhsIsPtr ? lhs : rhs;
        MIRValue *idx = lhsIsPtr ? rhs : lhs;

        const hir::HIRType *pointeeTy = nullptr;
        if (auto *pTy =
                llvm::dyn_cast_or_null<hir::PointerType>(ptr->getType())) {
          pointeeTy = pTy->getPointee();
        } else if (auto *rTy = llvm::dyn_cast_or_null<hir::ReferenceType>(
                       ptr->getType())) {
          pointeeTy = rTy->getInner();
        }

        lastExprValue =
            builder->createGEP(ptr, {idx}, pointeeTy, "ptr.add", expr.getLoc());
        return;
      } else if (expr.getOp() == hir::BinaryOp::Sub) {
        if (lhsIsPtr && !rhsIsPtr) {
          // Pointer - Integer -> GEP with negative index
          auto *zero =
              mirModule->getOrInsertConstant<ConstantInt>(0, rhs->getType());
          MIRValue *negIdx =
              builder->createSub(zero, rhs, "neg.idx", expr.getLoc());

          const hir::HIRType *pointeeTy = nullptr;
          if (auto *pTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(lhs->getType())) {
            pointeeTy = pTy->getPointee();
          }

          lastExprValue = builder->createGEP(lhs, {negIdx}, pointeeTy,
                                             "ptr.sub", expr.getLoc());
          return;
        } else if (lhsIsPtr && rhsIsPtr) {
          // Pointer - Pointer -> (int(ptr1) - int(ptr2)) / sizeof(T)
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
                  llvm::dyn_cast_or_null<hir::PointerType>(lhs->getType())) {
            pointeeTy = pTy->getPointee();
          }

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
            lastExprValue = byteDiff; // Fallback for void* subtraction
          }
          return;
        }
      }
    }

    // Detect floating point types for correct opcode dispatch
    bool isFloat = false;
    if (lhs->getType() &&
        (lhs->getType()->getKind() == hir::TypeKind::Float ||
         lhs->getType()->getKind() == hir::TypeKind::Decimal)) {
      isFloat = true;
    }

    // UNIVERSAL COERCION BLOCK FOR BINARY OPERANDS
    if (lhs->getType() != rhs->getType()) {
      if (llvm::dyn_cast_or_null<ConstantNull>(rhs) && lhs->getType()) {
        rhs = mirModule->getOrInsertConstant<ConstantNull>(lhs->getType());
      } else if (llvm::dyn_cast_or_null<ConstantNull>(lhs) && rhs->getType()) {
        lhs = mirModule->getOrInsertConstant<ConstantNull>(rhs->getType());
      } else if (lhs->getType() && rhs->getType()) {
        if (auto *cIntRhs = llvm::dyn_cast_or_null<ConstantInt>(rhs)) {
          rhs = mirModule->getOrInsertConstant<ConstantInt>(cIntRhs->getValue(),
                                                            lhs->getType());
        } else if (auto *cIntLhs = llvm::dyn_cast_or_null<ConstantInt>(lhs)) {
          lhs = mirModule->getOrInsertConstant<ConstantInt>(cIntLhs->getValue(),
                                                            rhs->getType());
        } else if (auto *cFloatRhs =
                       llvm::dyn_cast_or_null<ConstantFloat>(rhs)) {
          rhs = mirModule->getOrInsertConstant<ConstantFloat>(
              cFloatRhs->getValue(), lhs->getType());
        } else if (auto *cFloatLhs =
                       llvm::dyn_cast_or_null<ConstantFloat>(lhs)) {
          lhs = mirModule->getOrInsertConstant<ConstantFloat>(
              cFloatLhs->getValue(), rhs->getType());
        } else {
          // Fallback: Cast RHS to match LHS type
          rhs = builder->createBitCast(rhs, lhs->getType(), "bin.cast",
                                       expr.getLoc());
        }
      }
    }

    const hir::HIRType *boolTy =
        const_cast<hir::HIRModule *>(hirModule)->getBoolType();

    switch (expr.getOp()) {
    // --- Math Operators (Int vs Float Separation) ---
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

    // --- Bitwise Operators ---
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
    case hir::BinaryOp::Pow:
      lastExprValue = builder->insert(std::make_unique<BinaryInst>(
          Opcode::Pow, lhs, rhs, "", expr.getLoc()));
      break;

      // --- Comparison Operators (ICmp vs FCmp Separation) ---
    case hir::BinaryOp::Equal:
      if (isFloat)
        lastExprValue = builder->createFCmp(FCmpInst::Predicate::OEQ, lhs, rhs,
                                            boolTy, "", expr.getLoc());
      else
        lastExprValue = builder->createICmp(CompareInst::Predicate::EQ, lhs,
                                            rhs, boolTy, "", expr.getLoc());
      break;
    case hir::BinaryOp::NotEqual:
      if (isFloat)
        lastExprValue = builder->createFCmp(FCmpInst::Predicate::ONE, lhs, rhs,
                                            boolTy, "", expr.getLoc());
      else
        lastExprValue = builder->createICmp(CompareInst::Predicate::NE, lhs,
                                            rhs, boolTy, "", expr.getLoc());
      break;
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
      // Safely grab the memory pointer (L-Value) for variables
      if (auto *ident = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(
              expr.getOperand())) {
        std::string name = ident->getName();
        if (symbolMap.count(name))
          ptr = symbolMap[name];
        else
          ptr = mirModule->getGlobal(name);
      } else {
        // Fallback: evaluate complex memory access (like array[0]++)
        visit(expr.getOperand());
        if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(lastExprValue)) {
          ptr = loadInst->getPointer();
        } else {
          ptr = lastExprValue;
        }
      }

      if (!ptr)
        return;

      // Load the current value
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

      // Store the mutated value back into memory
      builder->insert(std::make_unique<StoreInst>(newVal, ptr, expr.getLoc()));

      // Return the correct temporal value
      if (op == hir::UnaryOp::PostInc || op == hir::UnaryOp::PostDec) {
        lastExprValue = loaded; // Post-fix evaluates to the old value
      } else {
        lastExprValue = newVal; // Pre-fix evaluates to the mutated value
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

        // Unary operators have no RHS arguments, so we pass an empty list
        std::string mangledName = mangleName(className + "." + opName, {});
        MIRFunction *opFunc = mirModule->getFunction(mangledName);

        if (opFunc) {
          // Ensure 'this' argument is passed by pointer
          MIRValue *thisArg = val;
          if (val->getType() &&
              val->getType()->getKind() != hir::TypeKind::Pointer) {
            thisArg =
                builder->createAlloca(val->getType(), "op.this", expr.getLoc());
            builder->createStore(val, thisArg, expr.getLoc());
          }

          // Bitcast to match the exact ownership semantics (None -> Borrowed)
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

      // Mask the constant to match the exact bit-width of the target
      uint64_t mask = ~0ULL;
      if (auto *intTy = llvm::dyn_cast<hir::HIRIntType>(operand->getType())) {
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
      // Arithmetic Negation: 0 - val
      auto *zero =
          mirModule->getOrInsertConstant<ConstantInt>(0, val->getType());
      lastExprValue = builder->createSub(zero, val, "neg", expr.getLoc());
      break;
    }
    default:
      lastExprValue = val;
      break;
    }
  }

  void visitMemberExpr(const hir::HIRMemberExpr &expr) override {
    // 1. Detect Namespace Access (e.g., `io.open`) or Enums (e.g.,
    // `Color.Green`)
    if (auto *ident =
            llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(expr.getObject())) {
      std::string baseName = ident->getName();
      if (symbolMap.find(baseName) == symbolMap.end() &&
          !mirModule->getGlobal(baseName)) {
        std::string fullName = baseName + "." + expr.getMemberName();

        if (MIRFunction *func = mirModule->getFunction(fullName)) {
          lastExprValue = func;
          return;
        }
        if (MIRGlobal *glob = mirModule->getGlobal(fullName)) {
          lastExprValue = builder->createLoad(glob, "enum.val", expr.getLoc());
          return;
        }

        const hir::HIRType *actualTy = expr.getType();
        bool isFunction =
            actualTy && actualTy->getKind() == hir::TypeKind::Function;

        if (isFunction) {
          auto *fnTy = llvm::dyn_cast_or_null<hir::FunctionType>(actualTy);
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
          // Enum Variant or Static Constant fallback
          if (!actualTy || actualTy->getKind() == hir::TypeKind::Void) {
            // Default enum variant backing type to a 32-bit integer
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

    // 2. Standard Object Member Access
    MIRValue *base = evaluateAsLValue(expr.getObject());
    if (!base)
      return;

    // L-Value to R-Value Conversion for Heap Pointers
    if (base && base->getType() &&
        base->getType()->getKind() == hir::TypeKind::Pointer) {
      auto *ptrTy = static_cast<const hir::PointerType *>(base->getType());
      const auto *pointee = ptrTy->getPointee();

      if (pointee && (pointee->getKind() == hir::TypeKind::Pointer ||
                      pointee->getKind() == hir::TypeKind::Reference ||
                      pointee->toString().find("shared") != std::string::npos ||
                      pointee->toString().find("weak") != std::string::npos ||
                      pointee->toString().find("Box<") != std::string::npos ||
                      pointee->toString().find("Arc<") != std::string::npos)) {

        if (llvm::isa<AllocaInst>(base) || llvm::isa<GetElementPtrInst>(base)) {
          base = builder->createLoad(base, "base.load", expr.getLoc());
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

      // If base is NOT null, go to access block. Else, skip to merge block.
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

    // 3. Unwrap smart pointers to find the inner struct fields
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
      // Clean up pointer prefixes (and potential spaces)
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

    // 4. Force Lookup in Classes (Using robust substring matching)
    const hir::HIRClass *targetCls = nullptr;
    for (const auto *cls : hirModule->getClasses()) {
      if (!typeName.empty() &&
          typeName.find(cls->getName()) != std::string::npos) {
        targetCls = cls;
        break;
      }
    }

    // Helper to recursively extract the field, bypassing opaque wrappers like
    // Box/Arc
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

      // Prevent infinite recursion on recursive struct definitions
      for (auto *v : visitedTy) {
        if (v == innerTy)
          return false;
      }
      visitedTy.push_back(innerTy);

      if (auto *st = llvm::dyn_cast_or_null<hir::StructType>(innerTy)) {
        int idx = st->getFieldIndex(fieldName);
        if (idx >= 0) {
          // --- [FIX] INHERITANCE FIELD OFFSET ACCUMULATION ---
          size_t baseOffset = 0;
          if (currentCls) {
            const hir::HIRClass *c = currentCls;
            // Traverse UP the inheritance chain to sum all base class fields
            while (!c->getParentTypes().empty()) {
              std::string pName = c->getParentTypes()[0]->toString();
              while (!pName.empty() && !isalnum(pName[0]))
                pName = pName.substr(1);

              const hir::HIRClass *pCls = nullptr;
              for (const auto *cls : hirModule->getClasses()) {
                // Ignore generic brackets for base class lookup
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
                if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(pTy))
                  pTy = ptrTy->getPointee();
                if (auto *pSt = llvm::dyn_cast_or_null<hir::StructType>(pTy)) {
                  baseOffset += pSt->getFields().size();
                }
                c = pCls;
              } else {
                break;
              }
            }
          }

          fieldIndex = baseOffset + idx;
          fieldType = expr.getType();
          if (!fieldType || fieldType->getKind() == hir::TypeKind::Void) {
            fieldType = st->getFields()[idx];
          }
          resolvedAggregateTy = st;
          return true; // Found the field!
        }

        // --- [NEW] Inheritance Traversal ---
        // If the field isn't in this struct, walk up the parent class chain!
        if (currentCls) {
          for (const auto *parentTy : currentCls->getParentTypes()) {
            std::string pName = parentTy->toString();
            // Clean up pointer prefixes
            while (!pName.empty() &&
                   (pName[0] == '&' || pName[0] == '*' || pName[0] == ' ')) {
              pName = pName.substr(1);
            }

            // Resolve the parent's HIRClass for recursive context
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

        // Fallback for embedded composition (nested structs)
        for (const auto *fieldTy : st->getFields()) {
          if (self(self, fieldTy, nullptr))
            return true;
        }
      } else if (auto *ut = llvm::dyn_cast_or_null<hir::UnionType>(innerTy)) {
        int idx = ut->getFieldIndex(fieldName);
        if (idx >= 0) {
          fieldIndex = idx;
          fieldType = expr.getType();
          if (!fieldType || fieldType->getKind() == hir::TypeKind::Void) {
            fieldType = ut->getFields()[idx];
          }
          resolvedAggregateTy = ut;
          isUnionField = true; // Mark that this is a union!
          return true;
        }
      }
      return false;
    };

    // Pass 'targetCls' into the initial search calls
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

    // 5. CAREFUL CAST: If base is a Box/Arc, we MUST cast it to the inner class
    // type before GEP/Bitcast
    if (resolvedAggregateTy) {
      bool needsCast = true;

      // Unwrap the pointer to check the raw structural type.
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
                resolvedAggregateTy, hir::Ownership::Borrowed);
        base = builder->createBitCast(base, expectedPtrTy, "base.cast",
                                      expr.getLoc());
      }
    }

    MIRValue *accessPtr = nullptr;

    if (isUnionField) {
      // UNION ACCESS: All fields share the same memory address (offset 0).
      auto *expectedPtrTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              fieldType, hir::Ownership::None);
      accessPtr = builder->createBitCast(base, expectedPtrTy,
                                         fieldName + ".ptr", expr.getLoc());
    } else {
      // STRUCT ACCESS: Use GEP to calculate the sequential memory offset.
      const hir::HIRType *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      MIRValue *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
      MIRValue *idxVal =
          mirModule->getOrInsertConstant<ConstantInt>(fieldIndex, i32Ty);
      accessPtr = builder->createGEP(base, {zero, idxVal}, fieldType,
                                     fieldName + ".ptr", expr.getLoc());
    }

    // Safely load weak fields using the dynamically generated accessPtr
    if (isWeakMemory(fieldType)) {
      lastExprValue = builder->createLoadWeak(
          accessPtr, fieldType, fieldName + ".weak", expr.getLoc());
    } else {
      auto *loadInst =
          builder->createLoad(accessPtr, fieldName + ".val", expr.getLoc());
      if (isVolatilePointer(accessPtr))
        loadInst->setVolatile(true);
      applyBorrowKind(loadInst, fieldType);
      lastExprValue = loadInst;
    }

    // Optional Chaining Merge & Phi Node
    if (isOptional) {
      MIRValue *loadedVal = lastExprValue;
      MIRBlock *accessEndBlock = builder->getInsertBlock();
      builder->createBr(mergeBlock);

      builder->setInsertPoint(mergeBlock);

      const hir::HIRType *resTy = expr.getType();
      if (!resTy || resTy->getKind() == hir::TypeKind::Void) {
        resTy = loadedVal->getType();
      }

      auto *phi = builder->createPhi(resTy, "opt.phi", expr.getLoc());

      // Incoming 1: The base was null, so the whole chain evaluates to null
      auto *nullRes = mirModule->getOrInsertConstant<ConstantNull>(resTy);
      phi->addIncoming(nullRes, checkBlock);

      // Incoming 2: The access succeeded, cast the loaded value if needed
      MIRValue *castLoaded = loadedVal;
      if (loadedVal->getType() != resTy) {
        castLoaded =
            builder->createBitCast(loadedVal, resTy, "opt.cast", expr.getLoc());
      }
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

    visit(expr.getBase());
    MIRValue *base = lastExprValue;
    visit(expr.getIndex());
    MIRValue *idx = lastExprValue;

    if (!base || !idx)
      return;

    // [FIX] If indexing a Map/Table, we must call a runtime function!
    bool isMapLookup =
        (idx->getType() && idx->getType()->getKind() == hir::TypeKind::String);

    if (isMapLookup) {
      std::string funcName = "__moksha_map_get";
      MIRFunction *mapGet = mirModule->getFunction(funcName);
      if (!mapGet) {
        auto fn = std::make_unique<MIRFunction>(expr.getType(), funcName,
                                                Linkage::External);
        fn->addArgument(
            std::make_unique<MIRArgument>(fn.get(), base->getType(), 0));
        fn->addArgument(
            std::make_unique<MIRArgument>(fn.get(), idx->getType(), 1));
        mapGet = fn.get();
        mirModule->addFunction(std::move(fn));
      }
      lastExprValue = builder->createCall(mapGet, {base, idx}, expr.getType(),
                                          "map.val", false, expr.getLoc());
    } else {
      // Unpack Fat Pointer if it's a Slice!
      if (base->getType() &&
          base->getType()->getKind() == hir::TypeKind::Slice) {
        auto *elemPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                expr.getType(), hir::Ownership::None);
        // Extract the data pointer (Index 0)
        base = builder->insert(std::make_unique<ExtractValueInst>(
            base, 0, elemPtrTy, "slice.data", expr.getLoc()));
      }

      // Safe to use GEP for Array/Buffer memory offsets
      MIRValue *gep = builder->createGEP(base, {idx}, expr.getType(),
                                         "index.ptr", expr.getLoc());
      auto *loadInst = builder->createLoad(gep, "index.val", expr.getLoc());
      if (isVolatilePointer(gep))
        loadInst->setVolatile(true);
      applyBorrowKind(loadInst, expr.getType());
      lastExprValue = loadInst;
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
    // FIX: Ensure trueVal matches the expression's expected type
    if (trueVal->getType() != expr.getType()) {
      trueVal = builder->createBitCast(trueVal, expr.getType(), "ternary.cast",
                                       expr.getLoc());
    }
    MIRBlock *trueEndBlock = builder->getInsertBlock();
    builder->createBr(mergeBlock);

    // False Branch
    builder->setInsertPoint(falseBlock);
    visit(expr.getFalseExpr());
    MIRValue *falseVal = lastExprValue;
    // FIX: Ensure falseVal matches the expression's expected type
    if (falseVal->getType() != expr.getType()) {
      falseVal = builder->createBitCast(falseVal, expr.getType(),
                                        "ternary.cast", expr.getLoc());
    }
    MIRBlock *falseEndBlock = builder->getInsertBlock();
    builder->createBr(mergeBlock);

    builder->setInsertPoint(mergeBlock);
    auto *phi =
        builder->createPhi(expr.getType(), "ternary.phi", expr.getLoc());
    phi->addIncoming(trueVal, trueEndBlock);
    phi->addIncoming(falseVal, falseEndBlock);
    lastExprValue = phi;
  }

  void visitCastExpr(const hir::HIRCastExpr &expr) override {
    visit(expr.getExpr());
    MIRValue *val = lastExprValue;
    if (!val)
      return;

    const hir::HIRType *destTy = expr.getType();
    if (!destTy || destTy->getKind() == hir::TypeKind::Void)
      return;

    // If the value is already the destination type, don't emit a cast
    if (val->getType() == destTy) {
      lastExprValue = val;
      return;
    }

    // Pierce through existing casts to avoid A -> B -> A chains
    if (auto *prevCast = llvm::dyn_cast_or_null<CastInst>(val)) {
      if (prevCast->getValue()->getType() == destTy) {
        lastExprValue = prevCast->getValue();
        return;
      }
    }

    lastExprValue =
        builder->createBitCast(val, destTy, "cast.fold", expr.getLoc());
    applyBorrowKind(lastExprValue, destTy);
  }

  void visitNewExpr(const hir::HIRNewExpr &expr) override {
    // 1. Resolve Class Name and Type
    const hir::HIRType *objTy = expr.getType();

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

      // This converts "Resource<i32>" -> "Resource" so the AST lookup succeeds.
      size_t bracketPos = className.find('<');
      if (bracketPos != std::string::npos) {
        className = className.substr(0, bracketPos);
      }

      // Handle optional internal prefixing (e.g., "struct.Resource" ->
      // "Resource")
      if (className.find("struct.") == 0) {
        className = className.substr(7);
      } else if (className.find("class.") == 0) {
        className = className.substr(6);
      }
    } else {
      pointeeTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    MIRValue *objPtr = nullptr;

    // 2. Allocate Memory (Heap vs Stack)
    if (isPtr) {
      // ---> Reference Semantics (Heap Allocation) <---
      ensureBuiltinMIR("__moksha_alloc");
      MIRFunction *allocFunc = mirModule->getFunction("__moksha_alloc");

      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
          hir::Ownership::None);

      if (!allocFunc) {
        auto fn = std::make_unique<MIRFunction>(voidPtrTy, "__moksha_alloc",
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 0));
        allocFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              pointeeTy, hir::Ownership::None));
      auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
      auto *sizeGep = builder->createGEP(nullPtr, {one}, pointeeTy,
                                         "sizeof.gep", expr.getLoc());
      MIRValue *sizeVal =
          builder->createBitCast(sizeGep, i32Ty, "sizeof.int", expr.getLoc());

      MIRValue *rawPtr = builder->createCall(allocFunc, {sizeVal}, voidPtrTy,
                                             "alloc", false, expr.getLoc());

      // Cast to the Object Type Pointer
      objPtr = builder->createBitCast(
          rawPtr,
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              pointeeTy, hir::Ownership::None),
          "new.obj", expr.getLoc());
    } else {
      // Value Semantics (Stack Allocation)
      objPtr = builder->createAlloca(objTy, "new.obj.stack", expr.getLoc());
    }

    // Bind VTable Pointer
    const hir::HIRClass *targetCls = nullptr;
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
        auto *vptrPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                vtableGlobal->getType(), hir::Ownership::None);

        // GEP to index 0 (the hidden vptr field in the struct)
        auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
        MIRValue *vptrDest =
            builder->createGEP(objPtr, std::vector<MIRValue *>{zero, zero},
                               vptrPtrTy, "vptr.dest", expr.getLoc());

        builder->insert(
            std::make_unique<StoreInst>(vtableGlobal, vptrDest, expr.getLoc()));
      }
    }

    // 3. Evaluate Arguments and Call Constructor
    std::vector<MIRValue *> args;
    args.push_back(objPtr); // Inject the implicit `this` pointer!
    for (const auto &arg : expr.getArgs()) {
      visit(arg.get());
      args.push_back(lastExprValue);
    }

    std::vector<const hir::HIRType *> pTys;
    for (size_t i = 1; i < args.size();
         ++i) { // i=1 to skip the implicit 'this'
      pTys.push_back(args[i]->getType());
    }

    std::string ctorName =
        mangleName(className + ".constructor", pTys) + "_ret_void";

    // --- [NEW] Trigger Monomorphization Queue for Generics ---
    if (targetCls) {
      bool isGenericCall = false;

      if (objTy && objTy->toString().find("<") != std::string::npos) {
        isGenericCall = true;
      }

      // Only queue it if it actually exists in the foreign/local AST and
      // hasn't been compiled yet
      if (isGenericCall && instantiatedGenerics.insert(ctorName).second) {
        MonomorphizationTask task;
        task.genericClass = targetCls;
        task.typeArgs =
            pTys; // The constructor arguments infer the template types!
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

        builder->createCall(
            ctorFunc, std::move(args),
            const_cast<hir::HIRModule *>(hirModule)->getVoidType(), "", false,
            expr.getLoc());
      }
    }

    // 4. Return the correct value type based on semantics!
    if (isPtr) {
      // Reference types return the pointer itself
      lastExprValue = objPtr;
    } else {
      // Value types must LOAD the stack memory to evaluate to the actual
      // struct data!
      lastExprValue = builder->insert(
          std::make_unique<LoadInst>(objPtr, "new.val", expr.getLoc()));
    }
  }

  void visitLambdaExpr(const hir::HIRLambdaExpr &expr) override {
    std::string lambdaName = "lambda." + std::to_string(lambdaCounter++);
    const hir::HIRType *i32Ty =
        const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);

    // ========================================================================
    // 1. Identify Captures & Build Environment Struct Type
    // ========================================================================
    std::vector<std::pair<std::string, MIRValue *>> captures;
    std::vector<const hir::HIRType *> envMemberTypes;
    std::vector<bool> captureByRef;

    if (!expr.getCaptures().empty()) {
      // Use explicit captures if provided by Frontend
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
      // Fallback: Auto-detect captures for implicit closures
      for (const auto &[name, val] : symbolMap) {
        // Prevent capturing variables that are shadowed by lambda parameters!
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

          if (valTy) {
            auto kind = valTy->getKind();
            if (kind == hir::TypeKind::String || kind == hir::TypeKind::Slice ||
                kind == hir::TypeKind::Promise ||
                kind == hir::TypeKind::Closure) {
              isRef =
                  false; // Move-only types implicitly move (capture by value)
            }
          }

          bool isMutated = false;
          if (expr.getBody()) {
            std::string buffer;
            llvm::raw_string_ostream ss(buffer);
            expr.getBody()->dump(ss);
            ss.flush();

            // Check if the identifier is mutated (LHS of assignment or
            // inc/dec)
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
                    prevLine.find("Op: --") != std::string::npos) {
                  isMutated = true;
                  break;
                }
              }
              pos++;
            }
          }

          // If it was forcefully mutated, it must be passed by reference
          if (isMutated) {
            isRef = true;
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

    // ========================================================================
    // 2. Create the Environment Backpack
    // ========================================================================
    MIRValue *envAlloca = nullptr;
    std::vector<MIRValue *> savedCapVals;
    if (!captures.empty()) {
      ensureBuiltinMIR("__moksha_alloc");
      MIRFunction *allocFunc = mirModule->getFunction("__moksha_alloc");

      auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(envPtrTy);
      auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
      auto *sizeGep = builder->createGEP(nullPtr, {one}, envStructTy,
                                         "env.sizeof.gep", expr.getLoc());
      MIRValue *sizeVal = builder->createBitCast(
          sizeGep, i32Ty, "env.sizeof.int", expr.getLoc());

      auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          const_cast<hir::HIRModule *>(hirModule)->getVoidType(),
          hir::Ownership::None);

      if (!allocFunc) {
        auto fn = std::make_unique<MIRFunction>(voidPtrTy, "__moksha_alloc",
                                                Linkage::External);
        fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 0));
        allocFunc = fn.get();
        mirModule->addFunction(std::move(fn));
      }

      MIRValue *rawAlloc = builder->createCall(
          allocFunc, {sizeVal}, voidPtrTy, "env.alloc", false, expr.getLoc());

      envAlloca =
          builder->createBitCast(rawAlloc, envPtrTy, "env.ptr", expr.getLoc());

      MIRValue *envStructVal =
          mirModule->getOrInsertConstant<ConstantNull>(envStructTy);

      for (size_t i = 0; i < captures.size(); ++i) {
        MIRValue *valToStore = nullptr;

        if (captureByRef[i]) {
          // By Reference: Store the Alloca pointer
          valToStore = captures[i].second;
        } else {
          // By Value: Load and copy the data
          valToStore = builder->insert(std::make_unique<LoadInst>(
              captures[i].second, "cap.load", expr.getLoc()));
        }

        savedCapVals.push_back(valToStore);

        // Coerce the value type to match the exact struct field type instance
        if (valToStore->getType() != envMemberTypes[i]) {
          valToStore = builder->createBitCast(valToStore, envMemberTypes[i],
                                              "cap.cast", expr.getLoc());
        }

        envStructVal = builder->insert(std::make_unique<InsertValueInst>(
            envStructVal, valToStore, i, "env.insert", expr.getLoc()));
      }

      // Store the fully packed environment struct into the heap allocation
      builder->insert(
          std::make_unique<StoreInst>(envStructVal, envAlloca, expr.getLoc()));
    } else {
      envAlloca = mirModule->getOrInsertConstant<ConstantNull>(envPtrTy);
    }

    // ========================================================================
    // 3. Create the Lambda Function & Resolve Types
    // ========================================================================
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

    auto lambdaFunc =
        std::make_unique<MIRFunction>(returnTy, lambdaName, Linkage::Internal);

    auto *envArg = new MIRArgument(lambdaFunc.get(), envPtrTy, 0);
    lambdaFunc->addArgument(std::unique_ptr<MIRArgument>(envArg));

    MIRBlock *entry = new MIRBlock("entry", lambdaFunc.get());
    lambdaFunc->addBlock(std::unique_ptr<MIRBlock>(entry));

    MIRBlock *oldInsertPoint = builder->getInsertBlock();
    auto oldSymbolMap = symbolMap;
    MIRFunction *oldFunc = currFunc;

    builder->setInsertPoint(entry);
    currFunc = lambdaFunc.get();
    symbolMap.clear();

    // ========================================================================
    // 4. Unpack the Environment inside the lambda body
    // ========================================================================
    if (!captures.empty()) {
      auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
      for (size_t i = 0; i < captures.size(); ++i) {
        auto *idxVal = mirModule->getOrInsertConstant<ConstantInt>(i, i32Ty);
        auto *gep =
            builder->createGEP(envArg, {zero, idxVal}, envMemberTypes[i],
                               captures[i].first + ".ptr", expr.getLoc());

        if (captureByRef[i]) {
          MIRValue *origPtr = builder->createLoad(
              gep, captures[i].first + ".ref", expr.getLoc());
          symbolMap[captures[i].first] = origPtr;
        } else {
          symbolMap[captures[i].first] = gep;
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
    if (expr.getBody())
      visit(expr.getBody());

    if (!getTerminator(builder->getInsertBlock())) {
      if (returnTy && returnTy->getKind() != hir::TypeKind::Void) {
        if (lastExprValue) {
          if (lastExprValue->getType() != returnTy) {
            lastExprValue = builder->createBitCast(lastExprValue, returnTy,
                                                   "ret.cast", expr.getLoc());
          }
          builder->insert(
              std::make_unique<ReturnInst>(lastExprValue, expr.getLoc()));
        } else {
          MIRValue *defVal = nullptr;
          if (returnTy->getKind() == hir::TypeKind::Int)
            defVal = mirModule->getOrInsertConstant<ConstantInt>(0, returnTy);
          else if (returnTy->getKind() == hir::TypeKind::Float ||
                   returnTy->getKind() == hir::TypeKind::Decimal)
            defVal =
                mirModule->getOrInsertConstant<ConstantFloat>(0.0, returnTy);
          else if (returnTy->getKind() == hir::TypeKind::Bool)
            defVal =
                mirModule->getOrInsertConstant<ConstantBool>(false, returnTy);
          else
            defVal = mirModule->getOrInsertConstant<ConstantNull>(returnTy);

          builder->insert(std::make_unique<ReturnInst>(defVal, expr.getLoc()));
        }
      } else {
        builder->insert(std::make_unique<ReturnInst>(nullptr, expr.getLoc()));
      }
    }

    lambdaFunc->numberUnnamedValues();

    builder->setInsertPoint(oldInsertPoint);
    symbolMap = oldSymbolMap;
    currFunc = oldFunc;

    MIRFunction *fnPtr = lambdaFunc.get();
    mirModule->addFunction(std::move(lambdaFunc));

    // ========================================================================
    // 7. Emit MakeClosure Instruction
    // ========================================================================
    const hir::HIRType *lambdaRetTy = fnPtr->getType();
    if (!lambdaRetTy) {
      lambdaRetTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    std::vector<const hir::HIRType *> actualFnParams;
    actualFnParams.push_back(envPtrTy); // Arg 0 is the hidden environment
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

    // [FIX] Push tagged captures for precise NLL tracking!
    for (size_t i = 0; i < captures.size(); ++i) {
      MIRValue *capVal = captures[i].second;

      if (captureByRef[i]) {
        // Create a phantom BitCast strictly to carry the BorrowKind tag
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
                  prevLine.find("Op: --") != std::string::npos) {
                isMutated = true;
                break;
              }
            }
            pos++;
          }
        }

        if (isMutated) {
          taggedCap->setBorrowKind(mir::BorrowKind::Mut);
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
  }

  void visitThreadExpr(const hir::HIRThreadExpr &expr) override {
    // 1. Evaluate the Lambda/Closure task
    visit(expr.getTask());
    MIRValue *closureVal = lastExprValue;
    if (!closureVal)
      return;

    // 2. Resolve Return Type
    const hir::HIRType *retTy = expr.getType();
    if (!retTy) {
      retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    // 3. [FIX] Get a unique SSA name for the thread handle
    MIRFunction *func = builder->getInsertBlock()->getParent();
    std::string callName = (retTy->getKind() == hir::TypeKind::Void)
                               ? ""
                               : func->getUniqueName("thread.handle");

    // 4. Emit native Spawn Instruction
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
    // 1. [FIX] Access the target type directly using getTargetType()
    const hir::HIRType *targetTy = expr.getTargetType();

    if (!targetTy) {
      // Fallback to i32 if type is unknown
      targetTy = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    }

    // 2. The GEP Trick for Scalable Size Calculation
    // (ptrtoint (gep null, 1))
    auto *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
        const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            targetTy, hir::Ownership::None));

    auto *one = mirModule->getOrInsertConstant<ConstantInt>(
        1, const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true));

    // GEP to index 1 of a null pointer of targetTy returns the size of
    // targetTy
    auto *gep = builder->createGEP(nullPtr, {one}, targetTy, "sizeof.gep",
                                   expr.getLoc());

    // Bitcast the resulting pointer address to 'usize' (u64)
    const hir::HIRType *usizeTy =
        const_cast<hir::HIRModule *>(hirModule)->getIntType(64, false);
    lastExprValue =
        builder->createBitCast(gep, usizeTy, "sizeof.int", expr.getLoc());
  }

  void visitAwaitExpr(const hir::HIRAwaitExpr &expr) override {
    // 1. Evaluate the inner Task/Promise
    visit(expr.getExpr());
    MIRValue *taskVal = lastExprValue;

    if (!taskVal || (taskVal->getType() &&
                     taskVal->getType()->getKind() == hir::TypeKind::Void)) {
      diags.report(expr.getLoc(), DiagID::err_invalid_type)
          << "Cannot await a void expression";
      lastExprValue = nullptr;
      return;
    }

    // 2. Resolve Return Type
    const hir::HIRType *retTy = expr.getType();
    if (!retTy && taskVal) {
      retTy = taskVal->getType();
    }
    if (!retTy) {
      retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    }

    // 3. [FIX] Get a unique SSA name for the unwrapped promise result
    MIRFunction *func = builder->getInsertBlock()->getParent();
    std::string callName = (retTy->getKind() == hir::TypeKind::Void)
                               ? ""
                               : func->getUniqueName("await.result");

    // 4. Emit native Await Instruction
    lastExprValue =
        builder->createAwait(taskVal, retTy, callName, expr.getLoc());
  }

  void visitSuperExpr(const hir::HIRSuperExpr &expr) override {
    // Stub: Used similarly to this-expr but bounds method resolution
    // statically
  }

  void visitDerefExpr(const hir::HIRDerefExpr &expr) override {
    visit(expr.getPointer());
    MIRValue *ptrVal = lastExprValue;
    if (!ptrVal)
      return;
    if (isWeakMemory(ptrVal->getType())) {
      lastExprValue = builder->createLoadWeak(ptrVal, expr.getType(),
                                              "deref.weak", expr.getLoc());
    } else {
      auto *loadInst = builder->createLoad(ptrVal, "deref.val", expr.getLoc());
      if (isVolatilePointer(ptrVal))
        loadInst->setVolatile(true);
      applyBorrowKind(loadInst, expr.getType());
      lastExprValue = loadInst;
    }
  }

  void visitAddressOfExpr(const hir::HIRAddressOfExpr &expr) override {
    const hir::HIRExpr *operand = expr.getOperand();

    // 1. Pierce through CastExprs to find the true underlying L-Value
    while (auto *castExpr = llvm::dyn_cast_or_null<hir::HIRCastExpr>(operand)) {
      operand = castExpr->getExpr();
    }

    MIRValue *ptr = nullptr;

    // 2. Resolve the memory location
    if (auto *ident = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(operand)) {
      std::string name = ident->getName();
      if (symbolMap.count(name)) {
        ptr = symbolMap[name];
      } else if (auto *global = mirModule->getGlobal(name)) {
        ptr = global;
      }
    } else {
      // Fallback for complex expressions (like &array[0] or &(*p))
      visit(operand);
      if (auto *loadInst = llvm::dyn_cast_or_null<LoadInst>(lastExprValue)) {
        ptr = loadInst->getPointer();
      } else {
        ptr = lastExprValue; // It might already be a pointer
      }
    }

    // 3. Ensure the extracted pointer matches the expected AddressOf type
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

    // 1. Pass the total number of parts as the first argument for variadic
    // safety
    size_t numParts = expr.getParts().size();
    callArgs.push_back(
        mirModule->getOrInsertConstant<ConstantInt>(numParts, i32Ty));

    // 2. Iterate through each part of the template string
    for (const auto &part : expr.getParts()) {
      visit(part.get());
      MIRValue *val = lastExprValue;
      const hir::HIRType *valTy = val->getType();

      if (valTy->getKind() != hir::TypeKind::String) {
        // --- FIX: Strongly Typed to_string Lowering ---
        std::string typeName;
        if (auto *intTy = llvm::dyn_cast<hir::HIRIntType>(valTy)) {
          typeName = (intTy->isSigned() ? "i" : "u") +
                     std::to_string(intTy->getWidth());
        } else if (auto *floatTy = llvm::dyn_cast<hir::HIRFloatType>(valTy)) {
          typeName = "f" + std::to_string(floatTy->getWidth());
        } else if (valTy->getKind() == hir::TypeKind::Bool) {
          typeName = "bool";
        } else {
          // Fallback for complex objects (Interfaces, Structs, etc.)
          typeName = "ptr";
          auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
          auto *voidPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  voidTy, hir::Ownership::None);

          val =
              builder->createBitCast(val, voidPtrTy, "ptr.cast", expr.getLoc());
          valTy = voidPtrTy;
        }

        std::string toStringName = "__moksha_" + typeName + "_to_string";
        MIRFunction *toStringFunc = mirModule->getFunction(toStringName);

        if (!toStringFunc) {
          auto fn = std::make_unique<MIRFunction>(stringTy, toStringName,
                                                  Linkage::External);
          fn->addArgument(std::make_unique<MIRArgument>(fn.get(), valTy, 0));
          toStringFunc = fn.get();
          mirModule->addFunction(std::move(fn));
        }

        // Convert the non-string value to a string pointer!
        val = builder->createCall(toStringFunc, {val}, stringTy,
                                  typeName + ".to_str", false, expr.getLoc());
      }

      callArgs.push_back(val);
    }

    // 3. Call the updated variadic join function that strictly expects (i32
    // count, string...)
    std::string joinName = "__moksha_template_join_strs";
    MIRFunction *joinFunc = mirModule->getFunction(joinName);
    if (!joinFunc) {
      auto fn =
          std::make_unique<MIRFunction>(stringTy, joinName, Linkage::External);
      fn->addArgument(
          std::make_unique<MIRArgument>(fn.get(), i32Ty, 0)); // count
      fn->setVariadic(true);
      joinFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    lastExprValue = builder->createCall(joinFunc, callArgs, stringTy,
                                        "interp.str", true, expr.getLoc());
  }

  void visitInputExpr(const hir::HIRInputExpr &expr) override {
    // 1. Resolve the runtime builtin function with a fixed signature:
    // string(string)
    std::string funcName = "__moksha_input";
    MIRFunction *inputFunc = mirModule->getFunction(funcName);
    auto *strTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();

    if (!inputFunc) {
      auto fn =
          std::make_unique<MIRFunction>(strTy, funcName, Linkage::External);

      // Always declare with exactly one string argument (the prompt)
      // to ensure consistency across different call sites.
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), strTy, 0));

      inputFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    // 2. Prepare arguments: provide an empty string if the prompt is missing
    std::vector<MIRValue *> args;
    if (expr.getPrompt()) {
      visit(expr.getPrompt());
      args.push_back(lastExprValue);
    } else {
      // [FIX] If no prompt is provided in HIR, inject an empty string
      // constant so the MIR call matches the fixed function signature.
      args.push_back(mirModule->getOrInsertConstant<ConstantString>("", strTy));
    }

    // 3. Emit the native Call
    lastExprValue = builder->createCall(inputFunc, std::move(args), strTy,
                                        "input.res", false, expr.getLoc());
  }

  void visitArrayLiteral(const hir::HIRArrayLiteral &expr) override {
    const hir::HIRType *arrayTy = expr.getType();
    const hir::HIRType *elemTy = nullptr;

    // 1. Extract the element type
    if (auto *arrTyInfo = llvm::dyn_cast_or_null<hir::ArrayType>(arrayTy)) {
      elemTy = arrTyInfo->getElementType();
    } else if (!expr.getElements().empty()) {
      elemTy = expr.getElements()[0]->getType();
    }

    if (!elemTy) {
      lastExprValue = mirModule->getOrInsertConstant<ConstantNull>(arrayTy);
      return;
    }

    auto *i32Ty = const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
    auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
    auto *voidPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        voidTy, hir::Ownership::None);

    MIRValue *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);
    MIRValue *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);

    // --- FIX 1: Two-Pass Evaluation (Calculate Dynamic Length) ---
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

      if (auto *spread = llvm::dyn_cast<hir::HIRSpreadExpr>(elementExpr)) {
        visit(spread->getIterable());
        MIRValue *sourceVal = lastExprValue;
        const hir::HIRType *sourceTy = spread->getIterable()->getType();

        MIRValue *spreadLenVal = nullptr;

        if (auto *arrTyInfo =
                llvm::dyn_cast_or_null<hir::ArrayType>(sourceTy)) {
          // Static Array: Size is known at compile time
          spreadLenVal = mirModule->getOrInsertConstant<ConstantInt>(
              arrTyInfo->getSize(), i32Ty);
        } else if (sourceTy->getKind() == hir::TypeKind::Slice) {
          // Dynamic Slice: Extract the length from the Fat Pointer struct
          // (index 1)
          spreadLenVal = builder->insert(std::make_unique<ExtractValueInst>(
              sourceVal, 1, i32Ty, "slice.len.extract", expr.getLoc()));
        } else {
          spreadLenVal = one; // Fallback
        }

        evaluatedElements.push_back({true, sourceVal, spreadLenVal, sourceTy});
        totalNumElementsVal = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Add, totalNumElementsVal, spreadLenVal, "total.add",
            expr.getLoc()));
      } else {
        // Standard Element
        visit(elementExpr);
        MIRValue *elemVal = lastExprValue;
        evaluatedElements.push_back({false, elemVal, one, elemTy});
        totalNumElementsVal = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Add, totalNumElementsVal, one, "total.add", expr.getLoc()));
      }
    }

    // 2. Calculate Allocation Size: sizeof(elemTy) * totalNumElementsVal
    MIRValue *nullPtr = mirModule->getOrInsertConstant<ConstantNull>(
        const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            elemTy, hir::Ownership::None));

    MIRValue *sizeGep =
        builder->createGEP(nullPtr, {one}, elemTy, "sizeof.gep", expr.getLoc());
    MIRValue *sizeofInt =
        builder->createBitCast(sizeGep, i32Ty, "sizeof.int", expr.getLoc());

    MIRValue *totalSize = builder->insert(std::make_unique<BinaryInst>(
        Opcode::Mul, sizeofInt, totalNumElementsVal, "array.size",
        expr.getLoc()));

    // 3. Allocate Array Memory
    std::string allocName = "__moksha_alloc";
    ensureBuiltinMIR(allocName);
    MIRFunction *allocFunc = mirModule->getFunction(allocName);
    if (!allocFunc) {
      auto fn = std::make_unique<MIRFunction>(voidPtrTy, allocName,
                                              Linkage::External);
      fn->addArgument(std::make_unique<MIRArgument>(fn.get(), i32Ty, 0));
      allocFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    MIRValue *rawAlloc = builder->createCall(allocFunc, {totalSize}, voidPtrTy,
                                             "array.raw", false, expr.getLoc());

    auto *elemPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
        elemTy, hir::Ownership::None);
    MIRValue *arrayElemPtr = builder->createBitCast(
        rawAlloc, elemPtrTy, "array.elem.ptr", expr.getLoc());

    // --- FIX 2: Dynamic Index Tracker ---
    MIRValue *dynIndex = zero;

    // --- FIX: Define a custom array copy to guarantee exact type matching
    // ---
    std::string memcpyName = "__moksha_array_copy";
    MIRFunction *memcpyFunc = mirModule->getFunction(memcpyName);
    if (!memcpyFunc) {
      // Use voidTy instead of voidPtrTy as it doesn't return a value
      auto fn =
          std::make_unique<MIRFunction>(voidTy, memcpyName, Linkage::External);
      fn->addArgument(
          std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 0)); // dest
      fn->addArgument(
          std::make_unique<MIRArgument>(fn.get(), voidPtrTy, 1)); // src
      fn->addArgument(
          std::make_unique<MIRArgument>(fn.get(), i32Ty, 2)); // n bytes
      memcpyFunc = fn.get();
      mirModule->addFunction(std::move(fn));
    }

    // Extract exact pointer type to bypass strict pointer equality checks
    const hir::HIRType *exactVoidPtrTy =
        memcpyFunc->getRawArguments()[0]->getType();

    // 4. Populate the array
    for (const auto &evalEl : evaluatedElements) {
      if (evalEl.isSpread) {
        // --- FIX 3: Memcpy Spread Operators ---

        // A. Calculate destination pointer
        MIRValue *destElemPtr = builder->createGEP(
            arrayElemPtr, {dynIndex}, elemTy, "spread.dest.ptr", expr.getLoc());
        MIRValue *destVoidPtr = builder->createBitCast(
            destElemPtr, exactVoidPtrTy, "dest.void", expr.getLoc());

        // B. Get source data pointer
        MIRValue *srcDataPtr = nullptr;
        if (evalEl.type->getKind() == hir::TypeKind::Slice) {
          // Dynamic Slice: Extract data pointer from Fat Pointer struct
          // (index 0)
          srcDataPtr = builder->insert(std::make_unique<ExtractValueInst>(
              evalEl.value, 0, elemPtrTy, "slice.ptr.extract", expr.getLoc()));
        } else {
          // Static Array: Store value to temporary alloca and cast to pointer
          auto *tmpPtrTy =
              const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                  evalEl.type, hir::Ownership::None);
          auto tmpAlloca = std::make_unique<AllocaInst>(
              tmpPtrTy, evalEl.type, "spread.tmp", expr.getLoc(), 8);
          MIRValue *tmpPtr = tmpAlloca.get();
          builder->insert(std::move(tmpAlloca));
          builder->insert(
              std::make_unique<StoreInst>(evalEl.value, tmpPtr, expr.getLoc()));
          srcDataPtr = builder->createBitCast(tmpPtr, elemPtrTy, "spread.base",
                                              expr.getLoc());
        }

        MIRValue *srcVoidPtr = builder->createBitCast(
            srcDataPtr, exactVoidPtrTy, "src.void", expr.getLoc());

        // C. Calculate bytes to copy (length * sizeof(element))
        MIRValue *bytesToCopy = builder->insert(
            std::make_unique<BinaryInst>(Opcode::Mul, sizeofInt, evalEl.length,
                                         "spread.bytes", expr.getLoc()));

        // D. Call Memcpy (Return type is voidTy, so name must be empty "")
        builder->createCall(memcpyFunc, {destVoidPtr, srcVoidPtr, bytesToCopy},
                            voidTy, "", false, expr.getLoc());

        // E. Update dynIndex += length
        dynIndex = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Add, dynIndex, evalEl.length, "idx.next", expr.getLoc()));

      } else {
        // Standard Element Evaluation
        MIRValue *elemVal = evalEl.value;

        if (elemVal->getType() != elemTy) {
          if (llvm::dyn_cast_or_null<ConstantNull>(elemVal)) {
            elemVal = mirModule->getOrInsertConstant<ConstantNull>(elemTy);
          } else {
            elemVal = builder->createBitCast(elemVal, elemTy, "elem.cast",
                                             expr.getLoc());
          }
        }

        MIRValue *elemPtr = builder->createGEP(arrayElemPtr, {dynIndex}, elemTy,
                                               "elem.ptr", expr.getLoc());

        builder->insert(
            std::make_unique<StoreInst>(elemVal, elemPtr, expr.getLoc()));

        // dynIndex += 1
        dynIndex = builder->insert(std::make_unique<BinaryInst>(
            Opcode::Add, dynIndex, one, "idx.next", expr.getLoc()));
      }
    }

    // 5. Return the array as an aggregate VALUE
    if (arrayTy->getKind() == hir::TypeKind::Slice) {
      // Create a Fat Pointer Struct: { data_ptr, length }
      MIRValue *emptyStruct =
          mirModule->getOrInsertConstant<ConstantNull>(arrayTy);

      MIRValue *insertedData =
          builder->insert(std::make_unique<InsertValueInst>(
              emptyStruct, arrayElemPtr, 0, "slice.ptr", expr.getLoc()));

      // Use the dynamically calculated total length!
      MIRValue *insertedLen = builder->insert(std::make_unique<InsertValueInst>(
          insertedData, totalNumElementsVal, 1, "slice.len", expr.getLoc()));

      lastExprValue = insertedLen;
    } else {
      // Standard Fixed-Array Pointer Logic
      auto *arrayPtrTy =
          const_cast<hir::HIRModule *>(hirModule)->getPointerType(
              arrayTy, hir::Ownership::None);
      MIRValue *typedArrayPtr = builder->createBitCast(
          rawAlloc, arrayPtrTy, "array.typed.ptr", expr.getLoc());

      lastExprValue =
          builder->createLoad(typedArrayPtr, "array.load", expr.getLoc());
    }
  }

  void visitMapLiteral(const hir::HIRMapLiteral &expr) override {
    std::vector<std::pair<MIRValue *, MIRValue *>> entries;
    for (auto &pair : expr.getEntries()) {
      visit(pair.first.get());
      MIRValue *k = lastExprValue;
      visit(pair.second.get());
      MIRValue *v = lastExprValue;
      entries.push_back({k, v});
    }
    lastExprValue = mirModule->getOrInsertConstant<ConstantMap>(
        expr.getType(), std::move(entries));
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
    const hir::HIRType *ty = expr.getType();
    if (!ty)
      ty = const_cast<hir::HIRModule *>(hirModule)->getStringType();

    // Evaluate the escape sequences before inserting into MIR
    std::string unescaped = unescapeString(expr.getValue());

    lastExprValue =
        mirModule->getOrInsertConstant<ConstantString>(unescaped, ty);
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
      if (isWeakMemory(ptr->getType())) {
        lastExprValue = builder->createLoadWeak(ptr, expr.getType(),
                                                name + ".weak", expr.getLoc());
      } else {
        auto *loadInst = builder->createLoad(ptr, "", expr.getLoc());
        if (isVolatilePointer(ptr))
          loadInst->setVolatile(true);
        applyBorrowKind(loadInst, expr.getType());
        lastExprValue = loadInst;
      }
    } else if (MIRGlobal *global = mirModule->getGlobal(name)) {
      if (isWeakMemory(global->getType())) {
        lastExprValue = builder->createLoadWeak(global, expr.getType(),
                                                name + ".weak", expr.getLoc());
      } else {
        auto *loadInst = builder->createLoad(global, "", expr.getLoc());
        if (isVolatilePointer(global))
          loadInst->setVolatile(true);
        applyBorrowKind(loadInst, expr.getType());
        lastExprValue = loadInst;
      }
    } else {
      ensureBuiltinMIR(name);

      if (MIRFunction *func = mirModule->getFunction(name)) {
        lastExprValue = func;
      } else {
        // [FIX] Unwrap pointer/reference types before attempting to cast
        const hir::HIRType *actualTy = expr.getType();
        if (actualTy) {
          if (auto *ptrTy =
                  llvm::dyn_cast_or_null<hir::PointerType>(actualTy)) {
            actualTy = ptrTy->getPointee();
          }
        }

        if (auto *fnTy = llvm::dyn_cast_or_null<hir::FunctionType>(actualTy)) {
          auto externFunc = std::make_unique<MIRFunction>(
              fnTy->getReturnType(), name, Linkage::External);

          unsigned idx = 0;
          for (const auto *pTy : fnTy->getParamTypes()) {
            externFunc->addArgument(
                std::make_unique<MIRArgument>(externFunc.get(), pTy, idx++));
          }

          if (name == "print" || name == "println")
            externFunc->setVariadic(true);

          lastExprValue = externFunc.get();
          mirModule->addFunction(std::move(externFunc));
        } else {
          diags.report(expr.getLoc(), DiagID::err_undeclared_identifier)
              << name;
          lastExprValue = nullptr;
        }
      }
    }
  }

  void visitCallExpr(const hir::HIRCallExpr &expr) override {
    MIRValue *callee = nullptr;
    std::string calleeName = "";
    std::vector<MIRValue *> args;

    // ========================================================================
    // 1. Resolve Callee (Identifier, Method, or Closure)
    // ========================================================================

    // Detect Namespace Calls (e.g., io.open) before Branch B
    bool isNamespaceCall = false;
    std::string namespaceFuncName = "";
    if (auto *memberExpr =
            llvm::dyn_cast_or_null<hir::HIRMemberExpr>(expr.getCallee())) {
      if (auto *ident = llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(
              memberExpr->getObject())) {
        std::string baseName = ident->getName();
        // If the base object isn't a known variable, it's a namespace prefix!
        if (symbolMap.find(baseName) == symbolMap.end() &&
            !mirModule->getGlobal(baseName)) {
          isNamespaceCall = true;
          namespaceFuncName = baseName + "." + memberExpr->getMemberName();
        }
      }
    }

    // A. Direct Identifier (Casts, Builtins, Normal Functions) AND Namespace
    // Calls
    if (isNamespaceCall ||
        llvm::dyn_cast_or_null<hir::HIRIdentifierExpr>(expr.getCallee())) {

      if (isNamespaceCall) {
        calleeName = namespaceFuncName;
      } else {
        calleeName =
            static_cast<const hir::HIRIdentifierExpr *>(expr.getCallee())
                ->getName();
      }

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

        // --- Route Native MIR Instructions ---
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
          // Not a native MIR op. Break to generate an external function
          // CallInst below.
          break;
        }

        // --- Route Standard Function Call Intrinsics (bswap, clz, etc.) ---
        callee = mirModule->getFunction(calleeName);
        if (!callee) {
          const hir::HIRType *argTy =
              expr.getArgs().empty()
                  ? const_cast<hir::HIRModule *>(hirModule)->getIntType(32,
                                                                        false)
                  : expr.getArgs()[0]->getType();

          const hir::HIRType *retTy =
              (intrinID == IntrinsicID::Bswap)
                  ? argTy
                  : const_cast<hir::HIRModule *>(hirModule)->getIntType(32,
                                                                        true);

          auto intrin = std::make_unique<MIRFunction>(retTy, calleeName,
                                                      Linkage::External);
          intrin->addArgument(
              std::make_unique<MIRArgument>(intrin.get(), argTy, 0));

          callee = intrin.get();
          mirModule->addFunction(std::move(intrin));
        }
      }

      // Super Constructor Interceptor
      if (calleeName == "super") {
        std::string className = "";
        if (currFunc) {
          std::string funcName = currFunc->getName();
          size_t dotPos = funcName.find('.');
          if (dotPos != std::string::npos) {
            className = funcName.substr(0, dotPos);
          }
        }

        // 1. Dynamically discover the Parent Constructor by matching argument
        // types!
        callee = nullptr;
        for (const auto &func : mirModule->getFunctions()) {
          std::string fnName = func->getName();

          // Search for other constructors (excluding our own)
          if (fnName.find(".constructor") != std::string::npos &&
              fnName != className + ".constructor") {
            auto params = func->getRawArguments();

            // Check if parameter count matches (params[0] is the implicit
            // 'this' pointer)
            if (params.size() == expr.getArgs().size() + 1) {
              bool match = true;
              for (size_t i = 0; i < expr.getArgs().size(); ++i) {
                const hir::HIRType *expected = params[i + 1]->getType();
                const hir::HIRType *actual = expr.getArgs()[i]->getType();

                // Strict type kind matching
                if (expected && actual &&
                    expected->getKind() != actual->getKind()) {
                  match = false;
                  break;
                }
              }
              if (match) {
                callee = func.get();
                calleeName = fnName;
                break;
              }
            }
          }
        }

        // 2. Inject and implicitly upcast the child's 'this' pointer
        if (callee && symbolMap.count("this")) {
          MIRValue *thisAddr = symbolMap["this"];
          MIRValue *loadedThis =
              builder->createLoad(thisAddr, "this.val", expr.getLoc());

          // [FIX] Cast callee to MIRFunction* so we can access
          // getRawArguments()
          auto *calleeFunc = static_cast<MIRFunction *>(callee);

          // Auto-upcast `this` to the parent's `this` type
          const hir::HIRType *parentThisTy =
              calleeFunc->getRawArguments()[0]->getType();
          MIRValue *castedThis = builder->createBitCast(
              loadedThis, parentThisTy, "base.cast", expr.getLoc());

          args.push_back(castedThis);
        }
      }

      // --- Functional Cast Interceptor ---
      const hir::HIRType *targetTy = nullptr;
      if (calleeName == "char")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(8, false);
      else if (calleeName == "short")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(16, true);
      else if (calleeName == "int")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      else if (calleeName == "long")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true);
      else if (calleeName == "usize")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getIntType(
            64, false, true);
      else if (calleeName == "isize")
        targetTy =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(64, true, true);
      else if (calleeName == "quarter")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(8);
      else if (calleeName == "half")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(16);
      else if (calleeName == "float")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(32);
      else if (calleeName == "double")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getFloatType(64);
      else if (calleeName == "bool")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getBoolType();
      else if (calleeName == "string")
        targetTy = const_cast<hir::HIRModule *>(hirModule)->getStringType();

      if (targetTy) {
        if (!expr.getArgs().empty()) {
          visit(expr.getArgs()[0].get());
          MIRValue *valToCast = lastExprValue;
          lastExprValue = builder->createBitCast(
              valToCast, targetTy, "cast." + calleeName, expr.getLoc());
          return; // Exit early: it's a cast, not a call
        }
      }

      // --- Standard Identifier Resolution ---
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

      // --- Builtin / Generic Discovery & Type Deduction ---
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
          for (auto *arg : args) {
            argTys.push_back(arg->getType());
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

          calleeName = mangledName; // Swap callee name so the fallback
                                    // generates the correct 'declare'
        }

        ensureBuiltinMIR(calleeName); // Your custom helper
        callee = mirModule->getFunction(calleeName);

        if (!callee) {
          const hir::HIRType *retTy = expr.getType();

          // Smart deduction for generic builtins (atomic_load, pop, etc)
          if (!retTy || retTy->getKind() == hir::TypeKind::Void) {
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
          unsigned idx = 0;
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

          if (calleeName == "print" || calleeName == "println") {
            externFunc->setVariadic(true);
          }
          callee = externFunc.get();
          mirModule->addFunction(std::move(externFunc));
        }
      }
    }
    // B. Method Call Interception (e.g., io.open())
    else if (auto *memberExpr =
                 llvm::dyn_cast_or_null<hir::HIRMemberExpr>(expr.getCallee())) {
      visit(memberExpr->getObject());
      MIRValue *baseObj = lastExprValue;

      std::string className = "";
      const hir::HIRType *baseTy = baseObj ? baseObj->getType() : nullptr;

      // Unwrap pointers/references to find the underlying class
      if (auto *ptrTy = llvm::dyn_cast_or_null<hir::PointerType>(baseTy)) {
        baseTy = ptrTy->getPointee();
      } else if (auto *refTy =
                     llvm::dyn_cast_or_null<hir::ReferenceType>(baseTy)) {
        baseTy = refTy->getInner();
      }

      if (baseTy)
        className = baseTy->toString();

      // Dynamic Generic Sanitizer
      if (!className.empty() && className[0] == '*')
        className = className.substr(1);
      std::replace(className.begin(), className.end(), '<', '_');
      std::replace(className.begin(), className.end(), '>', '_');
      while (!className.empty() && className.back() == '_')
        className.pop_back();

      // Strip internal prefixes generated by earlier passes
      if (className.find("struct.") == 0)
        className = className.substr(7);
      if (className.find("class.") == 0)
        className = className.substr(6);

      calleeName = className + "." + memberExpr->getMemberName();

      std::vector<const hir::HIRType *> argTys;
      for (const auto &arg : expr.getArgs()) {
        argTys.push_back(arg->getType());
      }

      calleeName = mangleName(calleeName, argTys);
      const hir::HIRType *callRetTy = expr.getType();
      std::string retStr = callRetTy ? callRetTy->toString() : "void";
      std::replace(retStr.begin(), retStr.end(), '*', 'p');
      calleeName += "_ret_" + retStr;

      callee = mirModule->getFunction(calleeName);

      // 1. Lazy Method Declaration (Generates the expected signature)
      if (!callee) {
        const hir::HIRType *retTy = expr.getType();
        if (!retTy || retTy->getKind() == hir::TypeKind::Void) {
          retTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        }

        auto externFunc =
            std::make_unique<MIRFunction>(retTy, calleeName, Linkage::External);

        // Add the implicit 'this' pointer as the very first parameter
        const hir::HIRType *thisTy =
            baseObj ? baseObj->getType()
                    : const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        externFunc->addArgument(
            std::make_unique<MIRArgument>(externFunc.get(), thisTy, 0));

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

      // 2. Resolve target class
      const hir::HIRClass *targetCls = nullptr;
      for (const auto *cls : hirModule->getClasses()) {
        if (cls->getName() == className) {
          targetCls = cls;
          break;
        }
      }

      // 3. Check if the method is virtual
      const hir::HIRFunction *hirMethod = nullptr;
      if (targetCls) {
        for (const auto &m : targetCls->getMethods()) {
          if (m->getName() == memberExpr->getMemberName()) {
            hirMethod = m.get();
            break;
          }
        }
      }

      // 4. Dynamic Dispatch (VTable Lookup)
      if (hirMethod &&
          (hirMethod->isVirtualFunc() || hirMethod->isOverrideFunc())) {
        auto *i32Ty =
            const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
        auto *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
        auto *idxVal = mirModule->getOrInsertConstant<ConstantInt>(
            hirMethod->getVTableIndex(), i32Ty);
        auto *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);

        // a. Load VTable Pointer from object (index 0)
        auto *vptrAddr = builder->createGEP(baseObj, {zero, zero}, nullptr,
                                            "vptr.addr", expr.getLoc());
        MIRValue *vtablePtr =
            builder->createLoad(vptrAddr, "vtable.ptr", expr.getLoc());

        // b. GEP to the specific function pointer in the VTable array (index
        // 1 of vtable struct)
        auto *funcPtrAddr =
            builder->createGEP(vtablePtr, {zero, one, idxVal}, nullptr,
                               "vfunc.addr", expr.getLoc());

        // c. Load the raw function pointer
        MIRValue *rawFuncPtr =
            builder->createLoad(funcPtrAddr, "vfunc.raw", expr.getLoc());

        // d. Cast it back to the specific function signature we discovered in
        // Step 1
        auto *expectedPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                callee->getType(), hir::Ownership::None);
        callee = builder->createBitCast(rawFuncPtr, expectedPtrTy, "vfunc.cast",
                                        expr.getLoc());
      }

      // Inject 'this' as the very first argument!
      if (baseObj) {
        args.push_back(baseObj);
      }
    }
    // C. Fallback (e.g., Function Pointers returned by expressions)
    else {
      visit(expr.getCallee());
      callee = lastExprValue;
      if (callee) {
        calleeName = callee->getName();
      }
    }

    if (!callee)
      return; // Safety guard

    // ========================================================================
    // 2. Handle Closure Unpacking (Fat Pointers)
    // ========================================================================
    bool isClosure = false;

    // 1. Check AST Type (Strip pointers if any)
    if (expr.getCallee() && expr.getCallee()->getType()) {
      const hir::HIRType *astTy = expr.getCallee()->getType();
      while (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(astTy)) {
        astTy = pTy->getPointee();
      }
      if (astTy && astTy->getKind() == hir::TypeKind::Closure) {
        isClosure = true;
      }
    }

    // 2. Fallback: Check MIR Type (Strip pointers if any)
    if (!isClosure && callee && callee->getType()) {
      const hir::HIRType *mirTy = callee->getType();
      while (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(mirTy)) {
        mirTy = pTy->getPointee();
      }

      // [FIX 1] Recognize abstract HIRClosureType!
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

      // 1. Spill to memory if Mem2Reg promoted the fat pointer to an SSA
      // value
      if (!llvm::dyn_cast_or_null<hir::PointerType>(closurePtr->getType())) {
        auto *spill = builder->createAlloca(closurePtr->getType(),
                                            "closure.spill", expr.getLoc());
        spill->setBorrowKind(mir::BorrowKind::View); // [FIX] Prevent artificial
                                                     // mutable borrows!
        builder->insert(
            std::make_unique<StoreInst>(closurePtr, spill, expr.getLoc()));
        closurePtr = spill;
      }

      // 2. Physical Layout Synthesis
      const hir::HIRType *fnPtrTy = nullptr;
      const hir::HIRType *envPtrTy = nullptr;

      const hir::HIRType *baseTy = closurePtr->getType();
      if (auto *pTy = llvm::dyn_cast_or_null<hir::PointerType>(baseTy)) {
        baseTy = pTy->getPointee();
      }

      const hir::HIRClosureType *closTy =
          llvm::dyn_cast_or_null<hir::HIRClosureType>(baseTy);
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

        auto *rawFuncTy =
            const_cast<hir::HIRModule *>(hirModule)->getFunctionType(retTy,
                                                                     fnParams);
        fnPtrTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            rawFuncTy, hir::Ownership::None);

        auto *structTy = const_cast<hir::HIRModule *>(hirModule)->getStructType(
            "Closure.Opaque", {fnPtrTy, envPtrTy});
        auto *structPtrTy =
            const_cast<hir::HIRModule *>(hirModule)->getPointerType(
                structTy, hir::Ownership::None);

        closurePtr = builder->createBitCast(closurePtr, structPtrTy,
                                            "closure.cast", expr.getLoc());
      } else {
        diags.report(expr.getLoc(), DiagID::err_invalid_type)
            << "Cannot resolve physical closure layout";
        return;
      }

      auto *i32Ty =
          const_cast<hir::HIRModule *>(hirModule)->getIntType(32, true);
      MIRValue *zero = mirModule->getOrInsertConstant<ConstantInt>(0, i32Ty);
      MIRValue *one = mirModule->getOrInsertConstant<ConstantInt>(1, i32Ty);

      // 3. GEP and Load Function Pointer (Index 0)
      MIRValue *fnGep = builder->createGEP(closurePtr, {zero, zero}, fnPtrTy,
                                           "fn.gep", expr.getLoc());
      MIRValue *fn = builder->createLoad(fnGep, "fn.load", expr.getLoc());
      fn->setBorrowKind(mir::BorrowKind::View);

      // 4. GEP and Load Environment Pointer (Index 1)
      MIRValue *envGep = builder->createGEP(closurePtr, {zero, one}, envPtrTy,
                                            "env.gep", expr.getLoc());
      MIRValue *env = builder->createLoad(envGep, "env.load", expr.getLoc());

      // [FIX] Evaluate and Coerce Arguments for the Closure Call!
      std::vector<MIRValue *> callArgs;
      callArgs.push_back(env); // Environment is always argument 0

      const auto &paramTys = closTy->getParamTypes();
      for (size_t i = 0; i < expr.getArgs().size(); ++i) {
        visit(expr.getArgs()[i].get());
        MIRValue *argVal = lastExprValue;

        // Perform type coercion to ensure the MIRVerifier is satisfied
        if (i < paramTys.size() && argVal->getType() != paramTys[i]) {
          argVal = builder->createBitCast(argVal, paramTys[i], "clos.arg.cast",
                                          expr.getLoc());
        }
        callArgs.push_back(argVal);
      }

      // 6. Resolve Return Type
      const hir::HIRType *callRetTy = expr.getType();
      if (!callRetTy || callRetTy->getKind() == hir::TypeKind::Void) {
        callRetTy = closTy->getReturnType();
      }
      if (!callRetTy) {
        callRetTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      }

      // 7. Emit Native Call/Invoke
      if (currentUnwindDest) {
        MIRBlock *normalDest = newBlock("invoke.cont");
        MIRBlock *cleanupDest = newBlock("invoke.cleanup");

        lastExprValue =
            builder->createInvoke(fn, std::move(callArgs), normalDest,
                                  cleanupDest, callRetTy, "", expr.getLoc());

        // [FIX] Cache the invoke result before cleanup evaluation!
        MIRValue *invokeVal = lastExprValue;

        // Local unwind block for closures
        builder->setInsertPoint(cleanupDest);
        auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
        auto *lpadTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
            voidTy, hir::Ownership::None);

        // Store the caught exception into the slot
        auto *lpad =
            builder->createLandingPad(lpadTy, "ex.invoke", expr.getLoc());
        if (!currentExceptionSlots.empty()) {
          builder->insert(std::make_unique<StoreInst>(
              lpad, currentExceptionSlots.top(), expr.getLoc()));
        }

        MIRBlock *savedUnwind = currentUnwindDest;
        currentUnwindDest = nullptr;

        size_t targetDepth = tryScopeDepths.empty() ? 0 : tryScopeDepths.top();
        for (size_t i = scopeStack.size(); i > targetDepth; --i) {
          emitScopeCleanup(scopeStack[i - 1], expr.getLoc());
        }

        currentUnwindDest = savedUnwind;

        builder->createBr(currentUnwindDest);
        builder->setInsertPoint(normalDest);

        // [FIX] Restore the invoke result so outer expressions get the right
        // value!
        lastExprValue = invokeVal;
      } else {
        lastExprValue = builder->createCall(fn, std::move(callArgs), callRetTy,
                                            "", false, expr.getLoc());
      }

      return;
    }

    // ========================================================================
    // 3. Evaluate and Coerce Arguments
    // ========================================================================
    auto *mirF = llvm::dyn_cast_or_null<MIRFunction>(callee);
    bool isVarArg = mirF ? mirF->isVariadic() : false;
    const auto &params =
        mirF ? mirF->getRawArguments() : std::vector<MIRArgument *>();

    // [FIX] Coerce the hidden 'this' or 'env' argument if necessary
    if (mirF && !args.empty() && !params.empty()) {
      const hir::HIRType *expectedTy = params[0]->getType();
      if (args[0]->getType() != expectedTy) {
        args[0] = builder->createBitCast(args[0], expectedTy, "this.cast",
                                         expr.getLoc());
      }
    }

    size_t hiddenArgOffset = args.size();

    for (size_t i = 0; i < expr.getArgs().size(); ++i) {
      size_t paramIdx = hiddenArgOffset + i;
      const hir::HIRType *expectedArgTy = nullptr;
      if (mirF && paramIdx < params.size()) {
        expectedArgTy = params[paramIdx]->getType();
      }

      // Pass the expected parameter signature down before evaluating the
      // argument
      const hir::HIRType *oldExpected = expectedLambdaReturnType;
      expectedLambdaReturnType = expectedArgTy;

      visit(expr.getArgs()[i].get());

      expectedLambdaReturnType = oldExpected; // Restore

      MIRValue *argVal = lastExprValue;

      if (expectedArgTy && argVal->getType() != expectedArgTy) {
        argVal = builder->createBitCast(argVal, expectedArgTy, "call.cast",
                                        expr.getLoc());
      }
      args.push_back(argVal);
    }

    // If we resolved a static HIR function, check if we are missing arguments
    if (!calleeName.empty()) {
      const hir::HIRFunction *hirTarget = hirModule->getFunction(calleeName);

      // If the target exists and we provided fewer arguments than it expects
      if (hirTarget && args.size() < hirTarget->getParams().size()) {

        // Loop through the missing parameters
        for (size_t i = args.size(); i < hirTarget->getParams().size(); ++i) {
          const auto &param = hirTarget->getParams()[i];

          if (param.getDefaultValue()) {
            // Evaluate the default HIR expression dynamically
            visit(param.getDefaultValue());
            MIRValue *defVal = lastExprValue;

            // Coerce the default value if necessary
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

    // ========================================================================
    // 4. Emit Call or Invoke with Return Type Validation
    // ========================================================================
    const hir::HIRType *callRetTy = expr.getType();
    if (!callRetTy || callRetTy->getKind() == hir::TypeKind::Void) {
      if (callee && callee->getKind() == ValueKind::Function) {
        callRetTy = callee->getType();
      }
    }

    std::string callName = "";

    if (currentUnwindDest) {
      MIRBlock *normalDest = newBlock("invoke.cont");
      MIRBlock *cleanupDest = newBlock("invoke.cleanup");

      lastExprValue = builder->createInvoke(callee, std::move(args), normalDest,
                                            cleanupDest, callRetTy, callName,
                                            expr.getLoc());

      // [FIX] Cache the invoke result!
      MIRValue *invokeVal = lastExprValue;

      // Local unwind block for standard calls
      builder->setInsertPoint(cleanupDest);
      auto *voidTy = const_cast<hir::HIRModule *>(hirModule)->getVoidType();
      auto *lpadTy = const_cast<hir::HIRModule *>(hirModule)->getPointerType(
          voidTy, hir::Ownership::None);

      // Store the caught exception into the slot
      auto *lpad =
          builder->createLandingPad(lpadTy, "ex.invoke", expr.getLoc());
      if (!currentExceptionSlots.empty()) {
        builder->insert(std::make_unique<StoreInst>(
            lpad, currentExceptionSlots.top(), expr.getLoc()));
      }

      MIRBlock *savedUnwind = currentUnwindDest;
      currentUnwindDest = nullptr;

      size_t targetDepth = tryScopeDepths.empty() ? 0 : tryScopeDepths.top();
      for (size_t i = scopeStack.size(); i > targetDepth; --i) {
        emitScopeCleanup(scopeStack[i - 1], expr.getLoc());
      }
      currentUnwindDest = savedUnwind;

      builder->createBr(currentUnwindDest);
      builder->setInsertPoint(normalDest);

      // [FIX] Restore the invoke result!
      lastExprValue = invokeVal;
    } else {
      lastExprValue = builder->createCall(callee, std::move(args), callRetTy,
                                          callName, isVarArg, expr.getLoc());
    }
    applyBorrowKind(lastExprValue, callRetTy);
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
