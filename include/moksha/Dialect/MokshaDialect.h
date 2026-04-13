#pragma once

#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/Dialect.h"

// 1. Include Dialect Declaration
#include "moksha/Dialect/MokshaDialect.h.inc"

// 2. Include Types (Operations belong in MokshaOps.h!)
#define GET_TYPEDEF_CLASSES
#include "moksha/Dialect/MokshaOpsTypes.h.inc"
