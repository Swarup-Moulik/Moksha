include "mlir/IR/OpBase.td"
include "mlir/Interfaces/SideEffectInterfaces.td"
include "mlir/Interfaces/ControlFlowInterfaces.td"
include "mlir/IR/BuiltinTypes.td"
include "mlir/IR/SymbolInterfaces.td"
include "MokshaInterfaces.td"
include "MokshaDialect.td"
include "MokshaTypes.td" // Include the new types!

// === Global Variables ===

def Moksha_GlobalOp : Moksha_Op<"global", [Symbol]> {
    let summary = "Defines a global variable.";
    let description = [{
        Represents a global variable with an optional initial value.
    }];
    let arguments = (ins SymbolNameAttr:$sym_name,
                         TypeAttr:$type,
                         OptionalAttr<AnyAttr>:$initial_value);
    let assemblyFormat = "$sym_name `:` $type ($initial_value^)? attr-dict";
}

// === Constants ===

def Moksha_ConstantOp : Moksha_Op<"constant", [Pure, ConstantLike]> {
    let summary = "Materializes a constant value.";
    let arguments = (ins AnyAttr:$value);
    let results = (outs AnyType:$res);
    let hasFolder = 1;
}

// === Basic Memory Ops ===

def Moksha_AllocaOp : Moksha_Op<"alloca"> {
    let summary = "Allocates memory and returns a pointer.";
    let arguments = (ins TypeAttr:$allocatedType);
    let results = (outs Moksha_PointerType:$res);
    let assemblyFormat = "$allocatedType attr-dict `:` type($res)";
}

def Moksha_LoadOp : Moksha_Op<"load", [MemoryEffects<[MemRead]>]> {
    let summary = "Loads a value from a pointer.";
    let arguments = (ins Moksha_PointerType:$ptr);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$ptr attr-dict `:` type($ptr) `->` type($res)";
}

def Moksha_StoreOp : Moksha_Op<"store", [MemoryEffects<[MemWrite]>]> {
    let summary = "Stores a value to a pointer.";
    let arguments = (ins AnyType:$value, Moksha_PointerType:$ptr);
    let assemblyFormat = "$value `,` $ptr attr-dict `:` type($value) `,` type($ptr)";
}

def Moksha_StoreWeakOp : Moksha_Op<"store_weak", [MemoryEffects<[MemWrite]>]> {
    let summary = "Stores a value into a weak pointer.";
    let arguments = (ins AnyType:$value, Moksha_PointerType:$ptr);
    let assemblyFormat = "$value `,` $ptr attr-dict `:` type($value) `,` type($ptr)";
}

def Moksha_LoadWeakOp : Moksha_Op<"load_weak", [MemoryEffects<[MemRead]>]> {
    let summary = "Loads a value from a weak pointer.";
    let arguments = (ins Moksha_PointerType:$ptr);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$ptr attr-dict `:` type($ptr) `->` type($res)";
}

// === Arithmetic & Logic Ops ===

def Moksha_AddOp : Moksha_Op<"add", [Pure]> {
    let summary = "Addition operation.";
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($res)";
}

def Moksha_SubOp : Moksha_Op<"sub", [Pure]> {
    let summary = "Subtraction operation.";
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($res)";
}

def Moksha_MulOp : Moksha_Op<"mul", [Pure]> {
    let summary = "Multiplication operation.";
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($res)";
}

def Moksha_DivOp : Moksha_Op<"div", [Pure]> {
    let summary = "Division operation.";
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($res)";
}

def Moksha_PowOp : Moksha_Op<"pow", [Pure]> {
    let summary = "Exponentiation operation.";
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$lhs `,` $rhs attr-dict `:` type($lhs) `,` type($rhs) `->` type($res)";
}

def Moksha_ModOp : Moksha_Op<"mod", [Pure]> {
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
}
def Moksha_AndOp : Moksha_Op<"and", [Pure]> {
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
}
def Moksha_OrOp : Moksha_Op<"or", [Pure]> {
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
}
def Moksha_XorOp : Moksha_Op<"xor", [Pure]> {
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
}
def Moksha_ShlOp : Moksha_Op<"shl", [Pure]> {
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
}
def Moksha_ShrOp : Moksha_Op<"shr", [Pure]> {
    let arguments = (ins AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
}
def Moksha_CmpOp : Moksha_Op<"cmp", [Pure]> {
    let arguments = (ins I32Attr:$predicate, AnyType:$lhs, AnyType:$rhs);
    let results = (outs AnyType:$res);
}
def Moksha_UnreachableOp : Moksha_Op<"unreachable", [Terminator]> {
    let summary = "Terminates execution.";
}

// === Structural Ops ===

def Moksha_CallOp : Moksha_Op<"call"> {
    let summary = "Function call operation.";
    let arguments = (ins SymbolRefAttr:$callee, Variadic<AnyType>:$operands);
    let results = (outs Variadic<AnyType>:$results);
    let assemblyFormat = "$callee `(` $operands `)` attr-dict `:` functional-type($operands, $results)";
}

// === Systems & Concurrency Operations ===

// 1. Spawn (Thread/Async creation)
def Moksha_SpawnOp : Moksha_Op<"spawn", [DeclareOpInterfaceMethods<Moksha_AsyncOpInterface>]> {
    let summary = "Spawns a closure on a new thread or async executor.";
    let arguments = (ins Moksha_ClosureType:$closure, I32Attr:$threadKind);
    let results = (outs Moksha_PointerType:$promise);
    let assemblyFormat = "$closure `,` $threadKind attr-dict `:` type($closure) `->` type($promise)";
}

// 2. Await (Async suspension)
def Moksha_AwaitOp : Moksha_Op<"await", [DeclareOpInterfaceMethods<Moksha_AsyncOpInterface>]> {
    let summary = "Suspends execution until the promise is resolved.";
    let arguments = (ins Moksha_PromiseType:$promise);
    let results = (outs AnyType:$result);
    let assemblyFormat = "$promise attr-dict `:` type($promise) `->` type($result)";
}

// 3. Hardware Atomics (Compare and Exchange)
def Moksha_AtomicCmpXchgOp : Moksha_Op<"atomic_cmpxchg", [MemoryEffects<[MemWrite, MemRead]>]> {
    let summary = "Hardware atomic compare and exchange.";
    let arguments = (
        ins Moksha_PointerType:$ptr,
            AnyType:$expected,
            AnyType:$desired,
            I32Attr:$successOrder,
            I32Attr:$failureOrder
    );
    let results = (outs AnyType:$old_value);
    let assemblyFormat = "$ptr `,` $expected `,` $desired attr-dict `:` type($ptr) `,` type($expected) `,` type($desired) `->` type($old_value)";
}

// 4. Hardware Atomics (Read-Modify-Write)
def Moksha_AtomicRMWOp : Moksha_Op<"atomic_rmw", [MemoryEffects<[MemWrite, MemRead]>]> {
    let summary = "Hardware atomic read-modify-write.";
    let arguments = (
        ins I32Attr:$binOp,
            Moksha_PointerType:$ptr,
            AnyType:$value,
            I32Attr:$ordering
    );
    let results = (outs AnyType:$res);
    let assemblyFormat = "$binOp `,` $ptr `,` $value attr-dict `:` type($ptr) `,` type($value) `->` type($res)";
}

// 5. Memory Fence
def Moksha_FenceOp : Moksha_Op<"fence"> {
    let summary = "Hardware memory fence.";
    let arguments = (ins I32Attr:$ordering);
    let assemblyFormat = "attr-dict";
}

// 6. Atomic Load
def Moksha_AtomicLoadOp : Moksha_Op<"atomic_load", [MemoryEffects<[MemRead]>]> {
    let summary = "Atomic load operation.";
    let arguments = (ins Moksha_PointerType:$ptr, I32Attr:$ordering);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$ptr attr-dict `:` type($ptr) `->` type($res)";
}

// 7. Atomic Store
def Moksha_AtomicStoreOp : Moksha_Op<"atomic_store", [MemoryEffects<[MemWrite]>]> {
    let summary = "Atomic store operation.";
    let arguments = (ins AnyType:$value, Moksha_PointerType:$ptr, I32Attr:$ordering);
    let assemblyFormat = "$value `,` $ptr attr-dict `:` type($value) `,` type($ptr)";
}

// 8. Inline Assembly
def Moksha_InlineAsmOp : Moksha_Op<"asm", [MemoryEffects<[MemWrite, MemRead]>]> {
    let summary = "Inline hardware assembly block.";
    let arguments = (
        ins StrAttr:$asm_string,
            StrAttr:$constraints,
            Variadic<AnyType>:$operands
    );
    let results = (outs Variadic<AnyType>:$results);
    let assemblyFormat = "$asm_string `,` $constraints `(` $operands `)` attr-dict `:` functional-type($operands, $results)";
}

// Attach the interface to Retain
def Moksha_RetainOp : Moksha_Op<"retain", [DeclareOpInterfaceMethods<Moksha_ARCOpInterface>]> {
    let summary = "Increments reference count.";
    let arguments = (ins Moksha_PointerType:$value);
    let assemblyFormat = "$value attr-dict `:` type($value)";
}

// Attach the interface to Release
def Moksha_ReleaseOp : Moksha_Op<"release", [DeclareOpInterfaceMethods<Moksha_ARCOpInterface>]> {
    let summary = "Decrements reference count.";
    let arguments = (ins
        Moksha_PointerType:$value,
        OptionalAttr<FlatSymbolRefAttr>:$drop_func
    );
    let assemblyFormat = "$value ( `[` `drop` `:` $drop_func^ `]` )? attr-dict `:` type($value)";
}

// === Globals & Casts ===

def Moksha_AddressOfOp : Moksha_Op<"address_of", [Pure]> {
    let summary = "Gets the address of a global variable.";
    let arguments = (ins FlatSymbolRefAttr:$global_name);
    let results = (outs Moksha_PointerType:$res);
    let assemblyFormat = "$global_name attr-dict `:` type($res)";
}

def Moksha_CastOp : Moksha_Op<"cast", [Pure]> {
    let summary = "Type cast operation.";
    let arguments = (ins AnyType:$value);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$value attr-dict `:` type($value) `to` type($res)";
}

// === Aggregate & Pointer Ops ===

def Moksha_GetElementPtrOp : Moksha_Op<"gep", [Pure]> {
    let summary = "Get element pointer";
    let arguments = (ins Moksha_PointerType:$base, Variadic<AnyType>:$indices);
    let results = (outs Moksha_PointerType:$res);
    let assemblyFormat = "$base `[` $indices `]` attr-dict `:` type($base) `,` type($indices) `->` type($res)";
}

def Moksha_ExtractValueOp : Moksha_Op<"extract_value", [Pure]> {
    let summary = "Extract value from aggregate";
    let arguments = (ins AnyType:$aggregate, I32Attr:$index);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$aggregate `[` $index `]` attr-dict `:` type($aggregate) `->` type($res)";
}

def Moksha_InsertValueOp : Moksha_Op<"insert_value", [Pure]> {
    let summary = "Insert value into aggregate";
    let arguments = (ins AnyType:$aggregate, AnyType:$value, I32Attr:$index);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$value `into` $aggregate `[` $index `]` attr-dict `:` type($value) `,` type($aggregate) `->` type($res)";
}

// === Closures ===

def Moksha_MakeClosureOp : Moksha_Op<"make_closure", [Pure]> {
    let summary = "Creates a closure (fat pointer) from a function and its captured environment.";
    let arguments = (ins FlatSymbolRefAttr:$callee, Variadic<AnyType>:$captures);
    let results = (outs AnyType:$res);
    let assemblyFormat = "$callee `(` $captures `)` attr-dict `:` type($captures) `->` type($res)";
}

// === Exceptions ===

def Moksha_ThrowOp : Moksha_Op<"throw", [Terminator]> {
    let summary = "Throws an exception and unwinds the stack.";
    // It takes the thrown value, and optionally branches to a local catch block
    let arguments = (ins AnyType:$value);
    let successors = (successor VariadicSuccessor<AnySuccessor>:$unwindDest);
}

def Moksha_InvokeOp : Moksha_Op<"invoke", [Terminator]> {
    let summary = "Calls a function that may throw, with normal and unwind destinations.";
    let arguments = (ins FlatSymbolRefAttr:$callee, Variadic<AnyType>:$operands);
    let results = (outs Variadic<AnyType>:$res);
    let successors = (successor AnySuccessor:$normalDest, AnySuccessor:$unwindDest);
    let assemblyFormat = "$callee `(` $operands `)` `to` $normalDest `unwind` $unwindDest attr-dict `:` type($operands) `->` type($res)";
}

def InvokeIndirectOp : Moksha_Op<"invoke_indirect", [Terminator]> {
  let summary = "Indirectly invoke a function pointer with an exception handler";
  let arguments = (ins
    AnyType:$callee,
    Variadic<AnyType>:$callArgs
  );
  let results = (outs Variadic<AnyType>);
  let successors = (successor AnySuccessor:$normalDest, AnySuccessor:$unwindDest);

  let assemblyFormat = [{
    $callee `(` $callArgs `)` `to` $normalDest `unwind` $unwindDest attr-dict
    `:` type($callee) `,` type($callArgs) `->` type(results)
  }];
}

def Moksha_LandingPadOp : Moksha_Op<"landingpad"> {
    let summary = "Catches an exception value in an unwind block.";
    let results = (outs AnyType:$res);
    let assemblyFormat = "attr-dict `:` type($res)";
}
