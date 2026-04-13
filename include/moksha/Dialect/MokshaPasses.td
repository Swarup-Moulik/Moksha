#ifndef MOKSHA_PASSES
#define MOKSHA_PASSES

include "mlir/Pass/PassBase.td"

// Pass 1: ARC Optimization
def CanonicalizeARCPass : Pass<"moksha-opt-arc", "::mlir::ModuleOp"> {
  let summary = "Removes redundant retain/release pairs at the MLIR level.";
  let description = [{
    A late-stage optimization pass that uses MLIR dataflow analysis to
    elide matching `moksha.retain` and `moksha.release` instructions.
  }];
  let constructor = "moksha::createCanonicalizeARCPass()";
}

// Pass 2: Lower to LLVM
def ConvertMokshaToLLVMPass : Pass<"convert-moksha-to-llvm", "::mlir::ModuleOp"> {
  let summary = "Lower Moksha Dialect to LLVM Dialect.";
  let constructor = "moksha::createConvertMokshaToLLVMPass()";
}

// Pass 3: Lower to Linalg (Accelerator Branch)
def ConvertMokshaToLinalgPass : Pass<"convert-moksha-to-linalg", "::mlir::ModuleOp"> {
  let summary = "Lower Moksha Math/Arrays to Linalg Dialect.";
  let constructor = "moksha::createConvertMokshaToLinalgPass()";
}

#endif // MOKSHA_PASSES
