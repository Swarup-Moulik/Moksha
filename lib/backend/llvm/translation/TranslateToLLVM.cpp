#include "moksha/Backend/LLVM/TranslateToLLVM.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "mlir/Target/LLVMIR/LLVMTranslationInterface.h"
#include "moksha/Dialect/MokshaDialect.h"
#include "llvm/IR/CallingConv.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"
#include "llvm/IR/Instructions.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/TargetParser/Host.h"
#include "llvm/Transforms/Utils/ModuleUtils.h"

namespace moksha {

class MokshaDialectLLVMIRTranslationInterface
    : public mlir::LLVMTranslationDialectInterface {
public:
  using LLVMTranslationDialectInterface::LLVMTranslationDialectInterface;

  // MLIR 22 Unified Signature for ALL operations
  mlir::LogicalResult
  amendOperation(mlir::Operation *op,
                 llvm::ArrayRef<llvm::Instruction *> instructions,
                 mlir::NamedAttribute attribute,
                 mlir::LLVM::ModuleTranslation &state) const override {
    return mlir::success(); // Safely ignore custom attributes
  }
};

std::unique_ptr<llvm::Module>
translateMokshaToLLVMIR(mlir::ModuleOp mlirModule,
                        llvm::LLVMContext &llvmContext,
                        const moksha::TargetConfig &config) {
  // 1. Register translations
  mlir::registerBuiltinDialectTranslation(*mlirModule->getContext());
  mlir::registerLLVMDialectTranslation(*mlirModule->getContext());

  mlirModule->getContext()
      ->getOrLoadDialect<moksha::IR::MokshaDialect>()
      ->addInterfaces<MokshaDialectLLVMIRTranslationInterface>();

  // Use the triple from our CLI config, fallback to host if empty
  std::string tripleStr = config.triple.empty()
                              ? llvm::sys::getDefaultTargetTriple()
                              : config.triple;

  // Tag the MLIR module with the correct triple
  mlirModule->setAttr(
      "llvm.target_triple",
      mlir::StringAttr::get(mlirModule->getContext(), tripleStr));
  llvm::Triple targetTriple(tripleStr);

  // Use the CPU and Features from our CLI config
  std::string cpu = config.cpu;
  std::string featureStr = config.features;

  // ONLY fallback to host CPU if config is empty AND we are compiling for the
  // host
  if (cpu.empty() && tripleStr == llvm::sys::getDefaultTargetTriple()) {
    cpu = llvm::sys::getHostCPUName().str();
    llvm::StringMap<bool> hostFeatures = llvm::sys::getHostCPUFeatures();

    if (!hostFeatures.empty()) {
      for (auto &f : hostFeatures) {
        if (!featureStr.empty())
          featureStr += ",";
        featureStr += (f.second ? "+" : "-") + f.first().str();
      }
    } else {
      featureStr = "+f16c,+avx";
    }
  }

  // llvm::errs() << "[DEBUG] Setting Target Machine & Data Layout...\n";
  std::string error;
  if (auto target = llvm::TargetRegistry::lookupTarget(tripleStr, error)) {
    llvm::TargetOptions opt;
    if (auto tm = target->createTargetMachine(targetTriple, cpu, featureStr,
                                              opt, std::nullopt)) {
      std::string dl = tm->createDataLayout().getStringRepresentation();
      mlirModule->setAttr("llvm.data_layout",
                          mlir::StringAttr::get(mlirModule->getContext(), dl));
    }
  }

  // llvm::errs() << "[DEBUG] Entering MLIR to LLVM IR Core Translator...\n";

  std::unique_ptr<llvm::Module> llvmModule =
      mlir::translateModuleToLLVMIR(mlirModule, llvmContext);

  // llvm::errs() << "[DEBUG] MLIR to LLVM IR Translation Successful!\n";

  if (!llvmModule) {
    llvm::errs() << "[FATAL] Translation returned nullptr.\n";
    return nullptr;
  }

  // llvm::errs() << "[DEBUG] Applying Post-Process Fixes...\n";

  llvm::StringRef persFnName = "__gxx_personality_v0";
  if (targetTriple.isOSWindows()) {
    if (targetTriple.isGNUEnvironment()) {
      persFnName = "__gxx_personality_seh0"; // For Windows MinGW
                                             // (x86_64-w64-windows-gnu)
    } else {
      persFnName = "__CxxFrameHandler3"; // For Windows MSVC
    }
  }

  // Define the standard C/C++ personality function
  llvm::Type *i32Ty = llvm::Type::getInt32Ty(llvmContext);
  llvm::FunctionType *persType = llvm::FunctionType::get(i32Ty, true);
  llvm::FunctionCallee persFuncCallee =
      llvmModule->getOrInsertFunction(persFnName, persType);
  llvm::Constant *persFunc =
      llvm::cast<llvm::Constant>(persFuncCallee.getCallee());

  std::vector<llvm::GlobalValue *> usedValues;

  // 4. Post-process to inject custom attributes and ABI specs
  mlirModule.walk([&](mlir::LLVM::LLVMFuncOp mlirFunc) {
    llvm::Function *llvmFunc = llvmModule->getFunction(mlirFunc.getName());
    if (!llvmFunc)
      return;

    if (!cpu.empty())
      llvmFunc->addFnAttr("target-cpu", cpu);
    if (!featureStr.empty())
      llvmFunc->addFnAttr("target-features", featureStr);

    // Standard LLVM Keyword Bindings
    if (mlirFunc->hasAttr("moksha.async")) {
      llvmFunc->addFnAttr(llvm::Attribute::PresplitCoroutine);

      if (!llvmFunc->empty()) {
        llvm::BasicBlock &entryBlock = llvmFunc->getEntryBlock();
        llvm::Instruction *coroBoundary = nullptr;

        // 1. Find the bottom of the coroutine header
        for (llvm::Instruction &I : entryBlock) {
          if (auto *call = llvm::dyn_cast_or_null<llvm::CallInst>(&I)) {
            if (llvm::Function *callee = call->getCalledFunction()) {
              llvm::StringRef name = callee->getName();
              if (name == "moksha_rt_coro_setup" ||
                  name.starts_with("llvm.coro.begin")) {
                coroBoundary = &I;
                break;
              }
            }
          }
        }

        if (coroBoundary) {
          // 2. Collect all allocas that MLIR incorrectly pushed to the top
          std::vector<llvm::AllocaInst *> allocasToSink;
          for (llvm::Instruction &I : entryBlock) {
            if (&I == coroBoundary)
              break; // Stop at the boundary
            if (auto *allocaInst =
                    llvm::dyn_cast_or_null<llvm::AllocaInst>(&I)) {
              allocasToSink.push_back(allocaInst);
            }
          }

          // 3. Move them immediately AFTER the coroutine boundary
          llvm::Instruction *insertionPt = coroBoundary->getNextNode();
          for (llvm::AllocaInst *allocaInst : allocasToSink) {
            allocaInst->moveBefore(insertionPt);
          }
        }
      }
    }
    if (mlirFunc->hasAttr("moksha.interrupt")) {
      llvmFunc->addFnAttr("interrupt");
    }
    if (mlirFunc->hasAttr("moksha.naked")) {
      llvmFunc->addFnAttr(llvm::Attribute::Naked);
    }
    if (mlirFunc->hasAttr("moksha.noreturn")) {
      llvmFunc->addFnAttr(llvm::Attribute::NoReturn);
    }
    if (mlirFunc->hasAttr("moksha.inline")) {
      llvmFunc->addFnAttr(llvm::Attribute::AlwaysInline);
    }
    if (mlirFunc->hasAttr("moksha.noinline")) {
      llvmFunc->addFnAttr(llvm::Attribute::NoInline);
    }
    if (mlirFunc->hasAttr("moksha.pure")) {
      llvmFunc->addFnAttr(llvm::Attribute::ReadNone);
      llvmFunc->addFnAttr(llvm::Attribute::NoUnwind);
    }
    if (mlirFunc->hasAttr("moksha.cold")) {
      llvmFunc->addFnAttr(llvm::Attribute::Cold);
    }
    if (mlirFunc->hasAttr("moksha.used")) {
      usedValues.push_back(llvmFunc);
    }

    if (auto ccAttr =
            mlirFunc->getAttrOfType<mlir::StringAttr>("moksha.calling_conv")) {
      llvm::StringRef cc = ccAttr.getValue();
      if (cc == "stdcall") {
        llvmFunc->setCallingConv(llvm::CallingConv::X86_StdCall);
      } else if (cc == "fastcall") {
        llvmFunc->setCallingConv(llvm::CallingConv::X86_FastCall);
      } else if (cc == "vectorcall") {
        llvmFunc->setCallingConv(llvm::CallingConv::X86_VectorCall);
      } else if (cc == "sysv64") {
        llvmFunc->setCallingConv(llvm::CallingConv::X86_64_SysV);
      } else if (cc == "win64") {
        llvmFunc->setCallingConv(llvm::CallingConv::Win64);
      } else if (cc == "interrupt") {
        llvmFunc->setCallingConv(llvm::CallingConv::X86_INTR);
        if (llvmFunc->arg_size() > 0) {
          llvm::Type *i64Ty = llvm::Type::getInt64Ty(llvmContext);
          llvm::Type *frameTy = llvm::ArrayType::get(i64Ty, 5);
          llvmFunc->addParamAttr(
              0, llvm::Attribute::getWithByValType(llvmContext, frameTy));
        }
      }
    }

    if (auto linkageAttr =
            mlirFunc->getAttrOfType<mlir::StringAttr>("moksha.linkage")) {
      llvm::StringRef linkage = linkageAttr.getValue();
      if (linkage == "weak") {
        llvmFunc->setLinkage(llvm::GlobalValue::WeakAnyLinkage);
      } else if (linkage == "linkonce") {
        llvmFunc->setLinkage(llvm::GlobalValue::LinkOnceODRLinkage);
        if (targetTriple.isOSWindows()) {
          llvm::Comdat *comdat =
              llvmModule->getOrInsertComdat(llvmFunc->getName());
          comdat->setSelectionKind(llvm::Comdat::Any);
          llvmFunc->setComdat(comdat);
        }
      } else if (linkage == "internal") {
        llvmFunc->setLinkage(llvm::GlobalValue::InternalLinkage);
      } else if (linkage == "external") {
        llvmFunc->setLinkage(llvm::GlobalValue::ExternalLinkage);
      }
    }

    if (auto secAttr =
            mlirFunc->getAttrOfType<mlir::StringAttr>("moksha.section")) {
      llvmFunc->setSection(secAttr.getValue());
    }

    bool hasLandingPad = false;
    mlirFunc.walk([&](mlir::LLVM::LandingpadOp op) { hasLandingPad = true; });

    if (hasLandingPad) {
      llvmFunc->setPersonalityFn(persFunc);
      llvm::PointerType *ptrTy = llvm::PointerType::getUnqual(llvmContext);
      llvm::Constant *nullPtr = llvm::ConstantPointerNull::get(ptrTy);
      for (llvm::BasicBlock &bb : *llvmFunc) {
        if (auto *lpad = llvm::dyn_cast_or_null<llvm::LandingPadInst>(
                bb.getFirstNonPHI())) {
          bool hasCatchAll = false;
          for (unsigned i = 0; i < lpad->getNumClauses(); ++i) {
            if (lpad->isCatch(i) && lpad->getClause(i)->isNullValue()) {
              hasCatchAll = true;
              break;
            }
          }
          if (!hasCatchAll) {
            lpad->addClause(nullPtr);
          }
        }
      }
    }
  });

  // 5. Post-process Globals for TLS and Used
  mlirModule.walk([&](mlir::LLVM::GlobalOp mlirGlob) {
    llvm::GlobalVariable *llvmGlob = llvmModule->getGlobalVariable(
        mlirGlob.getSymName(), /*AllowInternal=*/true);
    if (!llvmGlob)
      return;

    if (mlirGlob->hasAttr("moksha.zeroinit")) {
      llvmGlob->setInitializer(
          llvm::Constant::getNullValue(llvmGlob->getValueType()));
    }

    // Apply Section
    if (auto secAttr =
            mlirGlob->getAttrOfType<mlir::StringAttr>("moksha.section")) {
      llvmGlob->setSection(secAttr.getValue());
    }

    // Apply Linkage
    if (auto linkAttr =
            mlirGlob->getAttrOfType<mlir::StringAttr>("moksha.linkage")) {
      llvm::StringRef linkage = linkAttr.getValue();

      if (linkage == "weak") {
        llvmGlob->setLinkage(llvm::GlobalValue::WeakAnyLinkage);
      } else if (linkage == "linkonce") {
        llvmGlob->setLinkage(llvm::GlobalValue::LinkOnceODRLinkage);
        if (targetTriple.isOSWindows()) {
          llvm::Comdat *comdat =
              llvmModule->getOrInsertComdat(llvmGlob->getName());
          comdat->setSelectionKind(llvm::Comdat::Any);
          llvmGlob->setComdat(comdat);
        }
      } else if (linkage == "internal") {
        llvmGlob->setLinkage(llvm::GlobalValue::InternalLinkage);
      } else if (linkage == "external") {
        llvmGlob->setLinkage(llvm::GlobalValue::ExternalLinkage);
      }
    }

    // Assign Thread-Local Storage Model
    if (mlirGlob->hasAttr("moksha.thread_local")) {
      llvmGlob->setThreadLocalMode(llvm::GlobalValue::GeneralDynamicTLSModel);
    }

    if (mlirGlob->hasAttr("moksha.used")) {
      usedValues.push_back(llvmGlob);
    }
  });

  // 6. Construct the @llvm.used array in the LLVM module
  if (!usedValues.empty()) {
    llvm::appendToUsed(*llvmModule, usedValues);
  }

  // llvm::errs() << "[DEBUG] TranslateToLLVM Complete!\n";
  return llvmModule;
}

} // namespace moksha
