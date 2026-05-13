/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* TypeDef Declarations                                                       *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifdef GET_TYPEDEF_CLASSES
#undef GET_TYPEDEF_CLASSES


namespace mlir {
class AsmParser;
class AsmPrinter;
} // namespace mlir
namespace moksha::IR {

/// A managed or raw pointer type.
class PointerType;
/// A fixed-point decimal type for financial/AI math.
class DecimalType;
/// A fat pointer representing a function and its captured environment.
class ClosureType;
/// A handle to an asynchronously executing task.
class PromiseType;
/// A handle to a spawned thread.
class ThreadType;
/// Represents a dynamically typed 'any' value.
class AnyType;
/// A fixed-size Moksha array.
class ArrayType;
/// A dynamic slice/span of an array.
class SliceType;
/// A stack-allocated value type (C++ style class).
class ClassType;
/// A heap-allocated, ARC-managed reference type (Java style class).
class RefClassType;
/// A wrapper type that can hold a value or be null.
class NullableType;
/// The explicit type of the null literal.
class NullType;
/// A key-value map/table type.
class MapType;
namespace detail {

struct PointerTypeStorage;

} // namespace detail
class PointerType : public ::mlir::Type::TypeBase<PointerType, ::mlir::Type, detail::PointerTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.ptr";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static PointerType get(::mlir::MLIRContext *context, ::mlir::Type pointee);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"ptr"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  ::mlir::Type getPointee() const;
};
namespace detail {

struct DecimalTypeStorage;

} // namespace detail
class DecimalType : public ::mlir::Type::TypeBase<DecimalType, ::mlir::Type, detail::DecimalTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.dec";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static DecimalType get(::mlir::MLIRContext *context, unsigned precision, unsigned scale);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"dec"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  unsigned getPrecision() const;
  unsigned getScale() const;
};
namespace detail {

struct ClosureTypeStorage;

} // namespace detail
class ClosureType : public ::mlir::Type::TypeBase<ClosureType, ::mlir::Type, detail::ClosureTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.closure";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static ClosureType get(::mlir::MLIRContext *context, ::mlir::Type returnType, ::llvm::ArrayRef<::mlir::Type> inputs);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"closure"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  ::mlir::Type getReturnType() const;
  ::llvm::ArrayRef<::mlir::Type> getInputs() const;
};
namespace detail {

struct PromiseTypeStorage;

} // namespace detail
class PromiseType : public ::mlir::Type::TypeBase<PromiseType, ::mlir::Type, detail::PromiseTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.promise";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static PromiseType get(::mlir::MLIRContext *context, ::mlir::Type resolvedType);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"promise"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  ::mlir::Type getResolvedType() const;
};
class ThreadType : public ::mlir::Type::TypeBase<ThreadType, ::mlir::Type, ::mlir::TypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.thread";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"thread"};
  }

};
class AnyType : public ::mlir::Type::TypeBase<AnyType, ::mlir::Type, ::mlir::TypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.any";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"any"};
  }

};
namespace detail {

struct ArrayTypeStorage;

} // namespace detail
class ArrayType : public ::mlir::Type::TypeBase<ArrayType, ::mlir::Type, detail::ArrayTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.array";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static ArrayType get(::mlir::MLIRContext *context, ::mlir::Type elementType, unsigned size);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"array"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  ::mlir::Type getElementType() const;
  unsigned getSize() const;
};
namespace detail {

struct SliceTypeStorage;

} // namespace detail
class SliceType : public ::mlir::Type::TypeBase<SliceType, ::mlir::Type, detail::SliceTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.slice";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static SliceType get(::mlir::MLIRContext *context, ::mlir::Type elementType);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"slice"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  ::mlir::Type getElementType() const;
};
namespace detail {

struct ClassTypeStorage;

} // namespace detail
class ClassType : public ::mlir::Type::TypeBase<ClassType, ::mlir::Type, detail::ClassTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.class";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static ClassType get(::mlir::MLIRContext *context, ::llvm::StringRef className);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"class"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  ::llvm::StringRef getClassName() const;
};
namespace detail {

struct RefClassTypeStorage;

} // namespace detail
class RefClassType : public ::mlir::Type::TypeBase<RefClassType, ::mlir::Type, detail::RefClassTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.ref_class";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static RefClassType get(::mlir::MLIRContext *context, ::llvm::StringRef className);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"ref_class"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  ::llvm::StringRef getClassName() const;
};
namespace detail {

struct NullableTypeStorage;

} // namespace detail
class NullableType : public ::mlir::Type::TypeBase<NullableType, ::mlir::Type, detail::NullableTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.nullable";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static NullableType get(::mlir::MLIRContext *context, ::mlir::Type innerType);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"nullable"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  ::mlir::Type getInnerType() const;
};
class NullType : public ::mlir::Type::TypeBase<NullType, ::mlir::Type, ::mlir::TypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.null";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"null"};
  }

};
namespace detail {

struct MapTypeStorage;

} // namespace detail
class MapType : public ::mlir::Type::TypeBase<MapType, ::mlir::Type, detail::MapTypeStorage> {
public:
  using Base::Base;
  static constexpr ::llvm::StringLiteral name = "moksha.table";
  static constexpr ::llvm::StringLiteral dialectName = "moksha";
  static MapType get(::mlir::MLIRContext *context, ::mlir::Type keyType, ::mlir::Type valueType);
  static constexpr ::llvm::StringLiteral getMnemonic() {
    return {"table"};
  }

  static ::mlir::Type parse(::mlir::AsmParser &odsParser);
  void print(::mlir::AsmPrinter &odsPrinter) const;
  ::mlir::Type getKeyType() const;
  ::mlir::Type getValueType() const;
};

} // namespace moksha::IR
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::PointerType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::DecimalType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::ClosureType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::PromiseType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::ThreadType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::AnyType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::ArrayType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::SliceType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::ClassType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::RefClassType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::NullableType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::NullType)
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::MapType)

#endif // GET_TYPEDEF_CLASSES

