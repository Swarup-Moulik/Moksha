#include "moksha/Dialect/MokshaOps.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/OpImplementation.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h" // [FIX] Added for BranchOpInterface
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h" // [FIX] Added for MemoryEffects
#include "moksha/Dialect/MokshaDialect.h"         // [FIX] Added dialect header

// Generate the C++ code for operation definitions
#define GET_OP_CLASSES
#include "moksha/Dialect/MokshaOps.cpp.inc"
