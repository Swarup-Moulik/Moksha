// Standard MLIR base definitions
include "mlir/IR/OpBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/IR/BuiltinTypes.td"

// Your Dialect definition
include "MokshaDialect.td"

//===----------------------------------------------------------------------===//
// Moksha Types
//===----------------------------------------------------------------------===//

def Moksha_PointerType : Moksha_Type<"Pointer", "ptr"> {
    let summary = "A managed or raw pointer type.";
    let description = [{
        Represents a pointer to a specific type. Used for both stack
        allocations and managed reference types.
    }];
    // Use the fully qualified C++ type for the parameter
    let parameters = (ins "::mlir::Type":$pointee);
    let assemblyFormat = "`<` $pointee `>`";
}

//===----------------------------------------------------------------------===//
// Moksha Operations
//===----------------------------------------------------------------------===//

// Arithmetic: Addition
def Moksha_AddOp : Moksha_Op<"add", [Pure]> {
    let summary = "Addition operation";
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$result);
    let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($result)";
}

// Memory: Alloca
def Moksha_AllocaOp : Moksha_Op<"alloca"> {
    let summary = "Stack allocation";
    let description = [{
        Allocates memory on the stack for a specific type.
        Returns a moksha.ptr.
    }];
    let arguments = (ins TypeAttr:$elemType);
    let results = (outs Moksha_PointerType:$res);
    let assemblyFormat = "$elemType attr-dict `:` type($res)";
}

// Memory: Load
def Moksha_LoadOp : Moksha_Op<"load"> {
    let summary = "Load from a pointer";
    let arguments = (ins Moksha_PointerType:$ptr);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$ptr attr-dict `:` type($ptr) `->` type($res)";
}

// Memory: Store
def Moksha_StoreOp : Moksha_Op<"store"> {
    let summary = "Store value to a pointer";
    let arguments = (ins AnyType:$value, Moksha_PointerType:$ptr);
    let assemblyFormat = "$value `,` $ptr attr-dict `:` type($value) `,` type($ptr)";
}

// ARC: Retain
def Moksha_RetainOp : Moksha_Op<"retain"> {
    let summary = "Increments reference count.";
    let description = [{
        Signals to the ARC runtime that a reference is being held.
        Side-effecting on the runtime reference count.
    }];
    let arguments = (ins Moksha_PointerType:$value);
    let assemblyFormat = "$value attr-dict `:` type($value)";
}

// ARC: Release
def Moksha_ReleaseOp : Moksha_Op<"release"> {
    let summary = "Decrements reference count.";
    let description = [{
        Signals that a reference is no longer needed.
        May trigger deallocation.
    }];
    let arguments = (ins Moksha_PointerType:$value);
    let assemblyFormat = "$value attr-dict `:` type($value)";
}
