#pragma once

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

// 1. Include Dialect Declaration
#include "moksha/Dialect/MokshaDialect.h.inc"

// [FIX] Define Types BEFORE Operations
#define GET_TYPEDEF_CLASSES
#include "moksha/Dialect/MokshaOpsTypes.h.inc"

namespace moksha {
} // namespace moksha

// [FIX] Now include Operations (which may use PointerType)
#define GET_OP_CLASSES
#include "moksha/Dialect/MokshaOps.h.inc"
