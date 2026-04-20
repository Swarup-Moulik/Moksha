#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/IR/DialectRegistry.h"
#include "mlir/Tools/mlir-opt/MlirOptMain.h"

// Include your custom dialect header
#include "moksha/Dialect/MokshaDialect.h"

int main(int argc, char **argv) {
  mlir::DialectRegistry registry;

  // Register only the specific dialects Moksha actually uses!
  // This bypasses the massive (and broken) InitAllDialects aggregate.
  registry.insert<mlir::func::FuncDialect, mlir::cf::ControlFlowDialect,
                  mlir::LLVM::LLVMDialect, moksha::IR::MokshaDialect>();

  // Delegate execution to the standard MLIR opt driver
  return mlir::asMainReturnCode(
      mlir::MlirOptMain(argc, argv, "Moksha Optimizer Driver\n", registry));
}
