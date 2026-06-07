// include/moksha/Dialect/MokshaTypes.td
#ifndef MOKSHA_TYPES
#define MOKSHA_TYPES

include "MokshaDialect.td"

// 1. The Pointer Type (Already mostly correct, just moved here)
def Moksha_PointerType : Moksha_Type<"Pointer", "ptr"> {
    let summary = "A managed or raw pointer type.";
    let parameters = (ins "::mlir::Type":$pointee);
    let assemblyFormat = "`<` $pointee `>`";
}

// 2. The Decimal Type (Fixed-Point Math)
def Moksha_DecimalType : Moksha_Type<"Decimal", "dec"> {
    let summary = "A fixed-point decimal type for financial/AI math.";
    let parameters = (ins "unsigned":$precision, "unsigned":$scale);
    let assemblyFormat = "`<` $precision `,` $scale `>`";
}

// 3. The Closure Type
def Moksha_ClosureType : Moksha_Type<"Closure", "closure"> {
    let summary = "A fat pointer representing a function and its captured environment.";
    let parameters = (
        ins "::mlir::Type":$returnType,
            ArrayRefParameter<"::mlir::Type">:$inputs
    );
    let assemblyFormat = "`<` `(` $inputs `)` `->` $returnType `>`";
}

// 4. The Promise Type (Async/Await)
def Moksha_PromiseType : Moksha_Type<"Promise", "promise"> {
    let summary = "A handle to an asynchronously executing task.";
    let parameters = (ins "::mlir::Type":$resolvedType);
    let assemblyFormat = "`<` $resolvedType `>`";
}

// 5. The Thread Type (Handle for running threads)
def Moksha_ThreadType : Moksha_Type<"Thread", "thread"> {
    let summary = "A handle to a spawned thread.";
}

// 6. The Any Type (Dynamic Typing)
def Moksha_AnyType : Moksha_Type<"Any", "any"> {
    let summary = "Represents a dynamically typed 'any' value.";
}

// 7. The Array Type (Fixed Size)
def Moksha_ArrayType : Moksha_Type<"Array", "array"> {
    let summary = "A fixed-size Moksha array.";
    let parameters = (ins "::mlir::Type":$elementType, "unsigned":$size);
    let assemblyFormat = "`<` $elementType `,` $size `>`";
}

// 8. The Slice Type (Dynamic Array / Span)
def Moksha_SliceType : Moksha_Type<"Slice", "slice"> {
    let summary = "A dynamic slice/span of an array.";
    let parameters = (ins "::mlir::Type":$elementType);
    let assemblyFormat = "`<` $elementType `>`";
}

// 9. The Value Class Type (Stack Allocated / C++ Style)
def Moksha_ClassType : Moksha_Type<"Class", "class"> {
    let summary = "A stack-allocated value type (C++ style class).";
    let parameters = (ins StringRefParameter<"">:$className);
    let assemblyFormat = "`<` $className `>`";
}

// 10. The Ref Class Type (Heap Allocated / Java Style)
def Moksha_RefClassType : Moksha_Type<"RefClass", "ref_class"> {
    let summary = "A heap-allocated, ARC-managed reference type (Java style class).";
    let parameters = (ins StringRefParameter<"">:$className);
    let assemblyFormat = "`<` $className `>`";
}

// 11. The Nullable Type (T?)
def Moksha_NullableType : Moksha_Type<"Nullable", "nullable"> {
    let summary = "A wrapper type that can hold a value or be null.";
    let parameters = (ins "::mlir::Type":$innerType);
    let assemblyFormat = "`<` $innerType `>`";
}

// 12. The Null Type
def Moksha_NullType : Moksha_Type<"Null", "null"> {
    let summary = "The explicit type of the null literal.";
}

// 13. The Map Type (Table)
def Moksha_MapType : Moksha_Type<"Map", "table"> {
    let summary = "A key-value map/table type.";
    let parameters = (ins "::mlir::Type":$keyType, "::mlir::Type":$valueType);
    let assemblyFormat = "`<` $keyType `,` $valueType `>`";
}

#endif // MOKSHA_TYPES
