/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* TypeDef Definitions                                                        *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifdef GET_TYPEDEF_LIST
#undef GET_TYPEDEF_LIST

moksha::IR::PointerType,
moksha::IR::DecimalType,
moksha::IR::ClosureType,
moksha::IR::PromiseType,
moksha::IR::ThreadType,
moksha::IR::AnyType,
moksha::IR::ArrayType,
moksha::IR::SliceType,
moksha::IR::ClassType,
moksha::IR::RefClassType,
moksha::IR::NullableType,
moksha::IR::NullType,
moksha::IR::MapType

#endif // GET_TYPEDEF_LIST

#ifdef GET_TYPEDEF_CLASSES
#undef GET_TYPEDEF_CLASSES

static ::mlir::OptionalParseResult generatedTypeParser(::mlir::AsmParser &parser, ::llvm::StringRef *mnemonic, ::mlir::Type &value) {
  return ::mlir::AsmParser::KeywordSwitch<::mlir::OptionalParseResult>(parser)
    .Case(moksha::IR::PointerType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::PointerType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::DecimalType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::DecimalType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::ClosureType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::ClosureType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::PromiseType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::PromiseType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::ThreadType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::ThreadType::get(parser.getContext());
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::AnyType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::AnyType::get(parser.getContext());
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::ArrayType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::ArrayType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::SliceType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::SliceType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::ClassType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::ClassType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::RefClassType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::RefClassType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::NullableType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::NullableType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::NullType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::NullType::get(parser.getContext());
      return ::mlir::success(!!value);
    })
    .Case(moksha::IR::MapType::getMnemonic(), [&](llvm::StringRef, llvm::SMLoc) {
      value = moksha::IR::MapType::parse(parser);
      return ::mlir::success(!!value);
    })
    .Default([&](llvm::StringRef keyword, llvm::SMLoc) {
      *mnemonic = keyword;
      return std::nullopt;
    });
}

static ::llvm::LogicalResult generatedTypePrinter(::mlir::Type def, ::mlir::AsmPrinter &printer) {
  return ::llvm::TypeSwitch<::mlir::Type, ::llvm::LogicalResult>(def)    .Case<moksha::IR::PointerType>([&](auto t) {
      printer << moksha::IR::PointerType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Case<moksha::IR::DecimalType>([&](auto t) {
      printer << moksha::IR::DecimalType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Case<moksha::IR::ClosureType>([&](auto t) {
      printer << moksha::IR::ClosureType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Case<moksha::IR::PromiseType>([&](auto t) {
      printer << moksha::IR::PromiseType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Case<moksha::IR::ThreadType>([&](auto t) {
      printer << moksha::IR::ThreadType::getMnemonic();
      return ::mlir::success();
    })
    .Case<moksha::IR::AnyType>([&](auto t) {
      printer << moksha::IR::AnyType::getMnemonic();
      return ::mlir::success();
    })
    .Case<moksha::IR::ArrayType>([&](auto t) {
      printer << moksha::IR::ArrayType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Case<moksha::IR::SliceType>([&](auto t) {
      printer << moksha::IR::SliceType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Case<moksha::IR::ClassType>([&](auto t) {
      printer << moksha::IR::ClassType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Case<moksha::IR::RefClassType>([&](auto t) {
      printer << moksha::IR::RefClassType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Case<moksha::IR::NullableType>([&](auto t) {
      printer << moksha::IR::NullableType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Case<moksha::IR::NullType>([&](auto t) {
      printer << moksha::IR::NullType::getMnemonic();
      return ::mlir::success();
    })
    .Case<moksha::IR::MapType>([&](auto t) {
      printer << moksha::IR::MapType::getMnemonic();
t.print(printer);
      return ::mlir::success();
    })
    .Default([](auto) { return ::mlir::failure(); });
}

namespace moksha::IR {

namespace detail {

struct PointerTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<::mlir::Type>;
  PointerTypeStorage(::mlir::Type pointee) : pointee(std::move(pointee)) {}

  KeyTy getAsKey() const {
    return KeyTy(pointee);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (pointee == std::get<0>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey));
  }

  static PointerTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto pointee = std::move(std::get<0>(tblgenKey));
    return new (allocator.allocate<PointerTypeStorage>()) PointerTypeStorage(std::move(pointee));
  }

  ::mlir::Type pointee;
};

} // namespace detail
PointerType PointerType::get(::mlir::MLIRContext *context, ::mlir::Type pointee) {
  return Base::get(context, std::move(pointee));
}

::mlir::Type PointerType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<::mlir::Type> _result_pointee;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};

  // Parse variable 'pointee'
  _result_pointee = ::mlir::FieldParser<::mlir::Type>::parse(odsParser);
  if (::mlir::failed(_result_pointee)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_PointerType parameter 'pointee' which is to be a `::mlir::Type`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_pointee));
  return PointerType::get(odsParser.getContext(),
      ::mlir::Type((*_result_pointee)));
}

void PointerType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter.printStrippedAttrOrType(getPointee());
  odsPrinter << ">";
}

::mlir::Type PointerType::getPointee() const {
  return getImpl()->pointee;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::PointerType)
namespace moksha::IR {

namespace detail {

struct DecimalTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<unsigned, unsigned>;
  DecimalTypeStorage(unsigned precision, unsigned scale) : precision(std::move(precision)), scale(std::move(scale)) {}

  KeyTy getAsKey() const {
    return KeyTy(precision, scale);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (precision == std::get<0>(tblgenKey)) && (scale == std::get<1>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey), std::get<1>(tblgenKey));
  }

  static DecimalTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto precision = std::move(std::get<0>(tblgenKey));
    auto scale = std::move(std::get<1>(tblgenKey));
    return new (allocator.allocate<DecimalTypeStorage>()) DecimalTypeStorage(std::move(precision), std::move(scale));
  }

  unsigned precision;
  unsigned scale;
};

} // namespace detail
DecimalType DecimalType::get(::mlir::MLIRContext *context, unsigned precision, unsigned scale) {
  return Base::get(context, std::move(precision), std::move(scale));
}

::mlir::Type DecimalType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<unsigned> _result_precision;
  ::mlir::FailureOr<unsigned> _result_scale;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};

  // Parse variable 'precision'
  _result_precision = ::mlir::FieldParser<unsigned>::parse(odsParser);
  if (::mlir::failed(_result_precision)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_DecimalType parameter 'precision' which is to be a `unsigned`");
    return {};
  }
  // Parse literal ','
  if (odsParser.parseComma()) return {};

  // Parse variable 'scale'
  _result_scale = ::mlir::FieldParser<unsigned>::parse(odsParser);
  if (::mlir::failed(_result_scale)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_DecimalType parameter 'scale' which is to be a `unsigned`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_precision));
  assert(::mlir::succeeded(_result_scale));
  return DecimalType::get(odsParser.getContext(),
      unsigned((*_result_precision)),
      unsigned((*_result_scale)));
}

void DecimalType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter.printStrippedAttrOrType(getPrecision());
  odsPrinter << ",";
  odsPrinter << ' ';
  odsPrinter.printStrippedAttrOrType(getScale());
  odsPrinter << ">";
}

unsigned DecimalType::getPrecision() const {
  return getImpl()->precision;
}

unsigned DecimalType::getScale() const {
  return getImpl()->scale;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::DecimalType)
namespace moksha::IR {

namespace detail {

struct ClosureTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<::mlir::Type, ::llvm::ArrayRef<::mlir::Type>>;
  ClosureTypeStorage(::mlir::Type returnType, ::llvm::ArrayRef<::mlir::Type> inputs) : returnType(std::move(returnType)), inputs(std::move(inputs)) {}

  KeyTy getAsKey() const {
    return KeyTy(returnType, inputs);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (returnType == std::get<0>(tblgenKey)) && (inputs == std::get<1>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey), std::get<1>(tblgenKey));
  }

  static ClosureTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto returnType = std::move(std::get<0>(tblgenKey));
    auto inputs = std::move(std::get<1>(tblgenKey));
    inputs = allocator.copyInto(inputs);
    return new (allocator.allocate<ClosureTypeStorage>()) ClosureTypeStorage(std::move(returnType), std::move(inputs));
  }

  ::mlir::Type returnType;
  ::llvm::ArrayRef<::mlir::Type> inputs;
};

} // namespace detail
ClosureType ClosureType::get(::mlir::MLIRContext *context, ::mlir::Type returnType, ::llvm::ArrayRef<::mlir::Type> inputs) {
  return Base::get(context, std::move(returnType), std::move(inputs));
}

::mlir::Type ClosureType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<::mlir::Type> _result_returnType;
  ::mlir::FailureOr<::llvm::SmallVector<::mlir::Type>> _result_inputs;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};
  // Parse literal '('
  if (odsParser.parseLParen()) return {};

  // Parse variable 'inputs'
  _result_inputs = ::mlir::FieldParser<::llvm::SmallVector<::mlir::Type>>::parse(odsParser);
  if (::mlir::failed(_result_inputs)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_ClosureType parameter 'inputs' which is to be a `::llvm::ArrayRef<::mlir::Type>`");
    return {};
  }
  // Parse literal ')'
  if (odsParser.parseRParen()) return {};
  // Parse literal '->'
  if (odsParser.parseArrow()) return {};

  // Parse variable 'returnType'
  _result_returnType = ::mlir::FieldParser<::mlir::Type>::parse(odsParser);
  if (::mlir::failed(_result_returnType)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_ClosureType parameter 'returnType' which is to be a `::mlir::Type`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_returnType));
  assert(::mlir::succeeded(_result_inputs));
  return ClosureType::get(odsParser.getContext(),
      ::mlir::Type((*_result_returnType)),
      ::llvm::ArrayRef<::mlir::Type>((*_result_inputs)));
}

void ClosureType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter << "(";
  odsPrinter.printStrippedAttrOrType(getInputs());
  odsPrinter << ")";
  odsPrinter << ' ' << "->";
  odsPrinter << ' ';
  odsPrinter.printStrippedAttrOrType(getReturnType());
  odsPrinter << ">";
}

::mlir::Type ClosureType::getReturnType() const {
  return getImpl()->returnType;
}

::llvm::ArrayRef<::mlir::Type> ClosureType::getInputs() const {
  return getImpl()->inputs;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ClosureType)
namespace moksha::IR {

namespace detail {

struct PromiseTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<::mlir::Type>;
  PromiseTypeStorage(::mlir::Type resolvedType) : resolvedType(std::move(resolvedType)) {}

  KeyTy getAsKey() const {
    return KeyTy(resolvedType);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (resolvedType == std::get<0>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey));
  }

  static PromiseTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto resolvedType = std::move(std::get<0>(tblgenKey));
    return new (allocator.allocate<PromiseTypeStorage>()) PromiseTypeStorage(std::move(resolvedType));
  }

  ::mlir::Type resolvedType;
};

} // namespace detail
PromiseType PromiseType::get(::mlir::MLIRContext *context, ::mlir::Type resolvedType) {
  return Base::get(context, std::move(resolvedType));
}

::mlir::Type PromiseType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<::mlir::Type> _result_resolvedType;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};

  // Parse variable 'resolvedType'
  _result_resolvedType = ::mlir::FieldParser<::mlir::Type>::parse(odsParser);
  if (::mlir::failed(_result_resolvedType)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_PromiseType parameter 'resolvedType' which is to be a `::mlir::Type`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_resolvedType));
  return PromiseType::get(odsParser.getContext(),
      ::mlir::Type((*_result_resolvedType)));
}

void PromiseType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter.printStrippedAttrOrType(getResolvedType());
  odsPrinter << ">";
}

::mlir::Type PromiseType::getResolvedType() const {
  return getImpl()->resolvedType;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::PromiseType)
namespace moksha::IR {


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ThreadType)
namespace moksha::IR {


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AnyType)
namespace moksha::IR {

namespace detail {

struct ArrayTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<::mlir::Type, unsigned>;
  ArrayTypeStorage(::mlir::Type elementType, unsigned size) : elementType(std::move(elementType)), size(std::move(size)) {}

  KeyTy getAsKey() const {
    return KeyTy(elementType, size);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (elementType == std::get<0>(tblgenKey)) && (size == std::get<1>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey), std::get<1>(tblgenKey));
  }

  static ArrayTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto elementType = std::move(std::get<0>(tblgenKey));
    auto size = std::move(std::get<1>(tblgenKey));
    return new (allocator.allocate<ArrayTypeStorage>()) ArrayTypeStorage(std::move(elementType), std::move(size));
  }

  ::mlir::Type elementType;
  unsigned size;
};

} // namespace detail
ArrayType ArrayType::get(::mlir::MLIRContext *context, ::mlir::Type elementType, unsigned size) {
  return Base::get(context, std::move(elementType), std::move(size));
}

::mlir::Type ArrayType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<::mlir::Type> _result_elementType;
  ::mlir::FailureOr<unsigned> _result_size;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};

  // Parse variable 'elementType'
  _result_elementType = ::mlir::FieldParser<::mlir::Type>::parse(odsParser);
  if (::mlir::failed(_result_elementType)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_ArrayType parameter 'elementType' which is to be a `::mlir::Type`");
    return {};
  }
  // Parse literal ','
  if (odsParser.parseComma()) return {};

  // Parse variable 'size'
  _result_size = ::mlir::FieldParser<unsigned>::parse(odsParser);
  if (::mlir::failed(_result_size)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_ArrayType parameter 'size' which is to be a `unsigned`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_elementType));
  assert(::mlir::succeeded(_result_size));
  return ArrayType::get(odsParser.getContext(),
      ::mlir::Type((*_result_elementType)),
      unsigned((*_result_size)));
}

void ArrayType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter.printStrippedAttrOrType(getElementType());
  odsPrinter << ",";
  odsPrinter << ' ';
  odsPrinter.printStrippedAttrOrType(getSize());
  odsPrinter << ">";
}

::mlir::Type ArrayType::getElementType() const {
  return getImpl()->elementType;
}

unsigned ArrayType::getSize() const {
  return getImpl()->size;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ArrayType)
namespace moksha::IR {

namespace detail {

struct SliceTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<::mlir::Type>;
  SliceTypeStorage(::mlir::Type elementType) : elementType(std::move(elementType)) {}

  KeyTy getAsKey() const {
    return KeyTy(elementType);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (elementType == std::get<0>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey));
  }

  static SliceTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto elementType = std::move(std::get<0>(tblgenKey));
    return new (allocator.allocate<SliceTypeStorage>()) SliceTypeStorage(std::move(elementType));
  }

  ::mlir::Type elementType;
};

} // namespace detail
SliceType SliceType::get(::mlir::MLIRContext *context, ::mlir::Type elementType) {
  return Base::get(context, std::move(elementType));
}

::mlir::Type SliceType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<::mlir::Type> _result_elementType;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};

  // Parse variable 'elementType'
  _result_elementType = ::mlir::FieldParser<::mlir::Type>::parse(odsParser);
  if (::mlir::failed(_result_elementType)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_SliceType parameter 'elementType' which is to be a `::mlir::Type`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_elementType));
  return SliceType::get(odsParser.getContext(),
      ::mlir::Type((*_result_elementType)));
}

void SliceType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter.printStrippedAttrOrType(getElementType());
  odsPrinter << ">";
}

::mlir::Type SliceType::getElementType() const {
  return getImpl()->elementType;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::SliceType)
namespace moksha::IR {

namespace detail {

struct ClassTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<::llvm::StringRef>;
  ClassTypeStorage(::llvm::StringRef className) : className(std::move(className)) {}

  KeyTy getAsKey() const {
    return KeyTy(className);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (className == std::get<0>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey));
  }

  static ClassTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto className = std::move(std::get<0>(tblgenKey));
    className = allocator.copyInto(className);
    return new (allocator.allocate<ClassTypeStorage>()) ClassTypeStorage(std::move(className));
  }

  ::llvm::StringRef className;
};

} // namespace detail
ClassType ClassType::get(::mlir::MLIRContext *context, ::llvm::StringRef className) {
  return Base::get(context, std::move(className));
}

::mlir::Type ClassType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<std::string> _result_className;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};

  // Parse variable 'className'
  _result_className = ::mlir::FieldParser<std::string>::parse(odsParser);
  if (::mlir::failed(_result_className)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_ClassType parameter 'className' which is to be a `::llvm::StringRef`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_className));
  return ClassType::get(odsParser.getContext(),
      ::llvm::StringRef((*_result_className)));
}

void ClassType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter.printString(getClassName());;
  odsPrinter << ">";
}

::llvm::StringRef ClassType::getClassName() const {
  return getImpl()->className;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ClassType)
namespace moksha::IR {

namespace detail {

struct RefClassTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<::llvm::StringRef>;
  RefClassTypeStorage(::llvm::StringRef className) : className(std::move(className)) {}

  KeyTy getAsKey() const {
    return KeyTy(className);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (className == std::get<0>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey));
  }

  static RefClassTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto className = std::move(std::get<0>(tblgenKey));
    className = allocator.copyInto(className);
    return new (allocator.allocate<RefClassTypeStorage>()) RefClassTypeStorage(std::move(className));
  }

  ::llvm::StringRef className;
};

} // namespace detail
RefClassType RefClassType::get(::mlir::MLIRContext *context, ::llvm::StringRef className) {
  return Base::get(context, std::move(className));
}

::mlir::Type RefClassType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<std::string> _result_className;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};

  // Parse variable 'className'
  _result_className = ::mlir::FieldParser<std::string>::parse(odsParser);
  if (::mlir::failed(_result_className)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_RefClassType parameter 'className' which is to be a `::llvm::StringRef`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_className));
  return RefClassType::get(odsParser.getContext(),
      ::llvm::StringRef((*_result_className)));
}

void RefClassType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter.printString(getClassName());;
  odsPrinter << ">";
}

::llvm::StringRef RefClassType::getClassName() const {
  return getImpl()->className;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::RefClassType)
namespace moksha::IR {

namespace detail {

struct NullableTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<::mlir::Type>;
  NullableTypeStorage(::mlir::Type innerType) : innerType(std::move(innerType)) {}

  KeyTy getAsKey() const {
    return KeyTy(innerType);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (innerType == std::get<0>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey));
  }

  static NullableTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto innerType = std::move(std::get<0>(tblgenKey));
    return new (allocator.allocate<NullableTypeStorage>()) NullableTypeStorage(std::move(innerType));
  }

  ::mlir::Type innerType;
};

} // namespace detail
NullableType NullableType::get(::mlir::MLIRContext *context, ::mlir::Type innerType) {
  return Base::get(context, std::move(innerType));
}

::mlir::Type NullableType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<::mlir::Type> _result_innerType;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};

  // Parse variable 'innerType'
  _result_innerType = ::mlir::FieldParser<::mlir::Type>::parse(odsParser);
  if (::mlir::failed(_result_innerType)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_NullableType parameter 'innerType' which is to be a `::mlir::Type`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_innerType));
  return NullableType::get(odsParser.getContext(),
      ::mlir::Type((*_result_innerType)));
}

void NullableType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter.printStrippedAttrOrType(getInnerType());
  odsPrinter << ">";
}

::mlir::Type NullableType::getInnerType() const {
  return getImpl()->innerType;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::NullableType)
namespace moksha::IR {


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::NullType)
namespace moksha::IR {

namespace detail {

struct MapTypeStorage : public ::mlir::TypeStorage {
  using KeyTy = std::tuple<::mlir::Type, ::mlir::Type>;
  MapTypeStorage(::mlir::Type keyType, ::mlir::Type valueType) : keyType(std::move(keyType)), valueType(std::move(valueType)) {}

  KeyTy getAsKey() const {
    return KeyTy(keyType, valueType);
  }

  bool operator==(const KeyTy &tblgenKey) const {
    return (keyType == std::get<0>(tblgenKey)) && (valueType == std::get<1>(tblgenKey));
  }

  static ::llvm::hash_code hashKey(const KeyTy &tblgenKey) {
    return ::llvm::hash_combine(std::get<0>(tblgenKey), std::get<1>(tblgenKey));
  }

  static MapTypeStorage *construct(::mlir::TypeStorageAllocator &allocator, KeyTy &&tblgenKey) {
    auto keyType = std::move(std::get<0>(tblgenKey));
    auto valueType = std::move(std::get<1>(tblgenKey));
    return new (allocator.allocate<MapTypeStorage>()) MapTypeStorage(std::move(keyType), std::move(valueType));
  }

  ::mlir::Type keyType;
  ::mlir::Type valueType;
};

} // namespace detail
MapType MapType::get(::mlir::MLIRContext *context, ::mlir::Type keyType, ::mlir::Type valueType) {
  return Base::get(context, std::move(keyType), std::move(valueType));
}

::mlir::Type MapType::parse(::mlir::AsmParser &odsParser) {
  ::mlir::Builder odsBuilder(odsParser.getContext());
  ::llvm::SMLoc odsLoc = odsParser.getCurrentLocation();
  (void) odsLoc;
  ::mlir::FailureOr<::mlir::Type> _result_keyType;
  ::mlir::FailureOr<::mlir::Type> _result_valueType;
  // Parse literal '<'
  if (odsParser.parseLess()) return {};

  // Parse variable 'keyType'
  _result_keyType = ::mlir::FieldParser<::mlir::Type>::parse(odsParser);
  if (::mlir::failed(_result_keyType)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_MapType parameter 'keyType' which is to be a `::mlir::Type`");
    return {};
  }
  // Parse literal ','
  if (odsParser.parseComma()) return {};

  // Parse variable 'valueType'
  _result_valueType = ::mlir::FieldParser<::mlir::Type>::parse(odsParser);
  if (::mlir::failed(_result_valueType)) {
    odsParser.emitError(odsParser.getCurrentLocation(), "failed to parse Moksha_MapType parameter 'valueType' which is to be a `::mlir::Type`");
    return {};
  }
  // Parse literal '>'
  if (odsParser.parseGreater()) return {};
  assert(::mlir::succeeded(_result_keyType));
  assert(::mlir::succeeded(_result_valueType));
  return MapType::get(odsParser.getContext(),
      ::mlir::Type((*_result_keyType)),
      ::mlir::Type((*_result_valueType)));
}

void MapType::print(::mlir::AsmPrinter &odsPrinter) const {
  ::mlir::Builder odsBuilder(getContext());
  odsPrinter << "<";
  odsPrinter.printStrippedAttrOrType(getKeyType());
  odsPrinter << ",";
  odsPrinter << ' ';
  odsPrinter.printStrippedAttrOrType(getValueType());
  odsPrinter << ">";
}

::mlir::Type MapType::getKeyType() const {
  return getImpl()->keyType;
}

::mlir::Type MapType::getValueType() const {
  return getImpl()->valueType;
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::MapType)
namespace moksha::IR {


/// Parse a type registered to this dialect.
::mlir::Type MokshaDialect::parseType(::mlir::DialectAsmParser &parser) const {
  ::llvm::SMLoc typeLoc = parser.getCurrentLocation();
  ::llvm::StringRef mnemonic;
  ::mlir::Type genType;
  auto parseResult = generatedTypeParser(parser, &mnemonic, genType);
  if (parseResult.has_value())
    return genType;
  
  parser.emitError(typeLoc) << "unknown  type `"
      << mnemonic << "` in dialect `" << getNamespace() << "`";
  return {};
}
/// Print a type registered to this dialect.
void MokshaDialect::printType(::mlir::Type type,
                    ::mlir::DialectAsmPrinter &printer) const {
  if (::mlir::succeeded(generatedTypePrinter(type, printer)))
    return;
  
}

} // namespace moksha::IR

#endif // GET_TYPEDEF_CLASSES

