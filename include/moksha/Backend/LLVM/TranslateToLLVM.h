#pragma once

#include "mlir/IR/BuiltinOps.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include <memory>

namespace moksha {

// Converts a lowered MLIR Module into an actual LLVM IR Module
std::unique_ptr<llvm::Module>
translateMokshaToLLVMIR(mlir::ModuleOp mlirModule,
                        llvm::LLVMContext &llvmContext);

} // namespace moksha
