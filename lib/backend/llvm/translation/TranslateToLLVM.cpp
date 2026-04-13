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
#include "llvm/IR/Function.h"
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

  // Modern LLVM 22 Signature
  mlir::LogicalResult
  amendOperation(mlir::Operation *op,
                 llvm::ArrayRef<llvm::Instruction *> instructions,
                 mlir::NamedAttribute attribute,
                 mlir::LLVM::ModuleTranslation &state) const override {
    return mlir::success(); // Safely ignore the moksha attribute
  }
};

std::unique_ptr<llvm::Module>
translateMokshaToLLVMIR(mlir::ModuleOp mlirModule,
                        llvm::LLVMContext &llvmContext) {
  // 1. Register translations
  mlir::registerBuiltinDialectTranslation(*mlirModule->getContext());
  mlir::registerLLVMDialectTranslation(*mlirModule->getContext());

  mlirModule->getContext()
      ->getOrLoadDialect<moksha::IR::MokshaDialect>()
      ->addInterfaces<MokshaDialectLLVMIRTranslationInterface>();

  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  std::string tripleStr = llvm::sys::getDefaultTargetTriple();
  llvm::Triple targetTriple(tripleStr); // Create the LLVM Triple object!

  mlirModule->setAttr(
      "llvm.target_triple",
      mlir::StringAttr::get(mlirModule->getContext(), tripleStr));

  std::string error;
  if (auto target = llvm::TargetRegistry::lookupTarget(tripleStr, error)) {
    llvm::TargetOptions opt;
    // Pass the 'targetTriple' object here instead of the string!
    if (auto tm = target->createTargetMachine(targetTriple, "generic", "", opt,
                                              std::nullopt)) {
      std::string dl = tm->createDataLayout().getStringRepresentation();
      mlirModule->setAttr("llvm.data_layout",
                          mlir::StringAttr::get(mlirModule->getContext(), dl));
    }
  }

  llvm::errs() << "\n=== [Debug] FINAL CONVERTED MLIR (Right Before LLVM "
                  "Translator) ===\n";
  mlirModule.print(llvm::errs());
  llvm::errs() << "\n=========================================================="
                  "=========\n";

  // 3. Perform the translation
  llvm::errs() << "[Debug] Entering translateModuleToLLVMIR...\n";
  std::unique_ptr<llvm::Module> llvmModule =
      mlir::translateModuleToLLVMIR(mlirModule, llvmContext);

  if (!llvmModule) {
    llvm::errs() << "[FATAL] Translation returned nullptr.\n";
    return nullptr;
  }
  llvm::errs() << "[Debug] LLVM IR Translation completed successfully!\n";

  // --- [FIX 1] Define the standard C/C++ personality function ---
  llvm::Type *i32Ty = llvm::Type::getInt32Ty(llvmContext);
  llvm::FunctionType *persType =
      llvm::FunctionType::get(i32Ty, true); // Variadic
  llvm::FunctionCallee persFuncCallee =
      llvmModule->getOrInsertFunction("__gcc_personality_v0", persType);
  llvm::Constant *persFunc =
      llvm::cast<llvm::Constant>(persFuncCallee.getCallee());

  std::vector<llvm::GlobalValue *> usedValues;

  // 4. Post-process to inject custom attributes and ABI specs
  mlirModule.walk([&](mlir::LLVM::LLVMFuncOp mlirFunc) {
    llvm::Function *llvmFunc = llvmModule->getFunction(mlirFunc.getName());
    if (!llvmFunc)
      return;

    // --- Standard LLVM Keyword Bindings ---
    if (mlirFunc->hasAttr("moksha.interrupt")) {
      llvmFunc->addFnAttr("interrupt"); // Arch-specific string attr
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
      llvmFunc->addFnAttr(
          llvm::Attribute::NoUnwind); // Pure implies it cannot throw
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
      } else if (cc == "interrupt") {
        llvmFunc->setCallingConv(llvm::CallingConv::X86_INTR);
      }
    }

    if (auto linkageAttr =
            mlirFunc->getAttrOfType<mlir::StringAttr>("moksha.linkage")) {
      llvm::StringRef linkage = linkageAttr.getValue();
      if (linkage == "weak") {
        llvmFunc->setLinkage(llvm::GlobalValue::WeakAnyLinkage);
      } else if (linkage == "internal") {
        llvmFunc->setLinkage(llvm::GlobalValue::InternalLinkage);
      }
      // "external" is the LLVM default, so we don't strictly need to set it
    }

    if (auto secAttr =
            mlirFunc->getAttrOfType<mlir::StringAttr>("moksha.section")) {
      llvmFunc->setSection(secAttr.getValue());
    }

    // --- [FIX 2] Automatically detect and attach the Personality Function! ---
    bool hasLandingPad = false;
    mlirFunc.walk([&](mlir::LLVM::LandingpadOp op) { hasLandingPad = true; });

    if (hasLandingPad) {
      llvmFunc->setPersonalityFn(persFunc);
    }
  });

  // 5. Post-process Globals for TLS and Used
  mlirModule.walk([&](mlir::LLVM::GlobalOp mlirGlob) {
    llvm::GlobalVariable *llvmGlob = llvmModule->getGlobalVariable(
        mlirGlob.getSymName(), /*AllowInternal=*/true);
    if (!llvmGlob)
      return;

    // --- FIX 1: Convert 'undef' to strict 'zeroinitializer' ---
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
      if (linkage == "internal") {
        llvmGlob->setLinkage(llvm::GlobalValue::InternalLinkage);
      } else if (linkage == "weak") {
        llvmGlob->setLinkage(llvm::GlobalValue::WeakAnyLinkage);
      } else if (linkage == "external") {
        // --- FIX 2: Explicitly map external to prevent 'internal' default ---
        llvmGlob->setLinkage(llvm::GlobalValue::ExternalLinkage);
      }
    }

    // Assign Thread-Local Storage Model
    if (mlirGlob->hasAttr("moksha.thread_local")) {
      llvmGlob->setThreadLocalMode(llvm::GlobalValue::GeneralDynamicTLSModel);
    }

    // Route 'used' globals to the LLVM used array
    if (mlirGlob->hasAttr("moksha.used")) {
      usedValues.push_back(llvmGlob);
    }
  });

  // 6. Construct the @llvm.used array in the LLVM module
  if (!usedValues.empty()) {
    llvm::appendToUsed(*llvmModule, usedValues);
  }

  return llvmModule;
}

} // namespace moksha
