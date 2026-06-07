#ifndef MOKSHA_INTERFACES
#define MOKSHA_INTERFACES

include "mlir/IR/OpBase.td"

// ============================================================================
// ARC Operation Interface
// ============================================================================

def Moksha_ARCOpInterface : OpInterface<"ARCOpInterface"> {
    let description = [{
        An interface for operations that interact with Moksha's Automatic
        Reference Counting (ARC) system. Passes can use this to universally
        query and optimize retain/release traffic.
    }];

    // The C++ namespace for the generated interface
    let cppNamespace = "moksha::IR";

    let methods = [
        InterfaceMethod<
            "Returns the underlying pointer/object being managed by this ARC operation.",
            "::mlir::Value", "getARCManagedValue"
        >,
        InterfaceMethod<
            "Returns true if this operation increments the reference count.",
            "bool", "isRetain"
        >,
        InterfaceMethod<
            "Returns true if this operation decrements the reference count.",
            "bool", "isRelease"
        >
    ];
}

// ============================================================================
// Async / Concurrency Interface
// ============================================================================

def Moksha_AsyncOpInterface : OpInterface<"AsyncOpInterface"> {
    let description = [{
        Identifies operations that yield or spawn execution (like spawn/await).
    }];
    let cppNamespace = "moksha::IR";

    let methods = [
        InterfaceMethod<
            "Returns true if this operation blocks the current thread/task.",
            "bool", "isBlocking"
        >
    ];
}

#endif // MOKSHA_INTERFACES
