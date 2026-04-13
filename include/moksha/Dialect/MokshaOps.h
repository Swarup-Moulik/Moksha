#pragma once

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/Interfaces/ControlFlowInterfaces.h"
#include "mlir/Interfaces/InferTypeOpInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

// 1. Include the base dialect
#include "moksha/Dialect/MokshaDialect.h"

// 2. Include the auto-generated Interfaces FIRST
#define GET_OP_INTERFACE_DECLS
#include "moksha/Dialect/MokshaInterfaces.h.inc"

// 3. Auto-generate the C++ Operation classes
#define GET_OP_CLASSES
#include "moksha/Dialect/MokshaOps.h.inc"
