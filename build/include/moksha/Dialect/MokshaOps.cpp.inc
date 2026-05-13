/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Op Definitions                                                             *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: MokshaOps.td                                                         *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

#ifdef GET_OP_LIST
#undef GET_OP_LIST

moksha::IR::InvokeIndirectOp,
moksha::IR::AddOp,
moksha::IR::AddressOfOp,
moksha::IR::AllocaOp,
moksha::IR::AndOp,
moksha::IR::AnyCastOp,
moksha::IR::AtomicCmpXchgOp,
moksha::IR::AtomicLoadOp,
moksha::IR::AtomicRMWOp,
moksha::IR::AtomicStoreOp,
moksha::IR::AwaitOp,
moksha::IR::BitcastOp,
moksha::IR::CallOp,
moksha::IR::CastOp,
moksha::IR::CmpOp,
moksha::IR::ConstantOp,
moksha::IR::DivOp,
moksha::IR::ExtractValueOp,
moksha::IR::FenceOp,
moksha::IR::GetElementPtrOp,
moksha::IR::GlobalOp,
moksha::IR::InlineAsmOp,
moksha::IR::InsertValueOp,
moksha::IR::InvokeOp,
moksha::IR::LandingPadOp,
moksha::IR::LoadOp,
moksha::IR::LoadWeakOp,
moksha::IR::MakeClosureOp,
moksha::IR::MakeSharedOp,
moksha::IR::ModOp,
moksha::IR::MulOp,
moksha::IR::OrOp,
moksha::IR::PowOp,
moksha::IR::ReleaseOp,
moksha::IR::ResumeOp,
moksha::IR::RetainOp,
moksha::IR::ShlOp,
moksha::IR::ShrOp,
moksha::IR::SpawnOp,
moksha::IR::StoreOp,
moksha::IR::StoreWeakOp,
moksha::IR::SubOp,
moksha::IR::ThrowOp,
moksha::IR::UnreachableOp,
moksha::IR::UpcastOp,
moksha::IR::XorOp
#endif // GET_OP_LIST

#ifdef GET_OP_CLASSES
#undef GET_OP_CLASSES


//===----------------------------------------------------------------------===//
// Local Utility Method Definitions
//===----------------------------------------------------------------------===//


static ::llvm::LogicalResult __mlir_ods_local_type_constraint_MokshaOps1(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((true))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be any type, but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_MokshaOps2(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((true))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be variadic of any type, but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_MokshaOps3(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((::llvm::isa<moksha::IR::PointerType>(type)))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be A managed or raw pointer type., but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_MokshaOps4(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((::llvm::isa<moksha::IR::PromiseType>(type)))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be A handle to an asynchronously executing task., but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_type_constraint_MokshaOps5(
    ::mlir::Operation *op, ::mlir::Type type, ::llvm::StringRef valueKind,
    unsigned valueIndex) {
  if (!((::llvm::isa<moksha::IR::ClosureType>(type)))) {
    return op->emitOpError(valueKind) << " #" << valueIndex
        << " must be A fat pointer representing a function and its captured environment., but got " << type;
  }
  return ::mlir::success();
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps1(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !((::llvm::isa<::mlir::FlatSymbolRefAttr>(attr))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: flat symbol reference attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps1(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_MokshaOps1(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps2(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !(((::llvm::isa<::mlir::TypeAttr>(attr))) && ((::llvm::isa<::mlir::Type>(::llvm::cast<::mlir::TypeAttr>(attr).getValue()))) && ((true))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: any type attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps2(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_MokshaOps2(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps3(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !(((::llvm::isa<::mlir::IntegerAttr>(attr))) && ((::llvm::cast<::mlir::IntegerAttr>(attr).getType().isSignlessInteger(32)))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: 32-bit signless integer attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps3(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_MokshaOps3(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps4(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !((::llvm::isa<::mlir::SymbolRefAttr>(attr))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: symbol reference attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps4(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_MokshaOps4(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps5(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !((true)))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: any attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps5(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_MokshaOps5(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps6(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !((::llvm::isa<::mlir::StringAttr>(attr))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: string attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps6(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_MokshaOps6(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps7(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !((::llvm::isa<::mlir::UnitAttr>(attr))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: unit attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps7(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_MokshaOps7(attr, attrName, [op]() {
    return op->emitOpError();
  });
}

static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps8(
    ::mlir::Attribute attr, ::llvm::StringRef attrName, llvm::function_ref<::mlir::InFlightDiagnostic()> emitError) {
  if (attr && !((::llvm::isa<::mlir::ArrayAttr>(attr))))
    return emitError() << "attribute '" << attrName
        << "' failed to satisfy constraint: array attribute";
  return ::mlir::success();
}
static ::llvm::LogicalResult __mlir_ods_local_attr_constraint_MokshaOps8(
    ::mlir::Operation *op, ::mlir::Attribute attr, ::llvm::StringRef attrName) {
  return __mlir_ods_local_attr_constraint_MokshaOps8(attr, attrName, [op]() {
    return op->emitOpError();
  });
}
namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::InvokeIndirectOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

std::pair<unsigned, unsigned> InvokeIndirectOpGenericAdaptorBase::getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
  bool isVariadic[] = {false, true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (odsOperandsSize - 1) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}


} // namespace detail
InvokeIndirectOpAdaptor::InvokeIndirectOpAdaptor(InvokeIndirectOp op) : InvokeIndirectOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult InvokeIndirectOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

std::pair<unsigned, unsigned> InvokeIndirectOp::getODSOperandIndexAndLength(unsigned index) {
  bool isVariadic[] = {false, true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumOperands() - 1) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::MutableOperandRange InvokeIndirectOp::getCallArgsMutable() {
  auto range = getODSOperandIndexAndLength(1);
  auto mutableRange = ::mlir::MutableOperandRange(getOperation(), range.first, range.second);
  return mutableRange;
}

std::pair<unsigned, unsigned> InvokeIndirectOp::getODSResultIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumResults() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

void InvokeIndirectOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultType0, ::mlir::Value callee, ::mlir::ValueRange callArgs, ::mlir::Block *normalDest, ::mlir::Block *unwindDest) {
  odsState.addOperands(callee);
  odsState.addOperands(callArgs);
  odsState.addSuccessors(normalDest);
  odsState.addSuccessors(unwindDest);
  odsState.addTypes(resultType0);
}

InvokeIndirectOp InvokeIndirectOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultType0, ::mlir::Value callee, ::mlir::ValueRange callArgs, ::mlir::Block *normalDest, ::mlir::Block *unwindDest) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultType0)>(resultType0), std::forward<decltype(callee)>(callee), std::forward<decltype(callArgs)>(callArgs), std::forward<decltype(normalDest)>(normalDest), std::forward<decltype(unwindDest)>(unwindDest));
  auto __res__ = ::llvm::dyn_cast<InvokeIndirectOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InvokeIndirectOp InvokeIndirectOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultType0, ::mlir::Value callee, ::mlir::ValueRange callArgs, ::mlir::Block *normalDest, ::mlir::Block *unwindDest) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultType0)>(resultType0), std::forward<decltype(callee)>(callee), std::forward<decltype(callArgs)>(callArgs), std::forward<decltype(normalDest)>(normalDest), std::forward<decltype(unwindDest)>(unwindDest));
}

void InvokeIndirectOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() >= 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  odsState.addTypes(resultTypes);
}

InvokeIndirectOp InvokeIndirectOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<InvokeIndirectOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InvokeIndirectOp InvokeIndirectOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult InvokeIndirectOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
  }
  return ::mlir::success();
}

::llvm::LogicalResult InvokeIndirectOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult InvokeIndirectOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand calleeRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> calleeOperands(&calleeRawOperand, 1);  ::llvm::SMLoc calleeOperandsLoc;
  (void)calleeOperandsLoc;
  ::llvm::SmallVector<::mlir::OpAsmParser::UnresolvedOperand, 4> callArgsOperands;
  ::llvm::SMLoc callArgsOperandsLoc;
  (void)callArgsOperandsLoc;
  ::mlir::Block *normalDestSuccessor = nullptr;
  ::mlir::Block *unwindDestSuccessor = nullptr;
  ::mlir::Type calleeRawType{};
  ::llvm::ArrayRef<::mlir::Type> calleeTypes(&calleeRawType, 1);
  ::llvm::SmallVector<::mlir::Type, 1> callArgsTypes;
  ::llvm::SmallVector<::mlir::Type, 1> allResultTypes;

  calleeOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(calleeRawOperand))
    return ::mlir::failure();
  if (parser.parseLParen())
    return ::mlir::failure();

  callArgsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperandList(callArgsOperands))
    return ::mlir::failure();
  if (parser.parseRParen())
    return ::mlir::failure();
  if (parser.parseKeyword("to"))
    return ::mlir::failure();

  if (parser.parseSuccessor(normalDestSuccessor))
    return ::mlir::failure();
  if (parser.parseKeyword("unwind"))
    return ::mlir::failure();

  if (parser.parseSuccessor(unwindDestSuccessor))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    calleeRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  if (parser.parseTypeList(callArgsTypes))
    return ::mlir::failure();
  if (parser.parseArrow())
    return ::mlir::failure();

  if (parser.parseTypeList(allResultTypes))
    return ::mlir::failure();
  result.addSuccessors(normalDestSuccessor);
  result.addSuccessors(unwindDestSuccessor);
  result.addTypes(allResultTypes);
  if (parser.resolveOperands(calleeOperands, calleeTypes, calleeOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(callArgsOperands, callArgsTypes, callArgsOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void InvokeIndirectOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getCallee();
  _odsPrinter << "(";
  _odsPrinter << getCallArgs();
  _odsPrinter << ")";
  _odsPrinter << ' ' << "to";
  _odsPrinter << ' ';
  _odsPrinter << getNormalDest();
  _odsPrinter << ' ' << "unwind";
  _odsPrinter << ' ';
  _odsPrinter << getUnwindDest();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getCallee().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getCallArgs().getTypes();
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  _odsPrinter << getOperation()->getResultTypes();
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::InvokeIndirectOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AddOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
AddOpAdaptor::AddOpAdaptor(AddOp op) : AddOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AddOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void AddOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

AddOp AddOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<AddOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AddOp AddOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void AddOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AddOp AddOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<AddOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AddOp AddOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void AddOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AddOp AddOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AddOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AddOp AddOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AddOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AddOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult AddOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand lhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> lhsOperands(&lhsRawOperand, 1);  ::llvm::SMLoc lhsOperandsLoc;
  (void)lhsOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand rhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> rhsOperands(&rhsRawOperand, 1);  ::llvm::SMLoc rhsOperandsLoc;
  (void)rhsOperandsLoc;
  ::mlir::Type lhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> lhsTypes(&lhsRawType, 1);
  ::mlir::Type rhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> rhsTypes(&rhsRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  lhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(lhsRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  rhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(rhsRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    lhsRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    rhsRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(lhsOperands, lhsTypes, lhsOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(rhsOperands, rhsTypes, rhsOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void AddOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getLhs();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getRhs();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getLhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getRhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void AddOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AddOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AddressOfOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::FlatSymbolRefAttr AddressOfOpGenericAdaptorBase::getGlobalNameAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::FlatSymbolRefAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, AddressOfOp::getGlobalNameAttrName(*odsOpName)));
  return attr;
}

::llvm::StringRef AddressOfOpGenericAdaptorBase::getGlobalName() {
  auto attr = getGlobalNameAttr();
  return attr.getValue();
}


} // namespace detail
AddressOfOpAdaptor::AddressOfOpAdaptor(AddressOfOp op) : AddressOfOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AddressOfOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_global_name;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.address_of' op requires attribute 'global_name'");
    if (namedAttrIt->getName() == AddressOfOp::getGlobalNameAttrName(*odsOpName)) {
      tblgen_global_name = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_global_name && !((::llvm::isa<::mlir::FlatSymbolRefAttr>(tblgen_global_name))))
    return emitError(loc, "'moksha.address_of' op attribute 'global_name' failed to satisfy constraint: flat symbol reference attribute");
  return ::mlir::success();
}

::llvm::StringRef AddressOfOp::getGlobalName() {
  auto attr = getGlobalNameAttr();
  return attr.getValue();
}

void AddressOfOp::setGlobalName(::llvm::StringRef attrValue) {
  (*this)->setAttr(getGlobalNameAttrName(), ::mlir::SymbolRefAttr::get(::mlir::Builder((*this)->getContext()).getContext(), attrValue));
}

void AddressOfOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::FlatSymbolRefAttr global_name) {
  odsState.addAttribute(getGlobalNameAttrName(odsState.name), global_name);
  odsState.addTypes(res);
}

AddressOfOp AddressOfOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::FlatSymbolRefAttr global_name) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(global_name)>(global_name));
  auto __res__ = ::llvm::dyn_cast<AddressOfOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AddressOfOp AddressOfOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::FlatSymbolRefAttr global_name) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(global_name)>(global_name));
}

void AddressOfOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::FlatSymbolRefAttr global_name) {
  odsState.addAttribute(getGlobalNameAttrName(odsState.name), global_name);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AddressOfOp AddressOfOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::FlatSymbolRefAttr global_name) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(global_name)>(global_name));
  auto __res__ = ::llvm::dyn_cast<AddressOfOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AddressOfOp AddressOfOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::FlatSymbolRefAttr global_name) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(global_name)>(global_name));
}

void AddressOfOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::llvm::StringRef global_name) {
  odsState.addAttribute(getGlobalNameAttrName(odsState.name), ::mlir::SymbolRefAttr::get(odsBuilder.getContext(), global_name));
  odsState.addTypes(res);
}

AddressOfOp AddressOfOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::llvm::StringRef global_name) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(global_name)>(global_name));
  auto __res__ = ::llvm::dyn_cast<AddressOfOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AddressOfOp AddressOfOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::llvm::StringRef global_name) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(global_name)>(global_name));
}

void AddressOfOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::llvm::StringRef global_name) {
  odsState.addAttribute(getGlobalNameAttrName(odsState.name), ::mlir::SymbolRefAttr::get(odsBuilder.getContext(), global_name));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AddressOfOp AddressOfOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::llvm::StringRef global_name) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(global_name)>(global_name));
  auto __res__ = ::llvm::dyn_cast<AddressOfOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AddressOfOp AddressOfOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::llvm::StringRef global_name) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(global_name)>(global_name));
}

void AddressOfOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 0u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AddressOfOp AddressOfOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AddressOfOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AddressOfOp AddressOfOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AddressOfOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_global_name;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'global_name'");
    if (namedAttrIt->getName() == getGlobalNameAttrName()) {
      tblgen_global_name = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps1(*this, tblgen_global_name, "global_name")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AddressOfOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult AddressOfOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::FlatSymbolRefAttr global_nameAttr;
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  if (parser.parseCustomAttributeWithFallback(global_nameAttr, parser.getBuilder().getType<::mlir::NoneType>())) {
    return ::mlir::failure();
  }
  if (global_nameAttr) result.attributes.append("global_name", global_nameAttr);
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  return ::mlir::success();
}

void AddressOfOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getGlobalNameAttr());
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("global_name");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void AddressOfOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AddressOfOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AllocaOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::TypeAttr AllocaOpGenericAdaptorBase::getAllocatedTypeAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::TypeAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, AllocaOp::getAllocatedTypeAttrName(*odsOpName)));
  return attr;
}

::mlir::Type AllocaOpGenericAdaptorBase::getAllocatedType() {
  auto attr = getAllocatedTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}


} // namespace detail
AllocaOpAdaptor::AllocaOpAdaptor(AllocaOp op) : AllocaOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AllocaOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_allocatedType;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.alloca' op requires attribute 'allocatedType'");
    if (namedAttrIt->getName() == AllocaOp::getAllocatedTypeAttrName(*odsOpName)) {
      tblgen_allocatedType = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_allocatedType && !(((::llvm::isa<::mlir::TypeAttr>(tblgen_allocatedType))) && ((::llvm::isa<::mlir::Type>(::llvm::cast<::mlir::TypeAttr>(tblgen_allocatedType).getValue()))) && ((true))))
    return emitError(loc, "'moksha.alloca' op attribute 'allocatedType' failed to satisfy constraint: any type attribute");
  return ::mlir::success();
}

::mlir::Type AllocaOp::getAllocatedType() {
  auto attr = getAllocatedTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}

void AllocaOp::setAllocatedType(::mlir::Type attrValue) {
  (*this)->setAttr(getAllocatedTypeAttrName(), ::mlir::TypeAttr::get(attrValue));
}

void AllocaOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::TypeAttr allocatedType) {
  odsState.addAttribute(getAllocatedTypeAttrName(odsState.name), allocatedType);
  odsState.addTypes(res);
}

AllocaOp AllocaOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::TypeAttr allocatedType) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(allocatedType)>(allocatedType));
  auto __res__ = ::llvm::dyn_cast<AllocaOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AllocaOp AllocaOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::TypeAttr allocatedType) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(allocatedType)>(allocatedType));
}

void AllocaOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr allocatedType) {
  odsState.addAttribute(getAllocatedTypeAttrName(odsState.name), allocatedType);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AllocaOp AllocaOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr allocatedType) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(allocatedType)>(allocatedType));
  auto __res__ = ::llvm::dyn_cast<AllocaOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AllocaOp AllocaOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::TypeAttr allocatedType) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(allocatedType)>(allocatedType));
}

void AllocaOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Type allocatedType) {
  odsState.addAttribute(getAllocatedTypeAttrName(odsState.name), ::mlir::TypeAttr::get(allocatedType));
  odsState.addTypes(res);
}

AllocaOp AllocaOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Type allocatedType) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(allocatedType)>(allocatedType));
  auto __res__ = ::llvm::dyn_cast<AllocaOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AllocaOp AllocaOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Type allocatedType) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(allocatedType)>(allocatedType));
}

void AllocaOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Type allocatedType) {
  odsState.addAttribute(getAllocatedTypeAttrName(odsState.name), ::mlir::TypeAttr::get(allocatedType));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AllocaOp AllocaOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Type allocatedType) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(allocatedType)>(allocatedType));
  auto __res__ = ::llvm::dyn_cast<AllocaOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AllocaOp AllocaOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Type allocatedType) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(allocatedType)>(allocatedType));
}

void AllocaOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 0u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AllocaOp AllocaOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AllocaOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AllocaOp AllocaOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AllocaOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_allocatedType;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'allocatedType'");
    if (namedAttrIt->getName() == getAllocatedTypeAttrName()) {
      tblgen_allocatedType = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps2(*this, tblgen_allocatedType, "allocatedType")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AllocaOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult AllocaOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::TypeAttr allocatedTypeAttr;
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  if (parser.parseCustomAttributeWithFallback(allocatedTypeAttr, parser.getBuilder().getType<::mlir::NoneType>())) {
    return ::mlir::failure();
  }
  if (allocatedTypeAttr) result.attributes.append("allocatedType", allocatedTypeAttr);
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  return ::mlir::success();
}

void AllocaOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getAllocatedTypeAttr());
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("allocatedType");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AllocaOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AndOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
AndOpAdaptor::AndOpAdaptor(AndOp op) : AndOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AndOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void AndOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

AndOp AndOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<AndOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AndOp AndOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void AndOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AndOp AndOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<AndOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AndOp AndOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void AndOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AndOp AndOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AndOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AndOp AndOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AndOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AndOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

void AndOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AndOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AnyCastOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
AnyCastOpAdaptor::AnyCastOpAdaptor(AnyCastOp op) : AnyCastOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AnyCastOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void AnyCastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value operand) {
  odsState.addOperands(operand);
  odsState.addTypes(res);
}

AnyCastOp AnyCastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value operand) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(operand)>(operand));
  auto __res__ = ::llvm::dyn_cast<AnyCastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AnyCastOp AnyCastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value operand) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(operand)>(operand));
}

void AnyCastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value operand) {
  odsState.addOperands(operand);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AnyCastOp AnyCastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value operand) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operand)>(operand));
  auto __res__ = ::llvm::dyn_cast<AnyCastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AnyCastOp AnyCastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value operand) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operand)>(operand));
}

void AnyCastOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AnyCastOp AnyCastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AnyCastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AnyCastOp AnyCastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AnyCastOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AnyCastOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult AnyCastOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand operandRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> operandOperands(&operandRawOperand, 1);  ::llvm::SMLoc operandOperandsLoc;
  (void)operandOperandsLoc;
  ::mlir::Type operandRawType{};
  ::llvm::ArrayRef<::mlir::Type> operandTypes(&operandRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  operandOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(operandRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    operandRawType = type;
  }
  if (parser.parseKeyword("to"))
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(operandOperands, operandTypes, operandOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void AnyCastOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getOperand();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getOperand().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "to";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void AnyCastOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AnyCastOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AtomicCmpXchgOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr AtomicCmpXchgOpGenericAdaptorBase::getSuccessOrderAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 1, odsAttrs.end() - 0, AtomicCmpXchgOp::getSuccessOrderAttrName(*odsOpName)));
  return attr;
}

uint32_t AtomicCmpXchgOpGenericAdaptorBase::getSuccessOrder() {
  auto attr = getSuccessOrderAttr();
  return attr.getValue().getZExtValue();
}

::mlir::IntegerAttr AtomicCmpXchgOpGenericAdaptorBase::getFailureOrderAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 1, AtomicCmpXchgOp::getFailureOrderAttrName(*odsOpName)));
  return attr;
}

uint32_t AtomicCmpXchgOpGenericAdaptorBase::getFailureOrder() {
  auto attr = getFailureOrderAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
AtomicCmpXchgOpAdaptor::AtomicCmpXchgOpAdaptor(AtomicCmpXchgOp op) : AtomicCmpXchgOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AtomicCmpXchgOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_failureOrder;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.atomic_cmpxchg' op requires attribute 'failureOrder'");
    if (namedAttrIt->getName() == AtomicCmpXchgOp::getFailureOrderAttrName(*odsOpName)) {
      tblgen_failureOrder = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_successOrder;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.atomic_cmpxchg' op requires attribute 'successOrder'");
    if (namedAttrIt->getName() == AtomicCmpXchgOp::getSuccessOrderAttrName(*odsOpName)) {
      tblgen_successOrder = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_successOrder && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_successOrder))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_successOrder).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.atomic_cmpxchg' op attribute 'successOrder' failed to satisfy constraint: 32-bit signless integer attribute");

  if (tblgen_failureOrder && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_failureOrder))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_failureOrder).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.atomic_cmpxchg' op attribute 'failureOrder' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t AtomicCmpXchgOp::getSuccessOrder() {
  auto attr = getSuccessOrderAttr();
  return attr.getValue().getZExtValue();
}

uint32_t AtomicCmpXchgOp::getFailureOrder() {
  auto attr = getFailureOrderAttr();
  return attr.getValue().getZExtValue();
}

void AtomicCmpXchgOp::setSuccessOrder(uint32_t attrValue) {
  (*this)->setAttr(getSuccessOrderAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void AtomicCmpXchgOp::setFailureOrder(uint32_t attrValue) {
  (*this)->setAttr(getFailureOrderAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void AtomicCmpXchgOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type old_value, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, ::mlir::IntegerAttr successOrder, ::mlir::IntegerAttr failureOrder) {
  odsState.addOperands(ptr);
  odsState.addOperands(expected);
  odsState.addOperands(desired);
  odsState.addAttribute(getSuccessOrderAttrName(odsState.name), successOrder);
  odsState.addAttribute(getFailureOrderAttrName(odsState.name), failureOrder);
  odsState.addTypes(old_value);
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type old_value, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, ::mlir::IntegerAttr successOrder, ::mlir::IntegerAttr failureOrder) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(old_value)>(old_value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(expected)>(expected), std::forward<decltype(desired)>(desired), std::forward<decltype(successOrder)>(successOrder), std::forward<decltype(failureOrder)>(failureOrder));
  auto __res__ = ::llvm::dyn_cast<AtomicCmpXchgOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type old_value, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, ::mlir::IntegerAttr successOrder, ::mlir::IntegerAttr failureOrder) {
  return create(builder, builder.getLoc(), std::forward<decltype(old_value)>(old_value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(expected)>(expected), std::forward<decltype(desired)>(desired), std::forward<decltype(successOrder)>(successOrder), std::forward<decltype(failureOrder)>(failureOrder));
}

void AtomicCmpXchgOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, ::mlir::IntegerAttr successOrder, ::mlir::IntegerAttr failureOrder) {
  odsState.addOperands(ptr);
  odsState.addOperands(expected);
  odsState.addOperands(desired);
  odsState.addAttribute(getSuccessOrderAttrName(odsState.name), successOrder);
  odsState.addAttribute(getFailureOrderAttrName(odsState.name), failureOrder);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, ::mlir::IntegerAttr successOrder, ::mlir::IntegerAttr failureOrder) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr), std::forward<decltype(expected)>(expected), std::forward<decltype(desired)>(desired), std::forward<decltype(successOrder)>(successOrder), std::forward<decltype(failureOrder)>(failureOrder));
  auto __res__ = ::llvm::dyn_cast<AtomicCmpXchgOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, ::mlir::IntegerAttr successOrder, ::mlir::IntegerAttr failureOrder) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr), std::forward<decltype(expected)>(expected), std::forward<decltype(desired)>(desired), std::forward<decltype(successOrder)>(successOrder), std::forward<decltype(failureOrder)>(failureOrder));
}

void AtomicCmpXchgOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type old_value, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, uint32_t successOrder, uint32_t failureOrder) {
  odsState.addOperands(ptr);
  odsState.addOperands(expected);
  odsState.addOperands(desired);
  odsState.addAttribute(getSuccessOrderAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), successOrder));
  odsState.addAttribute(getFailureOrderAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), failureOrder));
  odsState.addTypes(old_value);
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type old_value, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, uint32_t successOrder, uint32_t failureOrder) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(old_value)>(old_value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(expected)>(expected), std::forward<decltype(desired)>(desired), std::forward<decltype(successOrder)>(successOrder), std::forward<decltype(failureOrder)>(failureOrder));
  auto __res__ = ::llvm::dyn_cast<AtomicCmpXchgOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type old_value, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, uint32_t successOrder, uint32_t failureOrder) {
  return create(builder, builder.getLoc(), std::forward<decltype(old_value)>(old_value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(expected)>(expected), std::forward<decltype(desired)>(desired), std::forward<decltype(successOrder)>(successOrder), std::forward<decltype(failureOrder)>(failureOrder));
}

void AtomicCmpXchgOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, uint32_t successOrder, uint32_t failureOrder) {
  odsState.addOperands(ptr);
  odsState.addOperands(expected);
  odsState.addOperands(desired);
  odsState.addAttribute(getSuccessOrderAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), successOrder));
  odsState.addAttribute(getFailureOrderAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), failureOrder));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, uint32_t successOrder, uint32_t failureOrder) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr), std::forward<decltype(expected)>(expected), std::forward<decltype(desired)>(desired), std::forward<decltype(successOrder)>(successOrder), std::forward<decltype(failureOrder)>(failureOrder));
  auto __res__ = ::llvm::dyn_cast<AtomicCmpXchgOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, ::mlir::Value expected, ::mlir::Value desired, uint32_t successOrder, uint32_t failureOrder) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr), std::forward<decltype(expected)>(expected), std::forward<decltype(desired)>(desired), std::forward<decltype(successOrder)>(successOrder), std::forward<decltype(failureOrder)>(failureOrder));
}

void AtomicCmpXchgOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 3u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AtomicCmpXchgOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicCmpXchgOp AtomicCmpXchgOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AtomicCmpXchgOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_failureOrder;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'failureOrder'");
    if (namedAttrIt->getName() == getFailureOrderAttrName()) {
      tblgen_failureOrder = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_successOrder;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'successOrder'");
    if (namedAttrIt->getName() == getSuccessOrderAttrName()) {
      tblgen_successOrder = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_successOrder, "successOrder")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_failureOrder, "failureOrder")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup2 = getODSOperands(2);

    for (auto v : valueGroup2) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AtomicCmpXchgOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult AtomicCmpXchgOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand ptrRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> ptrOperands(&ptrRawOperand, 1);  ::llvm::SMLoc ptrOperandsLoc;
  (void)ptrOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand expectedRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> expectedOperands(&expectedRawOperand, 1);  ::llvm::SMLoc expectedOperandsLoc;
  (void)expectedOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand desiredRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> desiredOperands(&desiredRawOperand, 1);  ::llvm::SMLoc desiredOperandsLoc;
  (void)desiredOperandsLoc;
  ::mlir::Type ptrRawType{};
  ::llvm::ArrayRef<::mlir::Type> ptrTypes(&ptrRawType, 1);
  ::mlir::Type expectedRawType{};
  ::llvm::ArrayRef<::mlir::Type> expectedTypes(&expectedRawType, 1);
  ::mlir::Type desiredRawType{};
  ::llvm::ArrayRef<::mlir::Type> desiredTypes(&desiredRawType, 1);
  ::mlir::Type old_valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> old_valueTypes(&old_valueRawType, 1);

  ptrOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(ptrRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  expectedOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(expectedRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  desiredOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(desiredRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    ptrRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    expectedRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    desiredRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    old_valueRawType = type;
  }
  result.addTypes(old_valueTypes);
  if (parser.resolveOperands(ptrOperands, ptrTypes, ptrOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(expectedOperands, expectedTypes, expectedOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(desiredOperands, desiredTypes, desiredOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void AtomicCmpXchgOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getPtr();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getExpected();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getDesired();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getPtr().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getExpected().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getDesired().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getOldValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void AtomicCmpXchgOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
  effects.emplace_back(::mlir::MemoryEffects::Write::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
  effects.emplace_back(::mlir::MemoryEffects::Read::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AtomicCmpXchgOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AtomicLoadOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr AtomicLoadOpGenericAdaptorBase::getOrderingAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, AtomicLoadOp::getOrderingAttrName(*odsOpName)));
  return attr;
}

uint32_t AtomicLoadOpGenericAdaptorBase::getOrdering() {
  auto attr = getOrderingAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
AtomicLoadOpAdaptor::AtomicLoadOpAdaptor(AtomicLoadOp op) : AtomicLoadOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AtomicLoadOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_ordering;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.atomic_load' op requires attribute 'ordering'");
    if (namedAttrIt->getName() == AtomicLoadOp::getOrderingAttrName(*odsOpName)) {
      tblgen_ordering = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_ordering && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_ordering))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_ordering).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.atomic_load' op attribute 'ordering' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t AtomicLoadOp::getOrdering() {
  auto attr = getOrderingAttr();
  return attr.getValue().getZExtValue();
}

void AtomicLoadOp::setOrdering(uint32_t attrValue) {
  (*this)->setAttr(getOrderingAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void AtomicLoadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  odsState.addOperands(ptr);
  odsState.addAttribute(getOrderingAttrName(odsState.name), ordering);
  odsState.addTypes(res);
}

AtomicLoadOp AtomicLoadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicLoadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicLoadOp AtomicLoadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
}

void AtomicLoadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  odsState.addOperands(ptr);
  odsState.addAttribute(getOrderingAttrName(odsState.name), ordering);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AtomicLoadOp AtomicLoadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicLoadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicLoadOp AtomicLoadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
}

void AtomicLoadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value ptr, uint32_t ordering) {
  odsState.addOperands(ptr);
  odsState.addAttribute(getOrderingAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), ordering));
  odsState.addTypes(res);
}

AtomicLoadOp AtomicLoadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value ptr, uint32_t ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicLoadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicLoadOp AtomicLoadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value ptr, uint32_t ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
}

void AtomicLoadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, uint32_t ordering) {
  odsState.addOperands(ptr);
  odsState.addAttribute(getOrderingAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), ordering));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AtomicLoadOp AtomicLoadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, uint32_t ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicLoadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicLoadOp AtomicLoadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value ptr, uint32_t ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
}

void AtomicLoadOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AtomicLoadOp AtomicLoadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AtomicLoadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicLoadOp AtomicLoadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AtomicLoadOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_ordering;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'ordering'");
    if (namedAttrIt->getName() == getOrderingAttrName()) {
      tblgen_ordering = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_ordering, "ordering")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AtomicLoadOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult AtomicLoadOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand ptrRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> ptrOperands(&ptrRawOperand, 1);  ::llvm::SMLoc ptrOperandsLoc;
  (void)ptrOperandsLoc;
  ::mlir::Type ptrRawType{};
  ::llvm::ArrayRef<::mlir::Type> ptrTypes(&ptrRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  ptrOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(ptrRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    ptrRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(ptrOperands, ptrTypes, ptrOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void AtomicLoadOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getPtr();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getPtr().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void AtomicLoadOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
  effects.emplace_back(::mlir::MemoryEffects::Read::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AtomicLoadOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AtomicRMWOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr AtomicRMWOpGenericAdaptorBase::getBinOpAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 1, AtomicRMWOp::getBinOpAttrName(*odsOpName)));
  return attr;
}

uint32_t AtomicRMWOpGenericAdaptorBase::getBinOp() {
  auto attr = getBinOpAttr();
  return attr.getValue().getZExtValue();
}

::mlir::IntegerAttr AtomicRMWOpGenericAdaptorBase::getOrderingAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 1, odsAttrs.end() - 0, AtomicRMWOp::getOrderingAttrName(*odsOpName)));
  return attr;
}

uint32_t AtomicRMWOpGenericAdaptorBase::getOrdering() {
  auto attr = getOrderingAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
AtomicRMWOpAdaptor::AtomicRMWOpAdaptor(AtomicRMWOp op) : AtomicRMWOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AtomicRMWOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_binOp;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.atomic_rmw' op requires attribute 'binOp'");
    if (namedAttrIt->getName() == AtomicRMWOp::getBinOpAttrName(*odsOpName)) {
      tblgen_binOp = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_ordering;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.atomic_rmw' op requires attribute 'ordering'");
    if (namedAttrIt->getName() == AtomicRMWOp::getOrderingAttrName(*odsOpName)) {
      tblgen_ordering = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_binOp && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_binOp))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_binOp).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.atomic_rmw' op attribute 'binOp' failed to satisfy constraint: 32-bit signless integer attribute");

  if (tblgen_ordering && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_ordering))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_ordering).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.atomic_rmw' op attribute 'ordering' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t AtomicRMWOp::getBinOp() {
  auto attr = getBinOpAttr();
  return attr.getValue().getZExtValue();
}

uint32_t AtomicRMWOp::getOrdering() {
  auto attr = getOrderingAttr();
  return attr.getValue().getZExtValue();
}

void AtomicRMWOp::setBinOp(uint32_t attrValue) {
  (*this)->setAttr(getBinOpAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void AtomicRMWOp::setOrdering(uint32_t attrValue) {
  (*this)->setAttr(getOrderingAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void AtomicRMWOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::IntegerAttr binOp, ::mlir::Value ptr, ::mlir::Value value, ::mlir::IntegerAttr ordering) {
  odsState.addOperands(ptr);
  odsState.addOperands(value);
  odsState.addAttribute(getBinOpAttrName(odsState.name), binOp);
  odsState.addAttribute(getOrderingAttrName(odsState.name), ordering);
  odsState.addTypes(res);
}

AtomicRMWOp AtomicRMWOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::IntegerAttr binOp, ::mlir::Value ptr, ::mlir::Value value, ::mlir::IntegerAttr ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(binOp)>(binOp), std::forward<decltype(ptr)>(ptr), std::forward<decltype(value)>(value), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicRMWOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicRMWOp AtomicRMWOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::IntegerAttr binOp, ::mlir::Value ptr, ::mlir::Value value, ::mlir::IntegerAttr ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(binOp)>(binOp), std::forward<decltype(ptr)>(ptr), std::forward<decltype(value)>(value), std::forward<decltype(ordering)>(ordering));
}

void AtomicRMWOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::IntegerAttr binOp, ::mlir::Value ptr, ::mlir::Value value, ::mlir::IntegerAttr ordering) {
  odsState.addOperands(ptr);
  odsState.addOperands(value);
  odsState.addAttribute(getBinOpAttrName(odsState.name), binOp);
  odsState.addAttribute(getOrderingAttrName(odsState.name), ordering);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AtomicRMWOp AtomicRMWOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::IntegerAttr binOp, ::mlir::Value ptr, ::mlir::Value value, ::mlir::IntegerAttr ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(binOp)>(binOp), std::forward<decltype(ptr)>(ptr), std::forward<decltype(value)>(value), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicRMWOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicRMWOp AtomicRMWOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::IntegerAttr binOp, ::mlir::Value ptr, ::mlir::Value value, ::mlir::IntegerAttr ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(binOp)>(binOp), std::forward<decltype(ptr)>(ptr), std::forward<decltype(value)>(value), std::forward<decltype(ordering)>(ordering));
}

void AtomicRMWOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, uint32_t binOp, ::mlir::Value ptr, ::mlir::Value value, uint32_t ordering) {
  odsState.addOperands(ptr);
  odsState.addOperands(value);
  odsState.addAttribute(getBinOpAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), binOp));
  odsState.addAttribute(getOrderingAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), ordering));
  odsState.addTypes(res);
}

AtomicRMWOp AtomicRMWOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, uint32_t binOp, ::mlir::Value ptr, ::mlir::Value value, uint32_t ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(binOp)>(binOp), std::forward<decltype(ptr)>(ptr), std::forward<decltype(value)>(value), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicRMWOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicRMWOp AtomicRMWOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, uint32_t binOp, ::mlir::Value ptr, ::mlir::Value value, uint32_t ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(binOp)>(binOp), std::forward<decltype(ptr)>(ptr), std::forward<decltype(value)>(value), std::forward<decltype(ordering)>(ordering));
}

void AtomicRMWOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, uint32_t binOp, ::mlir::Value ptr, ::mlir::Value value, uint32_t ordering) {
  odsState.addOperands(ptr);
  odsState.addOperands(value);
  odsState.addAttribute(getBinOpAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), binOp));
  odsState.addAttribute(getOrderingAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), ordering));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AtomicRMWOp AtomicRMWOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, uint32_t binOp, ::mlir::Value ptr, ::mlir::Value value, uint32_t ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(binOp)>(binOp), std::forward<decltype(ptr)>(ptr), std::forward<decltype(value)>(value), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicRMWOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicRMWOp AtomicRMWOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, uint32_t binOp, ::mlir::Value ptr, ::mlir::Value value, uint32_t ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(binOp)>(binOp), std::forward<decltype(ptr)>(ptr), std::forward<decltype(value)>(value), std::forward<decltype(ordering)>(ordering));
}

void AtomicRMWOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AtomicRMWOp AtomicRMWOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AtomicRMWOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicRMWOp AtomicRMWOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AtomicRMWOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_binOp;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'binOp'");
    if (namedAttrIt->getName() == getBinOpAttrName()) {
      tblgen_binOp = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_ordering;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'ordering'");
    if (namedAttrIt->getName() == getOrderingAttrName()) {
      tblgen_ordering = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_binOp, "binOp")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_ordering, "ordering")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AtomicRMWOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult AtomicRMWOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::IntegerAttr binOpAttr;
  ::mlir::OpAsmParser::UnresolvedOperand ptrRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> ptrOperands(&ptrRawOperand, 1);  ::llvm::SMLoc ptrOperandsLoc;
  (void)ptrOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::Type ptrRawType{};
  ::llvm::ArrayRef<::mlir::Type> ptrTypes(&ptrRawType, 1);
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  if (parser.parseCustomAttributeWithFallback(binOpAttr, parser.getBuilder().getIntegerType(32))) {
    return ::mlir::failure();
  }
  if (binOpAttr) result.attributes.append("binOp", binOpAttr);
  if (parser.parseComma())
    return ::mlir::failure();

  ptrOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(ptrRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    ptrRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(ptrOperands, ptrTypes, ptrOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void AtomicRMWOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getBinOpAttr());
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getPtr();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("binOp");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getPtr().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void AtomicRMWOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
  effects.emplace_back(::mlir::MemoryEffects::Write::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
  effects.emplace_back(::mlir::MemoryEffects::Read::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AtomicRMWOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AtomicStoreOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr AtomicStoreOpGenericAdaptorBase::getOrderingAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, AtomicStoreOp::getOrderingAttrName(*odsOpName)));
  return attr;
}

uint32_t AtomicStoreOpGenericAdaptorBase::getOrdering() {
  auto attr = getOrderingAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
AtomicStoreOpAdaptor::AtomicStoreOpAdaptor(AtomicStoreOp op) : AtomicStoreOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AtomicStoreOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_ordering;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.atomic_store' op requires attribute 'ordering'");
    if (namedAttrIt->getName() == AtomicStoreOp::getOrderingAttrName(*odsOpName)) {
      tblgen_ordering = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_ordering && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_ordering))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_ordering).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.atomic_store' op attribute 'ordering' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t AtomicStoreOp::getOrdering() {
  auto attr = getOrderingAttr();
  return attr.getValue().getZExtValue();
}

void AtomicStoreOp::setOrdering(uint32_t attrValue) {
  (*this)->setAttr(getOrderingAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void AtomicStoreOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value value, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  odsState.addOperands(value);
  odsState.addOperands(ptr);
  odsState.addAttribute(getOrderingAttrName(odsState.name), ordering);
}

AtomicStoreOp AtomicStoreOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Value value, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicStoreOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicStoreOp AtomicStoreOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Value value, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
}

void AtomicStoreOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  odsState.addOperands(value);
  odsState.addOperands(ptr);
  odsState.addAttribute(getOrderingAttrName(odsState.name), ordering);
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AtomicStoreOp AtomicStoreOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicStoreOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicStoreOp AtomicStoreOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr, ::mlir::IntegerAttr ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
}

void AtomicStoreOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value value, ::mlir::Value ptr, uint32_t ordering) {
  odsState.addOperands(value);
  odsState.addOperands(ptr);
  odsState.addAttribute(getOrderingAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), ordering));
}

AtomicStoreOp AtomicStoreOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Value value, ::mlir::Value ptr, uint32_t ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicStoreOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicStoreOp AtomicStoreOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Value value, ::mlir::Value ptr, uint32_t ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
}

void AtomicStoreOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr, uint32_t ordering) {
  odsState.addOperands(value);
  odsState.addOperands(ptr);
  odsState.addAttribute(getOrderingAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), ordering));
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AtomicStoreOp AtomicStoreOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr, uint32_t ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<AtomicStoreOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicStoreOp AtomicStoreOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr, uint32_t ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr), std::forward<decltype(ordering)>(ordering));
}

void AtomicStoreOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AtomicStoreOp AtomicStoreOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AtomicStoreOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AtomicStoreOp AtomicStoreOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AtomicStoreOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_ordering;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'ordering'");
    if (namedAttrIt->getName() == getOrderingAttrName()) {
      tblgen_ordering = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_ordering, "ordering")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AtomicStoreOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult AtomicStoreOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand ptrRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> ptrOperands(&ptrRawOperand, 1);  ::llvm::SMLoc ptrOperandsLoc;
  (void)ptrOperandsLoc;
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);
  ::mlir::Type ptrRawType{};
  ::llvm::ArrayRef<::mlir::Type> ptrTypes(&ptrRawType, 1);

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  ptrOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(ptrRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    ptrRawType = type;
  }
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(ptrOperands, ptrTypes, ptrOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void AtomicStoreOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getPtr();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getPtr().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void AtomicStoreOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
  effects.emplace_back(::mlir::MemoryEffects::Write::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AtomicStoreOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::AwaitOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
AwaitOpAdaptor::AwaitOpAdaptor(AwaitOp op) : AwaitOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult AwaitOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void AwaitOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type result, ::mlir::Value promise) {
  odsState.addOperands(promise);
  odsState.addTypes(result);
}

AwaitOp AwaitOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type result, ::mlir::Value promise) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(result)>(result), std::forward<decltype(promise)>(promise));
  auto __res__ = ::llvm::dyn_cast<AwaitOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AwaitOp AwaitOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type result, ::mlir::Value promise) {
  return create(builder, builder.getLoc(), std::forward<decltype(result)>(result), std::forward<decltype(promise)>(promise));
}

void AwaitOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value promise) {
  odsState.addOperands(promise);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

AwaitOp AwaitOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value promise) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(promise)>(promise));
  auto __res__ = ::llvm::dyn_cast<AwaitOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AwaitOp AwaitOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value promise) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(promise)>(promise));
}

void AwaitOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

AwaitOp AwaitOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<AwaitOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

AwaitOp AwaitOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult AwaitOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps4(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult AwaitOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult AwaitOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand promiseRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> promiseOperands(&promiseRawOperand, 1);  ::llvm::SMLoc promiseOperandsLoc;
  (void)promiseOperandsLoc;
  ::mlir::Type promiseRawType{};
  ::llvm::ArrayRef<::mlir::Type> promiseTypes(&promiseRawType, 1);
  ::mlir::Type resultRawType{};
  ::llvm::ArrayRef<::mlir::Type> resultTypes(&resultRawType, 1);

  promiseOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(promiseRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::PromiseType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    promiseRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resultRawType = type;
  }
  result.addTypes(resultTypes);
  if (parser.resolveOperands(promiseOperands, promiseTypes, promiseOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void AwaitOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getPromise();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getPromise().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PromiseType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getResult().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::AwaitOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::BitcastOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
BitcastOpAdaptor::BitcastOpAdaptor(BitcastOp op) : BitcastOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult BitcastOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void BitcastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value operand) {
  odsState.addOperands(operand);
  odsState.addTypes(res);
}

BitcastOp BitcastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value operand) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(operand)>(operand));
  auto __res__ = ::llvm::dyn_cast<BitcastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

BitcastOp BitcastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value operand) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(operand)>(operand));
}

void BitcastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value operand) {
  odsState.addOperands(operand);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

BitcastOp BitcastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value operand) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operand)>(operand));
  auto __res__ = ::llvm::dyn_cast<BitcastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

BitcastOp BitcastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value operand) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operand)>(operand));
}

void BitcastOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

BitcastOp BitcastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<BitcastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

BitcastOp BitcastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult BitcastOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult BitcastOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult BitcastOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand operandRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> operandOperands(&operandRawOperand, 1);  ::llvm::SMLoc operandOperandsLoc;
  (void)operandOperandsLoc;
  ::mlir::Type operandRawType{};
  ::llvm::ArrayRef<::mlir::Type> operandTypes(&operandRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  operandOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(operandRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    operandRawType = type;
  }
  if (parser.parseKeyword("to"))
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(operandOperands, operandTypes, operandOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void BitcastOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getOperand();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getOperand().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "to";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void BitcastOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::BitcastOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::CallOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

std::pair<unsigned, unsigned> CallOpGenericAdaptorBase::getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (odsOperandsSize - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::SymbolRefAttr CallOpGenericAdaptorBase::getCalleeAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::SymbolRefAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, CallOp::getCalleeAttrName(*odsOpName)));
  return attr;
}

::mlir::SymbolRefAttr CallOpGenericAdaptorBase::getCallee() {
  auto attr = getCalleeAttr();
  return attr;
}


} // namespace detail
CallOpAdaptor::CallOpAdaptor(CallOp op) : CallOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult CallOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_callee;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.call' op requires attribute 'callee'");
    if (namedAttrIt->getName() == CallOp::getCalleeAttrName(*odsOpName)) {
      tblgen_callee = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_callee && !((::llvm::isa<::mlir::SymbolRefAttr>(tblgen_callee))))
    return emitError(loc, "'moksha.call' op attribute 'callee' failed to satisfy constraint: symbol reference attribute");
  return ::mlir::success();
}

std::pair<unsigned, unsigned> CallOp::getODSOperandIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumOperands() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::MutableOperandRange CallOp::getOperandsMutable() {
  auto range = getODSOperandIndexAndLength(0);
  auto mutableRange = ::mlir::MutableOperandRange(getOperation(), range.first, range.second);
  return mutableRange;
}

std::pair<unsigned, unsigned> CallOp::getODSResultIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumResults() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::SymbolRefAttr CallOp::getCallee() {
  auto attr = getCalleeAttr();
  return attr;
}

void CallOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange results, ::mlir::SymbolRefAttr callee, ::mlir::ValueRange operands) {
  odsState.addOperands(operands);
  odsState.addAttribute(getCalleeAttrName(odsState.name), callee);
  odsState.addTypes(results);
}

CallOp CallOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange results, ::mlir::SymbolRefAttr callee, ::mlir::ValueRange operands) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(results)>(results), std::forward<decltype(callee)>(callee), std::forward<decltype(operands)>(operands));
  auto __res__ = ::llvm::dyn_cast<CallOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CallOp CallOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange results, ::mlir::SymbolRefAttr callee, ::mlir::ValueRange operands) {
  return create(builder, builder.getLoc(), std::forward<decltype(results)>(results), std::forward<decltype(callee)>(callee), std::forward<decltype(operands)>(operands));
}

void CallOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  odsState.addTypes(resultTypes);
}

CallOp CallOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<CallOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CallOp CallOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult CallOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_callee;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'callee'");
    if (namedAttrIt->getName() == getCalleeAttrName()) {
      tblgen_callee = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps4(*this, tblgen_callee, "callee")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult CallOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult CallOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::SymbolRefAttr calleeAttr;
  ::llvm::SmallVector<::mlir::OpAsmParser::UnresolvedOperand, 4> operandsOperands;
  ::llvm::SMLoc operandsOperandsLoc;
  (void)operandsOperandsLoc;
  ::llvm::ArrayRef<::mlir::Type> operandsTypes;
  ::llvm::ArrayRef<::mlir::Type> resultsTypes;

  if (parser.parseCustomAttributeWithFallback(calleeAttr, parser.getBuilder().getType<::mlir::NoneType>())) {
    return ::mlir::failure();
  }
  if (calleeAttr) result.attributes.append("callee", calleeAttr);
  if (parser.parseLParen())
    return ::mlir::failure();

  operandsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperandList(operandsOperands))
    return ::mlir::failure();
  if (parser.parseRParen())
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  ::mlir::FunctionType operands__results_functionType;
  if (parser.parseType(operands__results_functionType))
    return ::mlir::failure();
  operandsTypes = operands__results_functionType.getInputs();
  resultsTypes = operands__results_functionType.getResults();
  result.addTypes(resultsTypes);
  if (parser.resolveOperands(operandsOperands, operandsTypes, operandsOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void CallOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getCalleeAttr());
  _odsPrinter << "(";
  _odsPrinter << getOperands();
  _odsPrinter << ")";
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("callee");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  _odsPrinter.printFunctionalType(getOperands().getTypes(), getResults().getTypes());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::CallOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::CastOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
CastOpAdaptor::CastOpAdaptor(CastOp op) : CastOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult CastOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void CastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value value) {
  odsState.addOperands(value);
  odsState.addTypes(res);
}

CastOp CastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<CastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CastOp CastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value value) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(value)>(value));
}

void CastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  odsState.addOperands(value);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

CastOp CastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<CastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CastOp CastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
}

void CastOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

CastOp CastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<CastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CastOp CastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult CastOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult CastOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult CastOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.parseKeyword("to"))
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void CastOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "to";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void CastOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::CastOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::CmpOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr CmpOpGenericAdaptorBase::getPredicateAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, CmpOp::getPredicateAttrName(*odsOpName)));
  return attr;
}

uint32_t CmpOpGenericAdaptorBase::getPredicate() {
  auto attr = getPredicateAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
CmpOpAdaptor::CmpOpAdaptor(CmpOp op) : CmpOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult CmpOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_predicate;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.cmp' op requires attribute 'predicate'");
    if (namedAttrIt->getName() == CmpOp::getPredicateAttrName(*odsOpName)) {
      tblgen_predicate = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_predicate && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_predicate))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_predicate).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.cmp' op attribute 'predicate' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t CmpOp::getPredicate() {
  auto attr = getPredicateAttr();
  return attr.getValue().getZExtValue();
}

void CmpOp::setPredicate(uint32_t attrValue) {
  (*this)->setAttr(getPredicateAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void CmpOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::IntegerAttr predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addAttribute(getPredicateAttrName(odsState.name), predicate);
  odsState.addTypes(res);
}

CmpOp CmpOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::IntegerAttr predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(predicate)>(predicate), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<CmpOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CmpOp CmpOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::IntegerAttr predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(predicate)>(predicate), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void CmpOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::IntegerAttr predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addAttribute(getPredicateAttrName(odsState.name), predicate);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

CmpOp CmpOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::IntegerAttr predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(predicate)>(predicate), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<CmpOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CmpOp CmpOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::IntegerAttr predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(predicate)>(predicate), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void CmpOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, uint32_t predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addAttribute(getPredicateAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), predicate));
  odsState.addTypes(res);
}

CmpOp CmpOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, uint32_t predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(predicate)>(predicate), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<CmpOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CmpOp CmpOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, uint32_t predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(predicate)>(predicate), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void CmpOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, uint32_t predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addAttribute(getPredicateAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), predicate));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

CmpOp CmpOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, uint32_t predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(predicate)>(predicate), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<CmpOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CmpOp CmpOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, uint32_t predicate, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(predicate)>(predicate), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void CmpOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

CmpOp CmpOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<CmpOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

CmpOp CmpOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult CmpOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_predicate;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'predicate'");
    if (namedAttrIt->getName() == getPredicateAttrName()) {
      tblgen_predicate = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_predicate, "predicate")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult CmpOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

void CmpOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::CmpOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::ConstantOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::Attribute ConstantOpGenericAdaptorBase::getValueAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::Attribute>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, ConstantOp::getValueAttrName(*odsOpName)));
  return attr;
}

::mlir::Attribute ConstantOpGenericAdaptorBase::getValue() {
  auto attr = getValueAttr();
  return attr;
}


} // namespace detail
ConstantOpAdaptor::ConstantOpAdaptor(ConstantOp op) : ConstantOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ConstantOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_value;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.constant' op requires attribute 'value'");
    if (namedAttrIt->getName() == ConstantOp::getValueAttrName(*odsOpName)) {
      tblgen_value = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_value && !((true)))
    return emitError(loc, "'moksha.constant' op attribute 'value' failed to satisfy constraint: any attribute");
  return ::mlir::success();
}

::mlir::Attribute ConstantOp::getValue() {
  auto attr = getValueAttr();
  return attr;
}

void ConstantOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Attribute value) {
  odsState.addAttribute(getValueAttrName(odsState.name), value);
  odsState.addTypes(res);
}

ConstantOp ConstantOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Attribute value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<ConstantOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ConstantOp ConstantOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Attribute value) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(value)>(value));
}

void ConstantOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Attribute value) {
  odsState.addAttribute(getValueAttrName(odsState.name), value);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ConstantOp ConstantOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Attribute value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<ConstantOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ConstantOp ConstantOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Attribute value) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
}

void ConstantOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 0u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ConstantOp ConstantOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<ConstantOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ConstantOp ConstantOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult ConstantOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_value;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'value'");
    if (namedAttrIt->getName() == getValueAttrName()) {
      tblgen_value = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps5(*this, tblgen_value, "value")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ConstantOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

void ConstantOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ConstantOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::DivOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
DivOpAdaptor::DivOpAdaptor(DivOp op) : DivOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult DivOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void DivOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

DivOp DivOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<DivOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

DivOp DivOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void DivOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

DivOp DivOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<DivOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

DivOp DivOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void DivOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

DivOp DivOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<DivOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

DivOp DivOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult DivOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult DivOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult DivOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand lhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> lhsOperands(&lhsRawOperand, 1);  ::llvm::SMLoc lhsOperandsLoc;
  (void)lhsOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand rhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> rhsOperands(&rhsRawOperand, 1);  ::llvm::SMLoc rhsOperandsLoc;
  (void)rhsOperandsLoc;
  ::mlir::Type lhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> lhsTypes(&lhsRawType, 1);
  ::mlir::Type rhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> rhsTypes(&rhsRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  lhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(lhsRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  rhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(rhsRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    lhsRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    rhsRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(lhsOperands, lhsTypes, lhsOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(rhsOperands, rhsTypes, rhsOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void DivOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getLhs();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getRhs();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getLhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getRhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void DivOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::DivOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::ExtractValueOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr ExtractValueOpGenericAdaptorBase::getIndexAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, ExtractValueOp::getIndexAttrName(*odsOpName)));
  return attr;
}

uint32_t ExtractValueOpGenericAdaptorBase::getIndex() {
  auto attr = getIndexAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
ExtractValueOpAdaptor::ExtractValueOpAdaptor(ExtractValueOp op) : ExtractValueOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ExtractValueOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_index;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.extract_value' op requires attribute 'index'");
    if (namedAttrIt->getName() == ExtractValueOp::getIndexAttrName(*odsOpName)) {
      tblgen_index = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_index && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_index))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_index).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.extract_value' op attribute 'index' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t ExtractValueOp::getIndex() {
  auto attr = getIndexAttr();
  return attr.getValue().getZExtValue();
}

void ExtractValueOp::setIndex(uint32_t attrValue) {
  (*this)->setAttr(getIndexAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void ExtractValueOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value aggregate, ::mlir::IntegerAttr index) {
  odsState.addOperands(aggregate);
  odsState.addAttribute(getIndexAttrName(odsState.name), index);
  odsState.addTypes(res);
}

ExtractValueOp ExtractValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value aggregate, ::mlir::IntegerAttr index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(index)>(index));
  auto __res__ = ::llvm::dyn_cast<ExtractValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ExtractValueOp ExtractValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value aggregate, ::mlir::IntegerAttr index) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(index)>(index));
}

void ExtractValueOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, ::mlir::IntegerAttr index) {
  odsState.addOperands(aggregate);
  odsState.addAttribute(getIndexAttrName(odsState.name), index);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ExtractValueOp ExtractValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, ::mlir::IntegerAttr index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(index)>(index));
  auto __res__ = ::llvm::dyn_cast<ExtractValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ExtractValueOp ExtractValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, ::mlir::IntegerAttr index) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(index)>(index));
}

void ExtractValueOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value aggregate, uint32_t index) {
  odsState.addOperands(aggregate);
  odsState.addAttribute(getIndexAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), index));
  odsState.addTypes(res);
}

ExtractValueOp ExtractValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value aggregate, uint32_t index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(index)>(index));
  auto __res__ = ::llvm::dyn_cast<ExtractValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ExtractValueOp ExtractValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value aggregate, uint32_t index) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(index)>(index));
}

void ExtractValueOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, uint32_t index) {
  odsState.addOperands(aggregate);
  odsState.addAttribute(getIndexAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), index));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ExtractValueOp ExtractValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, uint32_t index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(index)>(index));
  auto __res__ = ::llvm::dyn_cast<ExtractValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ExtractValueOp ExtractValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, uint32_t index) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(index)>(index));
}

void ExtractValueOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ExtractValueOp ExtractValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<ExtractValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ExtractValueOp ExtractValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult ExtractValueOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_index;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'index'");
    if (namedAttrIt->getName() == getIndexAttrName()) {
      tblgen_index = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_index, "index")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ExtractValueOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult ExtractValueOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand aggregateRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> aggregateOperands(&aggregateRawOperand, 1);  ::llvm::SMLoc aggregateOperandsLoc;
  (void)aggregateOperandsLoc;
  ::mlir::IntegerAttr indexAttr;
  ::mlir::Type aggregateRawType{};
  ::llvm::ArrayRef<::mlir::Type> aggregateTypes(&aggregateRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  aggregateOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(aggregateRawOperand))
    return ::mlir::failure();
  if (parser.parseLSquare())
    return ::mlir::failure();

  if (parser.parseCustomAttributeWithFallback(indexAttr, parser.getBuilder().getIntegerType(32))) {
    return ::mlir::failure();
  }
  if (indexAttr) result.attributes.append("index", indexAttr);
  if (parser.parseRSquare())
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    aggregateRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(aggregateOperands, aggregateTypes, aggregateOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void ExtractValueOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getAggregate();
  _odsPrinter << "[";
  _odsPrinter.printAttributeWithoutType(getIndexAttr());
  _odsPrinter << "]";
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("index");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getAggregate().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void ExtractValueOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ExtractValueOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::FenceOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr FenceOpGenericAdaptorBase::getOrderingAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, FenceOp::getOrderingAttrName(*odsOpName)));
  return attr;
}

uint32_t FenceOpGenericAdaptorBase::getOrdering() {
  auto attr = getOrderingAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
FenceOpAdaptor::FenceOpAdaptor(FenceOp op) : FenceOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult FenceOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_ordering;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.fence' op requires attribute 'ordering'");
    if (namedAttrIt->getName() == FenceOp::getOrderingAttrName(*odsOpName)) {
      tblgen_ordering = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_ordering && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_ordering))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_ordering).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.fence' op attribute 'ordering' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t FenceOp::getOrdering() {
  auto attr = getOrderingAttr();
  return attr.getValue().getZExtValue();
}

void FenceOp::setOrdering(uint32_t attrValue) {
  (*this)->setAttr(getOrderingAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void FenceOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::IntegerAttr ordering) {
  odsState.addAttribute(getOrderingAttrName(odsState.name), ordering);
}

FenceOp FenceOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::IntegerAttr ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<FenceOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

FenceOp FenceOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::IntegerAttr ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(ordering)>(ordering));
}

void FenceOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::IntegerAttr ordering) {
  odsState.addAttribute(getOrderingAttrName(odsState.name), ordering);
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

FenceOp FenceOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::IntegerAttr ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<FenceOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

FenceOp FenceOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::IntegerAttr ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ordering)>(ordering));
}

void FenceOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, uint32_t ordering) {
  odsState.addAttribute(getOrderingAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), ordering));
}

FenceOp FenceOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, uint32_t ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<FenceOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

FenceOp FenceOp::create(::mlir::ImplicitLocOpBuilder &builder, uint32_t ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(ordering)>(ordering));
}

void FenceOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, uint32_t ordering) {
  odsState.addAttribute(getOrderingAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), ordering));
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

FenceOp FenceOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, uint32_t ordering) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ordering)>(ordering));
  auto __res__ = ::llvm::dyn_cast<FenceOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

FenceOp FenceOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, uint32_t ordering) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ordering)>(ordering));
}

void FenceOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 0u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

FenceOp FenceOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<FenceOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

FenceOp FenceOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult FenceOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_ordering;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'ordering'");
    if (namedAttrIt->getName() == getOrderingAttrName()) {
      tblgen_ordering = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_ordering, "ordering")))
    return ::mlir::failure();
  return ::mlir::success();
}

::llvm::LogicalResult FenceOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult FenceOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  return ::mlir::success();
}

void FenceOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::FenceOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::GetElementPtrOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

std::pair<unsigned, unsigned> GetElementPtrOpGenericAdaptorBase::getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
  bool isVariadic[] = {false, true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (odsOperandsSize - 1) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}


} // namespace detail
GetElementPtrOpAdaptor::GetElementPtrOpAdaptor(GetElementPtrOp op) : GetElementPtrOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult GetElementPtrOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

std::pair<unsigned, unsigned> GetElementPtrOp::getODSOperandIndexAndLength(unsigned index) {
  bool isVariadic[] = {false, true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumOperands() - 1) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::MutableOperandRange GetElementPtrOp::getIndicesMutable() {
  auto range = getODSOperandIndexAndLength(1);
  auto mutableRange = ::mlir::MutableOperandRange(getOperation(), range.first, range.second);
  return mutableRange;
}

void GetElementPtrOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value base, ::mlir::ValueRange indices) {
  odsState.addOperands(base);
  odsState.addOperands(indices);
  odsState.addTypes(res);
}

GetElementPtrOp GetElementPtrOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value base, ::mlir::ValueRange indices) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(base)>(base), std::forward<decltype(indices)>(indices));
  auto __res__ = ::llvm::dyn_cast<GetElementPtrOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

GetElementPtrOp GetElementPtrOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value base, ::mlir::ValueRange indices) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(base)>(base), std::forward<decltype(indices)>(indices));
}

void GetElementPtrOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value base, ::mlir::ValueRange indices) {
  odsState.addOperands(base);
  odsState.addOperands(indices);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

GetElementPtrOp GetElementPtrOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value base, ::mlir::ValueRange indices) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(base)>(base), std::forward<decltype(indices)>(indices));
  auto __res__ = ::llvm::dyn_cast<GetElementPtrOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

GetElementPtrOp GetElementPtrOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value base, ::mlir::ValueRange indices) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(base)>(base), std::forward<decltype(indices)>(indices));
}

void GetElementPtrOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() >= 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

GetElementPtrOp GetElementPtrOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<GetElementPtrOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

GetElementPtrOp GetElementPtrOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult GetElementPtrOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult GetElementPtrOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult GetElementPtrOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand baseRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> baseOperands(&baseRawOperand, 1);  ::llvm::SMLoc baseOperandsLoc;
  (void)baseOperandsLoc;
  ::llvm::SmallVector<::mlir::OpAsmParser::UnresolvedOperand, 4> indicesOperands;
  ::llvm::SMLoc indicesOperandsLoc;
  (void)indicesOperandsLoc;
  ::mlir::Type baseRawType{};
  ::llvm::ArrayRef<::mlir::Type> baseTypes(&baseRawType, 1);
  ::llvm::SmallVector<::mlir::Type, 1> indicesTypes;
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  baseOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(baseRawOperand))
    return ::mlir::failure();
  if (parser.parseLSquare())
    return ::mlir::failure();

  indicesOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperandList(indicesOperands))
    return ::mlir::failure();
  if (parser.parseRSquare())
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    baseRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  if (parser.parseTypeList(indicesTypes))
    return ::mlir::failure();
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(baseOperands, baseTypes, baseOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(indicesOperands, indicesTypes, indicesOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void GetElementPtrOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getBase();
  _odsPrinter << "[";
  _odsPrinter << getIndices();
  _odsPrinter << "]";
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getBase().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getIndices().getTypes();
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void GetElementPtrOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::GetElementPtrOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::GlobalOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::StringAttr GlobalOpGenericAdaptorBase::getSymNameAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::StringAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 1, GlobalOp::getSymNameAttrName(*odsOpName)));
  return attr;
}

::llvm::StringRef GlobalOpGenericAdaptorBase::getSymName() {
  auto attr = getSymNameAttr();
  return attr.getValue();
}

::mlir::TypeAttr GlobalOpGenericAdaptorBase::getTypeAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::TypeAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 1, odsAttrs.end() - 0, GlobalOp::getTypeAttrName(*odsOpName)));
  return attr;
}

::mlir::Type GlobalOpGenericAdaptorBase::getType() {
  auto attr = getTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}

::mlir::Attribute GlobalOpGenericAdaptorBase::getInitialValueAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::dyn_cast_or_null<::mlir::Attribute>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 2, GlobalOp::getInitialValueAttrName(*odsOpName)));
  return attr;
}

::std::optional<::mlir::Attribute> GlobalOpGenericAdaptorBase::getInitialValue() {
  auto attr = getInitialValueAttr();
  return attr ? ::std::optional<::mlir::Attribute>(attr) : (::std::nullopt);
}


} // namespace detail
GlobalOpAdaptor::GlobalOpAdaptor(GlobalOp op) : GlobalOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult GlobalOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_sym_name;
  ::mlir::Attribute tblgen_initial_value;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.global' op requires attribute 'sym_name'");
    if (namedAttrIt->getName() == GlobalOp::getSymNameAttrName(*odsOpName)) {
      tblgen_sym_name = namedAttrIt->getValue();
      break;
    }
    else if (namedAttrIt->getName() == GlobalOp::getInitialValueAttrName(*odsOpName)) {
      tblgen_initial_value = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_type;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.global' op requires attribute 'type'");
    if (namedAttrIt->getName() == GlobalOp::getTypeAttrName(*odsOpName)) {
      tblgen_type = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_sym_name && !((::llvm::isa<::mlir::StringAttr>(tblgen_sym_name))))
    return emitError(loc, "'moksha.global' op attribute 'sym_name' failed to satisfy constraint: string attribute");

  if (tblgen_type && !(((::llvm::isa<::mlir::TypeAttr>(tblgen_type))) && ((::llvm::isa<::mlir::Type>(::llvm::cast<::mlir::TypeAttr>(tblgen_type).getValue()))) && ((true))))
    return emitError(loc, "'moksha.global' op attribute 'type' failed to satisfy constraint: any type attribute");

  if (tblgen_initial_value && !((true)))
    return emitError(loc, "'moksha.global' op attribute 'initial_value' failed to satisfy constraint: any attribute");
  return ::mlir::success();
}

::llvm::StringRef GlobalOp::getSymName() {
  auto attr = getSymNameAttr();
  return attr.getValue();
}

::mlir::Type GlobalOp::getType() {
  auto attr = getTypeAttr();
  return ::llvm::cast<::mlir::Type>(attr.getValue());
}

::std::optional<::mlir::Attribute> GlobalOp::getInitialValue() {
  auto attr = getInitialValueAttr();
  return attr ? ::std::optional<::mlir::Attribute>(attr) : (::std::nullopt);
}

void GlobalOp::setSymName(::llvm::StringRef attrValue) {
  (*this)->setAttr(getSymNameAttrName(), ::mlir::Builder((*this)->getContext()).getStringAttr(attrValue));
}

void GlobalOp::setType(::mlir::Type attrValue) {
  (*this)->setAttr(getTypeAttrName(), ::mlir::TypeAttr::get(attrValue));
}

void GlobalOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::StringAttr sym_name, ::mlir::TypeAttr type, /*optional*/::mlir::Attribute initial_value) {
  odsState.addAttribute(getSymNameAttrName(odsState.name), sym_name);
  odsState.addAttribute(getTypeAttrName(odsState.name), type);
  if (initial_value) {
    odsState.addAttribute(getInitialValueAttrName(odsState.name), initial_value);
  }
}

GlobalOp GlobalOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::StringAttr sym_name, ::mlir::TypeAttr type, /*optional*/::mlir::Attribute initial_value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(sym_name)>(sym_name), std::forward<decltype(type)>(type), std::forward<decltype(initial_value)>(initial_value));
  auto __res__ = ::llvm::dyn_cast<GlobalOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

GlobalOp GlobalOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::StringAttr sym_name, ::mlir::TypeAttr type, /*optional*/::mlir::Attribute initial_value) {
  return create(builder, builder.getLoc(), std::forward<decltype(sym_name)>(sym_name), std::forward<decltype(type)>(type), std::forward<decltype(initial_value)>(initial_value));
}

void GlobalOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::StringAttr sym_name, ::mlir::TypeAttr type, /*optional*/::mlir::Attribute initial_value) {
  odsState.addAttribute(getSymNameAttrName(odsState.name), sym_name);
  odsState.addAttribute(getTypeAttrName(odsState.name), type);
  if (initial_value) {
    odsState.addAttribute(getInitialValueAttrName(odsState.name), initial_value);
  }
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

GlobalOp GlobalOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::StringAttr sym_name, ::mlir::TypeAttr type, /*optional*/::mlir::Attribute initial_value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(sym_name)>(sym_name), std::forward<decltype(type)>(type), std::forward<decltype(initial_value)>(initial_value));
  auto __res__ = ::llvm::dyn_cast<GlobalOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

GlobalOp GlobalOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::StringAttr sym_name, ::mlir::TypeAttr type, /*optional*/::mlir::Attribute initial_value) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(sym_name)>(sym_name), std::forward<decltype(type)>(type), std::forward<decltype(initial_value)>(initial_value));
}

void GlobalOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::llvm::StringRef sym_name, ::mlir::Type type, /*optional*/::mlir::Attribute initial_value) {
  odsState.addAttribute(getSymNameAttrName(odsState.name), odsBuilder.getStringAttr(sym_name));
  odsState.addAttribute(getTypeAttrName(odsState.name), ::mlir::TypeAttr::get(type));
  if (initial_value) {
    odsState.addAttribute(getInitialValueAttrName(odsState.name), initial_value);
  }
}

GlobalOp GlobalOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::llvm::StringRef sym_name, ::mlir::Type type, /*optional*/::mlir::Attribute initial_value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(sym_name)>(sym_name), std::forward<decltype(type)>(type), std::forward<decltype(initial_value)>(initial_value));
  auto __res__ = ::llvm::dyn_cast<GlobalOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

GlobalOp GlobalOp::create(::mlir::ImplicitLocOpBuilder &builder, ::llvm::StringRef sym_name, ::mlir::Type type, /*optional*/::mlir::Attribute initial_value) {
  return create(builder, builder.getLoc(), std::forward<decltype(sym_name)>(sym_name), std::forward<decltype(type)>(type), std::forward<decltype(initial_value)>(initial_value));
}

void GlobalOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::llvm::StringRef sym_name, ::mlir::Type type, /*optional*/::mlir::Attribute initial_value) {
  odsState.addAttribute(getSymNameAttrName(odsState.name), odsBuilder.getStringAttr(sym_name));
  odsState.addAttribute(getTypeAttrName(odsState.name), ::mlir::TypeAttr::get(type));
  if (initial_value) {
    odsState.addAttribute(getInitialValueAttrName(odsState.name), initial_value);
  }
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

GlobalOp GlobalOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::llvm::StringRef sym_name, ::mlir::Type type, /*optional*/::mlir::Attribute initial_value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(sym_name)>(sym_name), std::forward<decltype(type)>(type), std::forward<decltype(initial_value)>(initial_value));
  auto __res__ = ::llvm::dyn_cast<GlobalOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

GlobalOp GlobalOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::llvm::StringRef sym_name, ::mlir::Type type, /*optional*/::mlir::Attribute initial_value) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(sym_name)>(sym_name), std::forward<decltype(type)>(type), std::forward<decltype(initial_value)>(initial_value));
}

void GlobalOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 0u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

GlobalOp GlobalOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<GlobalOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

GlobalOp GlobalOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult GlobalOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_sym_name;
  ::mlir::Attribute tblgen_initial_value;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'sym_name'");
    if (namedAttrIt->getName() == getSymNameAttrName()) {
      tblgen_sym_name = namedAttrIt->getValue();
      break;
    }
    else if (namedAttrIt->getName() == getInitialValueAttrName()) {
      tblgen_initial_value = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_type;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'type'");
    if (namedAttrIt->getName() == getTypeAttrName()) {
      tblgen_type = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps6(*this, tblgen_sym_name, "sym_name")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps2(*this, tblgen_type, "type")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps5(*this, tblgen_initial_value, "initial_value")))
    return ::mlir::failure();
  return ::mlir::success();
}

::llvm::LogicalResult GlobalOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult GlobalOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::StringAttr sym_nameAttr;
  ::mlir::TypeAttr typeAttr;
  ::mlir::Attribute initial_valueAttr;

  if (parser.parseSymbolName(sym_nameAttr))
    return ::mlir::failure();
  if (sym_nameAttr) result.attributes.append("sym_name", sym_nameAttr);
  if (parser.parseColon())
    return ::mlir::failure();

  if (parser.parseCustomAttributeWithFallback(typeAttr, parser.getBuilder().getType<::mlir::NoneType>())) {
    return ::mlir::failure();
  }
  if (typeAttr) result.attributes.append("type", typeAttr);

  ::mlir::OptionalParseResult parseResultinitial_valueAttr =
    parser.parseOptionalAttribute(initial_valueAttr, ::mlir::Type{});
  if (parseResultinitial_valueAttr.has_value() && failed(*parseResultinitial_valueAttr))
    return ::mlir::failure();
  if (parseResultinitial_valueAttr.has_value() && succeeded(*parseResultinitial_valueAttr))
  if (initial_valueAttr) result.attributes.append("initial_value", initial_valueAttr);
  if (initial_valueAttr) {
  }
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  return ::mlir::success();
}

void GlobalOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter.printSymbolName(getSymNameAttr().getValue());
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getTypeAttr());
  if (getInitialValueAttr()) {
    _odsPrinter << ' ';
    _odsPrinter.printAttribute(getInitialValueAttr());
  }
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("sym_name");
  elidedAttrs.push_back("type");
  elidedAttrs.push_back("initial_value");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::GlobalOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::InlineAsmOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

std::pair<unsigned, unsigned> InlineAsmOpGenericAdaptorBase::getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (odsOperandsSize - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::StringAttr InlineAsmOpGenericAdaptorBase::getAsmStringAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::StringAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 1, InlineAsmOp::getAsmStringAttrName(*odsOpName)));
  return attr;
}

::llvm::StringRef InlineAsmOpGenericAdaptorBase::getAsmString() {
  auto attr = getAsmStringAttr();
  return attr.getValue();
}

::mlir::StringAttr InlineAsmOpGenericAdaptorBase::getConstraintsAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::StringAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 1, odsAttrs.end() - 0, InlineAsmOp::getConstraintsAttrName(*odsOpName)));
  return attr;
}

::llvm::StringRef InlineAsmOpGenericAdaptorBase::getConstraints() {
  auto attr = getConstraintsAttr();
  return attr.getValue();
}

::mlir::UnitAttr InlineAsmOpGenericAdaptorBase::getIsVolatileAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::dyn_cast_or_null<::mlir::UnitAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 2, odsAttrs.end() - 0, InlineAsmOp::getIsVolatileAttrName(*odsOpName)));
  return attr;
}

::std::optional<bool> InlineAsmOpGenericAdaptorBase::getIsVolatile() {
  auto attr = getIsVolatileAttr();
  return attr ? ::std::optional<bool>(attr != nullptr) : (::std::nullopt);
}


} // namespace detail
InlineAsmOpAdaptor::InlineAsmOpAdaptor(InlineAsmOp op) : InlineAsmOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult InlineAsmOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_asm_string;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.asm' op requires attribute 'asm_string'");
    if (namedAttrIt->getName() == InlineAsmOp::getAsmStringAttrName(*odsOpName)) {
      tblgen_asm_string = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_constraints;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.asm' op requires attribute 'constraints'");
    if (namedAttrIt->getName() == InlineAsmOp::getConstraintsAttrName(*odsOpName)) {
      tblgen_constraints = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_is_volatile;
  while (true) {
    if (namedAttrIt == namedAttrRange.end()) {
      break;
    }
    else if (namedAttrIt->getName() == InlineAsmOp::getIsVolatileAttrName(*odsOpName)) {
      tblgen_is_volatile = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }

  if (tblgen_asm_string && !((::llvm::isa<::mlir::StringAttr>(tblgen_asm_string))))
    return emitError(loc, "'moksha.asm' op attribute 'asm_string' failed to satisfy constraint: string attribute");

  if (tblgen_constraints && !((::llvm::isa<::mlir::StringAttr>(tblgen_constraints))))
    return emitError(loc, "'moksha.asm' op attribute 'constraints' failed to satisfy constraint: string attribute");

  if (tblgen_is_volatile && !((::llvm::isa<::mlir::UnitAttr>(tblgen_is_volatile))))
    return emitError(loc, "'moksha.asm' op attribute 'is_volatile' failed to satisfy constraint: unit attribute");
  return ::mlir::success();
}

std::pair<unsigned, unsigned> InlineAsmOp::getODSOperandIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumOperands() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::MutableOperandRange InlineAsmOp::getOperandsMutable() {
  auto range = getODSOperandIndexAndLength(0);
  auto mutableRange = ::mlir::MutableOperandRange(getOperation(), range.first, range.second);
  return mutableRange;
}

std::pair<unsigned, unsigned> InlineAsmOp::getODSResultIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumResults() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::llvm::StringRef InlineAsmOp::getAsmString() {
  auto attr = getAsmStringAttr();
  return attr.getValue();
}

::llvm::StringRef InlineAsmOp::getConstraints() {
  auto attr = getConstraintsAttr();
  return attr.getValue();
}

::std::optional<bool> InlineAsmOp::getIsVolatile() {
  auto attr = getIsVolatileAttr();
  return attr ? ::std::optional<bool>(attr != nullptr) : (::std::nullopt);
}

void InlineAsmOp::setAsmString(::llvm::StringRef attrValue) {
  (*this)->setAttr(getAsmStringAttrName(), ::mlir::Builder((*this)->getContext()).getStringAttr(attrValue));
}

void InlineAsmOp::setConstraints(::llvm::StringRef attrValue) {
  (*this)->setAttr(getConstraintsAttrName(), ::mlir::Builder((*this)->getContext()).getStringAttr(attrValue));
}

void InlineAsmOp::setIsVolatile(bool attrValue) {
    if (attrValue)
      return (*this)->setAttr(getIsVolatileAttrName(), ((attrValue) ? ::mlir::Builder((*this)->getContext()).getUnitAttr() : nullptr));
    (*this)->removeAttr(getIsVolatileAttrName());
}

void InlineAsmOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange results, ::mlir::StringAttr asm_string, ::mlir::StringAttr constraints, /*optional*/::mlir::UnitAttr is_volatile, ::mlir::ValueRange operands) {
  odsState.addOperands(operands);
  odsState.addAttribute(getAsmStringAttrName(odsState.name), asm_string);
  odsState.addAttribute(getConstraintsAttrName(odsState.name), constraints);
  if (is_volatile) {
    odsState.addAttribute(getIsVolatileAttrName(odsState.name), is_volatile);
  }
  odsState.addTypes(results);
}

InlineAsmOp InlineAsmOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange results, ::mlir::StringAttr asm_string, ::mlir::StringAttr constraints, /*optional*/::mlir::UnitAttr is_volatile, ::mlir::ValueRange operands) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(results)>(results), std::forward<decltype(asm_string)>(asm_string), std::forward<decltype(constraints)>(constraints), std::forward<decltype(is_volatile)>(is_volatile), std::forward<decltype(operands)>(operands));
  auto __res__ = ::llvm::dyn_cast<InlineAsmOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InlineAsmOp InlineAsmOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange results, ::mlir::StringAttr asm_string, ::mlir::StringAttr constraints, /*optional*/::mlir::UnitAttr is_volatile, ::mlir::ValueRange operands) {
  return create(builder, builder.getLoc(), std::forward<decltype(results)>(results), std::forward<decltype(asm_string)>(asm_string), std::forward<decltype(constraints)>(constraints), std::forward<decltype(is_volatile)>(is_volatile), std::forward<decltype(operands)>(operands));
}

void InlineAsmOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange results, ::llvm::StringRef asm_string, ::llvm::StringRef constraints, /*optional*/::mlir::UnitAttr is_volatile, ::mlir::ValueRange operands) {
  odsState.addOperands(operands);
  odsState.addAttribute(getAsmStringAttrName(odsState.name), odsBuilder.getStringAttr(asm_string));
  odsState.addAttribute(getConstraintsAttrName(odsState.name), odsBuilder.getStringAttr(constraints));
  if (is_volatile) {
    odsState.addAttribute(getIsVolatileAttrName(odsState.name), is_volatile);
  }
  odsState.addTypes(results);
}

InlineAsmOp InlineAsmOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange results, ::llvm::StringRef asm_string, ::llvm::StringRef constraints, /*optional*/::mlir::UnitAttr is_volatile, ::mlir::ValueRange operands) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(results)>(results), std::forward<decltype(asm_string)>(asm_string), std::forward<decltype(constraints)>(constraints), std::forward<decltype(is_volatile)>(is_volatile), std::forward<decltype(operands)>(operands));
  auto __res__ = ::llvm::dyn_cast<InlineAsmOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InlineAsmOp InlineAsmOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange results, ::llvm::StringRef asm_string, ::llvm::StringRef constraints, /*optional*/::mlir::UnitAttr is_volatile, ::mlir::ValueRange operands) {
  return create(builder, builder.getLoc(), std::forward<decltype(results)>(results), std::forward<decltype(asm_string)>(asm_string), std::forward<decltype(constraints)>(constraints), std::forward<decltype(is_volatile)>(is_volatile), std::forward<decltype(operands)>(operands));
}

void InlineAsmOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  odsState.addTypes(resultTypes);
}

InlineAsmOp InlineAsmOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<InlineAsmOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InlineAsmOp InlineAsmOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult InlineAsmOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_asm_string;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'asm_string'");
    if (namedAttrIt->getName() == getAsmStringAttrName()) {
      tblgen_asm_string = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_constraints;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'constraints'");
    if (namedAttrIt->getName() == getConstraintsAttrName()) {
      tblgen_constraints = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_is_volatile;
  while (true) {
    if (namedAttrIt == namedAttrRange.end()) {
      break;
    }
    else if (namedAttrIt->getName() == getIsVolatileAttrName()) {
      tblgen_is_volatile = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps6(*this, tblgen_asm_string, "asm_string")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps6(*this, tblgen_constraints, "constraints")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps7(*this, tblgen_is_volatile, "is_volatile")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult InlineAsmOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult InlineAsmOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::StringAttr asm_stringAttr;
  ::mlir::StringAttr constraintsAttr;
  ::llvm::SmallVector<::mlir::OpAsmParser::UnresolvedOperand, 4> operandsOperands;
  ::llvm::SMLoc operandsOperandsLoc;
  (void)operandsOperandsLoc;
  ::llvm::ArrayRef<::mlir::Type> operandsTypes;
  ::llvm::ArrayRef<::mlir::Type> resultsTypes;

  if (parser.parseCustomAttributeWithFallback(asm_stringAttr, parser.getBuilder().getType<::mlir::NoneType>())) {
    return ::mlir::failure();
  }
  if (asm_stringAttr) result.attributes.append("asm_string", asm_stringAttr);
  if (parser.parseComma())
    return ::mlir::failure();

  if (parser.parseCustomAttributeWithFallback(constraintsAttr, parser.getBuilder().getType<::mlir::NoneType>())) {
    return ::mlir::failure();
  }
  if (constraintsAttr) result.attributes.append("constraints", constraintsAttr);
  if (parser.parseLParen())
    return ::mlir::failure();

  operandsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperandList(operandsOperands))
    return ::mlir::failure();
  if (parser.parseRParen())
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  ::mlir::FunctionType operands__results_functionType;
  if (parser.parseType(operands__results_functionType))
    return ::mlir::failure();
  operandsTypes = operands__results_functionType.getInputs();
  resultsTypes = operands__results_functionType.getResults();
  result.addTypes(resultsTypes);
  if (parser.resolveOperands(operandsOperands, operandsTypes, operandsOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void InlineAsmOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getAsmStringAttr());
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getConstraintsAttr());
  _odsPrinter << "(";
  _odsPrinter << getOperands();
  _odsPrinter << ")";
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("asm_string");
  elidedAttrs.push_back("constraints");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  _odsPrinter.printFunctionalType(getOperands().getTypes(), getResults().getTypes());
}

void InlineAsmOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
  effects.emplace_back(::mlir::MemoryEffects::Write::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
  effects.emplace_back(::mlir::MemoryEffects::Read::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::InlineAsmOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::InsertValueOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr InsertValueOpGenericAdaptorBase::getIndexAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, InsertValueOp::getIndexAttrName(*odsOpName)));
  return attr;
}

uint32_t InsertValueOpGenericAdaptorBase::getIndex() {
  auto attr = getIndexAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
InsertValueOpAdaptor::InsertValueOpAdaptor(InsertValueOp op) : InsertValueOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult InsertValueOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_index;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.insert_value' op requires attribute 'index'");
    if (namedAttrIt->getName() == InsertValueOp::getIndexAttrName(*odsOpName)) {
      tblgen_index = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_index && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_index))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_index).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.insert_value' op attribute 'index' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t InsertValueOp::getIndex() {
  auto attr = getIndexAttr();
  return attr.getValue().getZExtValue();
}

void InsertValueOp::setIndex(uint32_t attrValue) {
  (*this)->setAttr(getIndexAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void InsertValueOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value aggregate, ::mlir::Value value, ::mlir::IntegerAttr index) {
  odsState.addOperands(aggregate);
  odsState.addOperands(value);
  odsState.addAttribute(getIndexAttrName(odsState.name), index);
  odsState.addTypes(res);
}

InsertValueOp InsertValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value aggregate, ::mlir::Value value, ::mlir::IntegerAttr index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(value)>(value), std::forward<decltype(index)>(index));
  auto __res__ = ::llvm::dyn_cast<InsertValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InsertValueOp InsertValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value aggregate, ::mlir::Value value, ::mlir::IntegerAttr index) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(value)>(value), std::forward<decltype(index)>(index));
}

void InsertValueOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, ::mlir::Value value, ::mlir::IntegerAttr index) {
  odsState.addOperands(aggregate);
  odsState.addOperands(value);
  odsState.addAttribute(getIndexAttrName(odsState.name), index);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

InsertValueOp InsertValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, ::mlir::Value value, ::mlir::IntegerAttr index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(value)>(value), std::forward<decltype(index)>(index));
  auto __res__ = ::llvm::dyn_cast<InsertValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InsertValueOp InsertValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, ::mlir::Value value, ::mlir::IntegerAttr index) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(value)>(value), std::forward<decltype(index)>(index));
}

void InsertValueOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value aggregate, ::mlir::Value value, uint32_t index) {
  odsState.addOperands(aggregate);
  odsState.addOperands(value);
  odsState.addAttribute(getIndexAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), index));
  odsState.addTypes(res);
}

InsertValueOp InsertValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value aggregate, ::mlir::Value value, uint32_t index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(value)>(value), std::forward<decltype(index)>(index));
  auto __res__ = ::llvm::dyn_cast<InsertValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InsertValueOp InsertValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value aggregate, ::mlir::Value value, uint32_t index) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(value)>(value), std::forward<decltype(index)>(index));
}

void InsertValueOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, ::mlir::Value value, uint32_t index) {
  odsState.addOperands(aggregate);
  odsState.addOperands(value);
  odsState.addAttribute(getIndexAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), index));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

InsertValueOp InsertValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, ::mlir::Value value, uint32_t index) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(value)>(value), std::forward<decltype(index)>(index));
  auto __res__ = ::llvm::dyn_cast<InsertValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InsertValueOp InsertValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value aggregate, ::mlir::Value value, uint32_t index) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(aggregate)>(aggregate), std::forward<decltype(value)>(value), std::forward<decltype(index)>(index));
}

void InsertValueOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

InsertValueOp InsertValueOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<InsertValueOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InsertValueOp InsertValueOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult InsertValueOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_index;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'index'");
    if (namedAttrIt->getName() == getIndexAttrName()) {
      tblgen_index = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_index, "index")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult InsertValueOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult InsertValueOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand aggregateRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> aggregateOperands(&aggregateRawOperand, 1);  ::llvm::SMLoc aggregateOperandsLoc;
  (void)aggregateOperandsLoc;
  ::mlir::IntegerAttr indexAttr;
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);
  ::mlir::Type aggregateRawType{};
  ::llvm::ArrayRef<::mlir::Type> aggregateTypes(&aggregateRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  if (parser.parseKeyword("into"))
    return ::mlir::failure();

  aggregateOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(aggregateRawOperand))
    return ::mlir::failure();
  if (parser.parseLSquare())
    return ::mlir::failure();

  if (parser.parseCustomAttributeWithFallback(indexAttr, parser.getBuilder().getIntegerType(32))) {
    return ::mlir::failure();
  }
  if (indexAttr) result.attributes.append("index", indexAttr);
  if (parser.parseRSquare())
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    aggregateRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(aggregateOperands, aggregateTypes, aggregateOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void InsertValueOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  _odsPrinter << ' ' << "into";
  _odsPrinter << ' ';
  _odsPrinter << getAggregate();
  _odsPrinter << "[";
  _odsPrinter.printAttributeWithoutType(getIndexAttr());
  _odsPrinter << "]";
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("index");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getAggregate().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void InsertValueOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::InsertValueOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::InvokeOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

std::pair<unsigned, unsigned> InvokeOpGenericAdaptorBase::getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (odsOperandsSize - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::FlatSymbolRefAttr InvokeOpGenericAdaptorBase::getCalleeAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::FlatSymbolRefAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, InvokeOp::getCalleeAttrName(*odsOpName)));
  return attr;
}

::llvm::StringRef InvokeOpGenericAdaptorBase::getCallee() {
  auto attr = getCalleeAttr();
  return attr.getValue();
}


} // namespace detail
InvokeOpAdaptor::InvokeOpAdaptor(InvokeOp op) : InvokeOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult InvokeOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_callee;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.invoke' op requires attribute 'callee'");
    if (namedAttrIt->getName() == InvokeOp::getCalleeAttrName(*odsOpName)) {
      tblgen_callee = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_callee && !((::llvm::isa<::mlir::FlatSymbolRefAttr>(tblgen_callee))))
    return emitError(loc, "'moksha.invoke' op attribute 'callee' failed to satisfy constraint: flat symbol reference attribute");
  return ::mlir::success();
}

std::pair<unsigned, unsigned> InvokeOp::getODSOperandIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumOperands() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::MutableOperandRange InvokeOp::getOperandsMutable() {
  auto range = getODSOperandIndexAndLength(0);
  auto mutableRange = ::mlir::MutableOperandRange(getOperation(), range.first, range.second);
  return mutableRange;
}

std::pair<unsigned, unsigned> InvokeOp::getODSResultIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumResults() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::llvm::StringRef InvokeOp::getCallee() {
  auto attr = getCalleeAttr();
  return attr.getValue();
}

void InvokeOp::setCallee(::llvm::StringRef attrValue) {
  (*this)->setAttr(getCalleeAttrName(), ::mlir::SymbolRefAttr::get(::mlir::Builder((*this)->getContext()).getContext(), attrValue));
}

void InvokeOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange res, ::mlir::FlatSymbolRefAttr callee, ::mlir::ValueRange operands, ::mlir::Block *normalDest, ::mlir::Block *unwindDest) {
  odsState.addOperands(operands);
  odsState.addAttribute(getCalleeAttrName(odsState.name), callee);
  odsState.addSuccessors(normalDest);
  odsState.addSuccessors(unwindDest);
  odsState.addTypes(res);
}

InvokeOp InvokeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange res, ::mlir::FlatSymbolRefAttr callee, ::mlir::ValueRange operands, ::mlir::Block *normalDest, ::mlir::Block *unwindDest) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(callee)>(callee), std::forward<decltype(operands)>(operands), std::forward<decltype(normalDest)>(normalDest), std::forward<decltype(unwindDest)>(unwindDest));
  auto __res__ = ::llvm::dyn_cast<InvokeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InvokeOp InvokeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange res, ::mlir::FlatSymbolRefAttr callee, ::mlir::ValueRange operands, ::mlir::Block *normalDest, ::mlir::Block *unwindDest) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(callee)>(callee), std::forward<decltype(operands)>(operands), std::forward<decltype(normalDest)>(normalDest), std::forward<decltype(unwindDest)>(unwindDest));
}

void InvokeOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange res, ::llvm::StringRef callee, ::mlir::ValueRange operands, ::mlir::Block *normalDest, ::mlir::Block *unwindDest) {
  odsState.addOperands(operands);
  odsState.addAttribute(getCalleeAttrName(odsState.name), ::mlir::SymbolRefAttr::get(odsBuilder.getContext(), callee));
  odsState.addSuccessors(normalDest);
  odsState.addSuccessors(unwindDest);
  odsState.addTypes(res);
}

InvokeOp InvokeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange res, ::llvm::StringRef callee, ::mlir::ValueRange operands, ::mlir::Block *normalDest, ::mlir::Block *unwindDest) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(callee)>(callee), std::forward<decltype(operands)>(operands), std::forward<decltype(normalDest)>(normalDest), std::forward<decltype(unwindDest)>(unwindDest));
  auto __res__ = ::llvm::dyn_cast<InvokeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InvokeOp InvokeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange res, ::llvm::StringRef callee, ::mlir::ValueRange operands, ::mlir::Block *normalDest, ::mlir::Block *unwindDest) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(callee)>(callee), std::forward<decltype(operands)>(operands), std::forward<decltype(normalDest)>(normalDest), std::forward<decltype(unwindDest)>(unwindDest));
}

void InvokeOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  odsState.addTypes(resultTypes);
}

InvokeOp InvokeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<InvokeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

InvokeOp InvokeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult InvokeOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_callee;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'callee'");
    if (namedAttrIt->getName() == getCalleeAttrName()) {
      tblgen_callee = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps1(*this, tblgen_callee, "callee")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
  }
  return ::mlir::success();
}

::llvm::LogicalResult InvokeOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult InvokeOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::FlatSymbolRefAttr calleeAttr;
  ::llvm::SmallVector<::mlir::OpAsmParser::UnresolvedOperand, 4> operandsOperands;
  ::llvm::SMLoc operandsOperandsLoc;
  (void)operandsOperandsLoc;
  ::mlir::Block *normalDestSuccessor = nullptr;
  ::mlir::Block *unwindDestSuccessor = nullptr;
  ::llvm::SmallVector<::mlir::Type, 1> operandsTypes;
  ::llvm::SmallVector<::mlir::Type, 1> resTypes;

  if (parser.parseCustomAttributeWithFallback(calleeAttr, parser.getBuilder().getType<::mlir::NoneType>())) {
    return ::mlir::failure();
  }
  if (calleeAttr) result.attributes.append("callee", calleeAttr);
  if (parser.parseLParen())
    return ::mlir::failure();

  operandsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperandList(operandsOperands))
    return ::mlir::failure();
  if (parser.parseRParen())
    return ::mlir::failure();
  if (parser.parseKeyword("to"))
    return ::mlir::failure();

  if (parser.parseSuccessor(normalDestSuccessor))
    return ::mlir::failure();
  if (parser.parseKeyword("unwind"))
    return ::mlir::failure();

  if (parser.parseSuccessor(unwindDestSuccessor))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  if (parser.parseTypeList(operandsTypes))
    return ::mlir::failure();
  if (parser.parseArrow())
    return ::mlir::failure();

  if (parser.parseTypeList(resTypes))
    return ::mlir::failure();
  result.addSuccessors(normalDestSuccessor);
  result.addSuccessors(unwindDestSuccessor);
  result.addTypes(resTypes);
  if (parser.resolveOperands(operandsOperands, operandsTypes, operandsOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void InvokeOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getCalleeAttr());
  _odsPrinter << "(";
  _odsPrinter << getOperands();
  _odsPrinter << ")";
  _odsPrinter << ' ' << "to";
  _odsPrinter << ' ';
  _odsPrinter << getNormalDest();
  _odsPrinter << ' ' << "unwind";
  _odsPrinter << ' ';
  _odsPrinter << getUnwindDest();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("callee");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  _odsPrinter << getOperands().getTypes();
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  _odsPrinter << getRes().getTypes();
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::InvokeOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::LandingPadOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::UnitAttr LandingPadOpGenericAdaptorBase::getCleanupAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::dyn_cast_or_null<::mlir::UnitAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 1, odsAttrs.end() - 0, LandingPadOp::getCleanupAttrName(*odsOpName)));
  if (!attr)
    attr = ((false) ? ::mlir::Builder(odsAttrs.getContext()).getUnitAttr() : nullptr);
  return attr;
}

bool LandingPadOpGenericAdaptorBase::getCleanup() {
  auto attr = getCleanupAttr();
    if (!attr)
      return ((false) ? ::mlir::Builder(odsAttrs.getContext()).getUnitAttr() : nullptr) != nullptr;
  return attr != nullptr;
}

::mlir::ArrayAttr LandingPadOpGenericAdaptorBase::getCatchClausesAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::ArrayAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, LandingPadOp::getCatchClausesAttrName(*odsOpName)));
  return attr;
}

::mlir::ArrayAttr LandingPadOpGenericAdaptorBase::getCatchClauses() {
  auto attr = getCatchClausesAttr();
  return attr;
}


} // namespace detail
LandingPadOpAdaptor::LandingPadOpAdaptor(LandingPadOp op) : LandingPadOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult LandingPadOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_catch_clauses;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.landingpad' op requires attribute 'catch_clauses'");
    if (namedAttrIt->getName() == LandingPadOp::getCatchClausesAttrName(*odsOpName)) {
      tblgen_catch_clauses = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_cleanup;
  while (true) {
    if (namedAttrIt == namedAttrRange.end()) {
      break;
    }
    else if (namedAttrIt->getName() == LandingPadOp::getCleanupAttrName(*odsOpName)) {
      tblgen_cleanup = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }

  if (tblgen_cleanup && !((::llvm::isa<::mlir::UnitAttr>(tblgen_cleanup))))
    return emitError(loc, "'moksha.landingpad' op attribute 'cleanup' failed to satisfy constraint: unit attribute");

  if (tblgen_catch_clauses && !((::llvm::isa<::mlir::ArrayAttr>(tblgen_catch_clauses))))
    return emitError(loc, "'moksha.landingpad' op attribute 'catch_clauses' failed to satisfy constraint: array attribute");
  return ::mlir::success();
}

bool LandingPadOp::getCleanup() {
  auto attr = getCleanupAttr();
    if (!attr)
      return ((false) ? ::mlir::Builder((*this)->getContext()).getUnitAttr() : nullptr) != nullptr;
  return attr != nullptr;
}

::mlir::ArrayAttr LandingPadOp::getCatchClauses() {
  auto attr = getCatchClausesAttr();
  return attr;
}

void LandingPadOp::setCleanup(bool attrValue) {
    if (attrValue)
      return (*this)->setAttr(getCleanupAttrName(), ((attrValue) ? ::mlir::Builder((*this)->getContext()).getUnitAttr() : nullptr));
    (*this)->removeAttr(getCleanupAttrName());
}

void LandingPadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, /*optional*/::mlir::UnitAttr cleanup, ::mlir::ArrayAttr catch_clauses) {
  if (cleanup) {
    odsState.addAttribute(getCleanupAttrName(odsState.name), cleanup);
  }
  odsState.addAttribute(getCatchClausesAttrName(odsState.name), catch_clauses);
  odsState.addTypes(res);
}

LandingPadOp LandingPadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, /*optional*/::mlir::UnitAttr cleanup, ::mlir::ArrayAttr catch_clauses) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(cleanup)>(cleanup), std::forward<decltype(catch_clauses)>(catch_clauses));
  auto __res__ = ::llvm::dyn_cast<LandingPadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LandingPadOp LandingPadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, /*optional*/::mlir::UnitAttr cleanup, ::mlir::ArrayAttr catch_clauses) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(cleanup)>(cleanup), std::forward<decltype(catch_clauses)>(catch_clauses));
}

void LandingPadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, /*optional*/::mlir::UnitAttr cleanup, ::mlir::ArrayAttr catch_clauses) {
  if (cleanup) {
    odsState.addAttribute(getCleanupAttrName(odsState.name), cleanup);
  }
  odsState.addAttribute(getCatchClausesAttrName(odsState.name), catch_clauses);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

LandingPadOp LandingPadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, /*optional*/::mlir::UnitAttr cleanup, ::mlir::ArrayAttr catch_clauses) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(cleanup)>(cleanup), std::forward<decltype(catch_clauses)>(catch_clauses));
  auto __res__ = ::llvm::dyn_cast<LandingPadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LandingPadOp LandingPadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, /*optional*/::mlir::UnitAttr cleanup, ::mlir::ArrayAttr catch_clauses) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(cleanup)>(cleanup), std::forward<decltype(catch_clauses)>(catch_clauses));
}

void LandingPadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, /*optional*/bool cleanup, ::mlir::ArrayAttr catch_clauses) {
  if (cleanup) {
    odsState.addAttribute(getCleanupAttrName(odsState.name), ((cleanup) ? odsBuilder.getUnitAttr() : nullptr));
  }
  odsState.addAttribute(getCatchClausesAttrName(odsState.name), catch_clauses);
  odsState.addTypes(res);
}

LandingPadOp LandingPadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, /*optional*/bool cleanup, ::mlir::ArrayAttr catch_clauses) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(cleanup)>(cleanup), std::forward<decltype(catch_clauses)>(catch_clauses));
  auto __res__ = ::llvm::dyn_cast<LandingPadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LandingPadOp LandingPadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, /*optional*/bool cleanup, ::mlir::ArrayAttr catch_clauses) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(cleanup)>(cleanup), std::forward<decltype(catch_clauses)>(catch_clauses));
}

void LandingPadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, /*optional*/bool cleanup, ::mlir::ArrayAttr catch_clauses) {
  if (cleanup) {
    odsState.addAttribute(getCleanupAttrName(odsState.name), ((cleanup) ? odsBuilder.getUnitAttr() : nullptr));
  }
  odsState.addAttribute(getCatchClausesAttrName(odsState.name), catch_clauses);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

LandingPadOp LandingPadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, /*optional*/bool cleanup, ::mlir::ArrayAttr catch_clauses) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(cleanup)>(cleanup), std::forward<decltype(catch_clauses)>(catch_clauses));
  auto __res__ = ::llvm::dyn_cast<LandingPadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LandingPadOp LandingPadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, /*optional*/bool cleanup, ::mlir::ArrayAttr catch_clauses) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(cleanup)>(cleanup), std::forward<decltype(catch_clauses)>(catch_clauses));
}

void LandingPadOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 0u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

LandingPadOp LandingPadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<LandingPadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LandingPadOp LandingPadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult LandingPadOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_catch_clauses;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'catch_clauses'");
    if (namedAttrIt->getName() == getCatchClausesAttrName()) {
      tblgen_catch_clauses = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }
  ::mlir::Attribute tblgen_cleanup;
  while (true) {
    if (namedAttrIt == namedAttrRange.end()) {
      break;
    }
    else if (namedAttrIt->getName() == getCleanupAttrName()) {
      tblgen_cleanup = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps7(*this, tblgen_cleanup, "cleanup")))
    return ::mlir::failure();

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps8(*this, tblgen_catch_clauses, "catch_clauses")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult LandingPadOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult LandingPadOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  return ::mlir::success();
}

void LandingPadOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  {
     ::mlir::Builder odsBuilder(getContext());
     ::mlir::Attribute attr = getCleanupAttr();
     if(attr && (attr == ((false) ? odsBuilder.getUnitAttr() : nullptr)))
       elidedAttrs.push_back("cleanup");
  }
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::LandingPadOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::LoadOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
LoadOpAdaptor::LoadOpAdaptor(LoadOp op) : LoadOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult LoadOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void LoadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value ptr) {
  odsState.addOperands(ptr);
  odsState.addTypes(res);
}

LoadOp LoadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value ptr) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(ptr)>(ptr));
  auto __res__ = ::llvm::dyn_cast<LoadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LoadOp LoadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value ptr) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(ptr)>(ptr));
}

void LoadOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value ptr) {
  odsState.addOperands(ptr);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

LoadOp LoadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value ptr) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr));
  auto __res__ = ::llvm::dyn_cast<LoadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LoadOp LoadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value ptr) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr));
}

void LoadOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

LoadOp LoadOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<LoadOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LoadOp LoadOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult LoadOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult LoadOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult LoadOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand ptrRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> ptrOperands(&ptrRawOperand, 1);  ::llvm::SMLoc ptrOperandsLoc;
  (void)ptrOperandsLoc;
  ::mlir::Type ptrRawType{};
  ::llvm::ArrayRef<::mlir::Type> ptrTypes(&ptrRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  ptrOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(ptrRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    ptrRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(ptrOperands, ptrTypes, ptrOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void LoadOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getPtr();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getPtr().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void LoadOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
  effects.emplace_back(::mlir::MemoryEffects::Read::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::LoadOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::LoadWeakOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
LoadWeakOpAdaptor::LoadWeakOpAdaptor(LoadWeakOp op) : LoadWeakOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult LoadWeakOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void LoadWeakOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value ptr) {
  odsState.addOperands(ptr);
  odsState.addTypes(res);
}

LoadWeakOp LoadWeakOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value ptr) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(ptr)>(ptr));
  auto __res__ = ::llvm::dyn_cast<LoadWeakOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LoadWeakOp LoadWeakOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value ptr) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(ptr)>(ptr));
}

void LoadWeakOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value ptr) {
  odsState.addOperands(ptr);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

LoadWeakOp LoadWeakOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value ptr) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr));
  auto __res__ = ::llvm::dyn_cast<LoadWeakOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LoadWeakOp LoadWeakOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value ptr) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(ptr)>(ptr));
}

void LoadWeakOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

LoadWeakOp LoadWeakOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<LoadWeakOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

LoadWeakOp LoadWeakOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult LoadWeakOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult LoadWeakOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult LoadWeakOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand ptrRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> ptrOperands(&ptrRawOperand, 1);  ::llvm::SMLoc ptrOperandsLoc;
  (void)ptrOperandsLoc;
  ::mlir::Type ptrRawType{};
  ::llvm::ArrayRef<::mlir::Type> ptrTypes(&ptrRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  ptrOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(ptrRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    ptrRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(ptrOperands, ptrTypes, ptrOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void LoadWeakOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getPtr();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getPtr().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void LoadWeakOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
  effects.emplace_back(::mlir::MemoryEffects::Read::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::LoadWeakOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::MakeClosureOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

std::pair<unsigned, unsigned> MakeClosureOpGenericAdaptorBase::getODSOperandIndexAndLength(unsigned index, unsigned odsOperandsSize) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (odsOperandsSize - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::FlatSymbolRefAttr MakeClosureOpGenericAdaptorBase::getCalleeAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::FlatSymbolRefAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, MakeClosureOp::getCalleeAttrName(*odsOpName)));
  return attr;
}

::llvm::StringRef MakeClosureOpGenericAdaptorBase::getCallee() {
  auto attr = getCalleeAttr();
  return attr.getValue();
}


} // namespace detail
MakeClosureOpAdaptor::MakeClosureOpAdaptor(MakeClosureOp op) : MakeClosureOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult MakeClosureOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_callee;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.make_closure' op requires attribute 'callee'");
    if (namedAttrIt->getName() == MakeClosureOp::getCalleeAttrName(*odsOpName)) {
      tblgen_callee = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_callee && !((::llvm::isa<::mlir::FlatSymbolRefAttr>(tblgen_callee))))
    return emitError(loc, "'moksha.make_closure' op attribute 'callee' failed to satisfy constraint: flat symbol reference attribute");
  return ::mlir::success();
}

std::pair<unsigned, unsigned> MakeClosureOp::getODSOperandIndexAndLength(unsigned index) {
  bool isVariadic[] = {true};
  int prevVariadicCount = 0;
  for (unsigned i = 0; i < index; ++i)
    if (isVariadic[i]) ++prevVariadicCount;

  // Calculate how many dynamic values a static variadic operand corresponds to.
  // This assumes all static variadic operands have the same dynamic value count.
  int variadicSize = (getOperation()->getNumOperands() - 0) / 1;
  // `index` passed in as the parameter is the static index which counts each
  // operand (variadic or not) as size 1. So here for each previous static variadic
  // operand, we need to offset by (variadicSize - 1) to get where the dynamic
  // value pack for this static operand starts.
  int start = index + (variadicSize - 1) * prevVariadicCount;
  int size = isVariadic[index] ? variadicSize : 1;
  return {start, size};
}

::mlir::MutableOperandRange MakeClosureOp::getCapturesMutable() {
  auto range = getODSOperandIndexAndLength(0);
  auto mutableRange = ::mlir::MutableOperandRange(getOperation(), range.first, range.second);
  return mutableRange;
}

::llvm::StringRef MakeClosureOp::getCallee() {
  auto attr = getCalleeAttr();
  return attr.getValue();
}

void MakeClosureOp::setCallee(::llvm::StringRef attrValue) {
  (*this)->setAttr(getCalleeAttrName(), ::mlir::SymbolRefAttr::get(::mlir::Builder((*this)->getContext()).getContext(), attrValue));
}

void MakeClosureOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::FlatSymbolRefAttr callee, ::mlir::ValueRange captures) {
  odsState.addOperands(captures);
  odsState.addAttribute(getCalleeAttrName(odsState.name), callee);
  odsState.addTypes(res);
}

MakeClosureOp MakeClosureOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::FlatSymbolRefAttr callee, ::mlir::ValueRange captures) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(callee)>(callee), std::forward<decltype(captures)>(captures));
  auto __res__ = ::llvm::dyn_cast<MakeClosureOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MakeClosureOp MakeClosureOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::FlatSymbolRefAttr callee, ::mlir::ValueRange captures) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(callee)>(callee), std::forward<decltype(captures)>(captures));
}

void MakeClosureOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::FlatSymbolRefAttr callee, ::mlir::ValueRange captures) {
  odsState.addOperands(captures);
  odsState.addAttribute(getCalleeAttrName(odsState.name), callee);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

MakeClosureOp MakeClosureOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::FlatSymbolRefAttr callee, ::mlir::ValueRange captures) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(callee)>(callee), std::forward<decltype(captures)>(captures));
  auto __res__ = ::llvm::dyn_cast<MakeClosureOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MakeClosureOp MakeClosureOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::FlatSymbolRefAttr callee, ::mlir::ValueRange captures) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(callee)>(callee), std::forward<decltype(captures)>(captures));
}

void MakeClosureOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::llvm::StringRef callee, ::mlir::ValueRange captures) {
  odsState.addOperands(captures);
  odsState.addAttribute(getCalleeAttrName(odsState.name), ::mlir::SymbolRefAttr::get(odsBuilder.getContext(), callee));
  odsState.addTypes(res);
}

MakeClosureOp MakeClosureOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::llvm::StringRef callee, ::mlir::ValueRange captures) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(callee)>(callee), std::forward<decltype(captures)>(captures));
  auto __res__ = ::llvm::dyn_cast<MakeClosureOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MakeClosureOp MakeClosureOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::llvm::StringRef callee, ::mlir::ValueRange captures) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(callee)>(callee), std::forward<decltype(captures)>(captures));
}

void MakeClosureOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::llvm::StringRef callee, ::mlir::ValueRange captures) {
  odsState.addOperands(captures);
  odsState.addAttribute(getCalleeAttrName(odsState.name), ::mlir::SymbolRefAttr::get(odsBuilder.getContext(), callee));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

MakeClosureOp MakeClosureOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::llvm::StringRef callee, ::mlir::ValueRange captures) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(callee)>(callee), std::forward<decltype(captures)>(captures));
  auto __res__ = ::llvm::dyn_cast<MakeClosureOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MakeClosureOp MakeClosureOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::llvm::StringRef callee, ::mlir::ValueRange captures) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(callee)>(callee), std::forward<decltype(captures)>(captures));
}

void MakeClosureOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

MakeClosureOp MakeClosureOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<MakeClosureOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MakeClosureOp MakeClosureOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult MakeClosureOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_callee;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'callee'");
    if (namedAttrIt->getName() == getCalleeAttrName()) {
      tblgen_callee = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps1(*this, tblgen_callee, "callee")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps2(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult MakeClosureOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult MakeClosureOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::FlatSymbolRefAttr calleeAttr;
  ::llvm::SmallVector<::mlir::OpAsmParser::UnresolvedOperand, 4> capturesOperands;
  ::llvm::SMLoc capturesOperandsLoc;
  (void)capturesOperandsLoc;
  ::llvm::SmallVector<::mlir::Type, 1> capturesTypes;
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  if (parser.parseCustomAttributeWithFallback(calleeAttr, parser.getBuilder().getType<::mlir::NoneType>())) {
    return ::mlir::failure();
  }
  if (calleeAttr) result.attributes.append("callee", calleeAttr);
  if (parser.parseLParen())
    return ::mlir::failure();

  capturesOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperandList(capturesOperands))
    return ::mlir::failure();
  if (parser.parseRParen())
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  if (parser.parseTypeList(capturesTypes))
    return ::mlir::failure();
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(capturesOperands, capturesTypes, capturesOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void MakeClosureOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getCalleeAttr());
  _odsPrinter << "(";
  _odsPrinter << getCaptures();
  _odsPrinter << ")";
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("callee");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  _odsPrinter << getCaptures().getTypes();
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void MakeClosureOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::MakeClosureOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::MakeSharedOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
MakeSharedOpAdaptor::MakeSharedOpAdaptor(MakeSharedOp op) : MakeSharedOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult MakeSharedOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void MakeSharedOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value value) {
  odsState.addOperands(value);
  odsState.addTypes(res);
}

MakeSharedOp MakeSharedOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<MakeSharedOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MakeSharedOp MakeSharedOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value value) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(value)>(value));
}

void MakeSharedOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  odsState.addOperands(value);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

MakeSharedOp MakeSharedOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<MakeSharedOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MakeSharedOp MakeSharedOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
}

void MakeSharedOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

MakeSharedOp MakeSharedOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<MakeSharedOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MakeSharedOp MakeSharedOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult MakeSharedOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult MakeSharedOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult MakeSharedOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void MakeSharedOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::MakeSharedOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::ModOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
ModOpAdaptor::ModOpAdaptor(ModOp op) : ModOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ModOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void ModOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

ModOp ModOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<ModOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ModOp ModOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void ModOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ModOp ModOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<ModOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ModOp ModOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void ModOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ModOp ModOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<ModOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ModOp ModOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult ModOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ModOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

void ModOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ModOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::MulOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
MulOpAdaptor::MulOpAdaptor(MulOp op) : MulOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult MulOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void MulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

MulOp MulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<MulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MulOp MulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void MulOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

MulOp MulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<MulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MulOp MulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void MulOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

MulOp MulOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<MulOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

MulOp MulOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult MulOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult MulOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult MulOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand lhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> lhsOperands(&lhsRawOperand, 1);  ::llvm::SMLoc lhsOperandsLoc;
  (void)lhsOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand rhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> rhsOperands(&rhsRawOperand, 1);  ::llvm::SMLoc rhsOperandsLoc;
  (void)rhsOperandsLoc;
  ::mlir::Type lhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> lhsTypes(&lhsRawType, 1);
  ::mlir::Type rhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> rhsTypes(&rhsRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  lhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(lhsRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  rhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(rhsRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    lhsRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    rhsRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(lhsOperands, lhsTypes, lhsOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(rhsOperands, rhsTypes, rhsOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void MulOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getLhs();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getRhs();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getLhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getRhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void MulOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::MulOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::OrOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
OrOpAdaptor::OrOpAdaptor(OrOp op) : OrOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult OrOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void OrOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

OrOp OrOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<OrOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

OrOp OrOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void OrOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

OrOp OrOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<OrOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

OrOp OrOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void OrOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

OrOp OrOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<OrOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

OrOp OrOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult OrOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult OrOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

void OrOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::OrOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::PowOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
PowOpAdaptor::PowOpAdaptor(PowOp op) : PowOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult PowOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void PowOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

PowOp PowOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<PowOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PowOp PowOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void PowOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

PowOp PowOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<PowOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PowOp PowOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void PowOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

PowOp PowOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<PowOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

PowOp PowOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult PowOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult PowOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult PowOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand lhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> lhsOperands(&lhsRawOperand, 1);  ::llvm::SMLoc lhsOperandsLoc;
  (void)lhsOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand rhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> rhsOperands(&rhsRawOperand, 1);  ::llvm::SMLoc rhsOperandsLoc;
  (void)rhsOperandsLoc;
  ::mlir::Type lhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> lhsTypes(&lhsRawType, 1);
  ::mlir::Type rhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> rhsTypes(&rhsRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  lhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(lhsRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  rhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(rhsRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    lhsRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    rhsRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(lhsOperands, lhsTypes, lhsOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(rhsOperands, rhsTypes, rhsOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void PowOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getLhs();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getRhs();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getLhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getRhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void PowOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::PowOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::ReleaseOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::FlatSymbolRefAttr ReleaseOpGenericAdaptorBase::getDropFuncAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::dyn_cast_or_null<::mlir::FlatSymbolRefAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, ReleaseOp::getDropFuncAttrName(*odsOpName)));
  return attr;
}

::std::optional< ::llvm::StringRef > ReleaseOpGenericAdaptorBase::getDropFunc() {
  auto attr = getDropFuncAttr();
  return attr ? ::std::optional< ::llvm::StringRef >(attr.getValue()) : (::std::nullopt);
}


} // namespace detail
ReleaseOpAdaptor::ReleaseOpAdaptor(ReleaseOp op) : ReleaseOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ReleaseOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_drop_func;
  while (true) {
    if (namedAttrIt == namedAttrRange.end()) {
      break;
    }
    else if (namedAttrIt->getName() == ReleaseOp::getDropFuncAttrName(*odsOpName)) {
      tblgen_drop_func = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }

  if (tblgen_drop_func && !((::llvm::isa<::mlir::FlatSymbolRefAttr>(tblgen_drop_func))))
    return emitError(loc, "'moksha.release' op attribute 'drop_func' failed to satisfy constraint: flat symbol reference attribute");
  return ::mlir::success();
}

::std::optional< ::llvm::StringRef > ReleaseOp::getDropFunc() {
  auto attr = getDropFuncAttr();
  return attr ? ::std::optional< ::llvm::StringRef >(attr.getValue()) : (::std::nullopt);
}

void ReleaseOp::setDropFunc(::std::optional<::llvm::StringRef> attrValue) {
    if (attrValue)
      return (*this)->setAttr(getDropFuncAttrName(), ::mlir::SymbolRefAttr::get(::mlir::Builder((*this)->getContext()).getContext(), *attrValue));
    (*this)->removeAttr(getDropFuncAttrName());
}

void ReleaseOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value value, /*optional*/::mlir::FlatSymbolRefAttr drop_func) {
  odsState.addOperands(value);
  if (drop_func) {
    odsState.addAttribute(getDropFuncAttrName(odsState.name), drop_func);
  }
}

ReleaseOp ReleaseOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Value value, /*optional*/::mlir::FlatSymbolRefAttr drop_func) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(value)>(value), std::forward<decltype(drop_func)>(drop_func));
  auto __res__ = ::llvm::dyn_cast<ReleaseOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ReleaseOp ReleaseOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Value value, /*optional*/::mlir::FlatSymbolRefAttr drop_func) {
  return create(builder, builder.getLoc(), std::forward<decltype(value)>(value), std::forward<decltype(drop_func)>(drop_func));
}

void ReleaseOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value, /*optional*/::mlir::FlatSymbolRefAttr drop_func) {
  odsState.addOperands(value);
  if (drop_func) {
    odsState.addAttribute(getDropFuncAttrName(odsState.name), drop_func);
  }
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ReleaseOp ReleaseOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value, /*optional*/::mlir::FlatSymbolRefAttr drop_func) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(drop_func)>(drop_func));
  auto __res__ = ::llvm::dyn_cast<ReleaseOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ReleaseOp ReleaseOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value, /*optional*/::mlir::FlatSymbolRefAttr drop_func) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(drop_func)>(drop_func));
}

void ReleaseOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ReleaseOp ReleaseOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<ReleaseOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ReleaseOp ReleaseOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult ReleaseOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_drop_func;
  while (true) {
    if (namedAttrIt == namedAttrRange.end()) {
      break;
    }
    else if (namedAttrIt->getName() == getDropFuncAttrName()) {
      tblgen_drop_func = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps1(*this, tblgen_drop_func, "drop_func")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ReleaseOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult ReleaseOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::FlatSymbolRefAttr drop_funcAttr;
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  if (::mlir::succeeded(parser.parseOptionalLSquare())) {
  if (parser.parseKeyword("drop"))
    return ::mlir::failure();
  if (parser.parseColon())
    return ::mlir::failure();

  if (parser.parseCustomAttributeWithFallback(drop_funcAttr, parser.getBuilder().getType<::mlir::NoneType>())) {
    return ::mlir::failure();
  }
  if (drop_funcAttr) result.attributes.append("drop_func", drop_funcAttr);
  if (parser.parseRSquare())
    return ::mlir::failure();
  }
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void ReleaseOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  if (getDropFuncAttr()) {
    _odsPrinter << "[";
    _odsPrinter << "drop";
    _odsPrinter << ' ' << ":";
    _odsPrinter << ' ';
    _odsPrinter.printAttributeWithoutType(getDropFuncAttr());
    _odsPrinter << "]";
  }
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("drop_func");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ReleaseOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::ResumeOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
ResumeOpAdaptor::ResumeOpAdaptor(ResumeOp op) : ResumeOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ResumeOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void ResumeOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value value) {
  odsState.addOperands(value);
}

ResumeOp ResumeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<ResumeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ResumeOp ResumeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Value value) {
  return create(builder, builder.getLoc(), std::forward<decltype(value)>(value));
}

void ResumeOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  odsState.addOperands(value);
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ResumeOp ResumeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<ResumeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ResumeOp ResumeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
}

void ResumeOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ResumeOp ResumeOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<ResumeOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ResumeOp ResumeOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult ResumeOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ResumeOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult ResumeOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void ResumeOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ResumeOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::RetainOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
RetainOpAdaptor::RetainOpAdaptor(RetainOp op) : RetainOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult RetainOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void RetainOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value value) {
  odsState.addOperands(value);
}

RetainOp RetainOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<RetainOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

RetainOp RetainOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Value value) {
  return create(builder, builder.getLoc(), std::forward<decltype(value)>(value));
}

void RetainOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  odsState.addOperands(value);
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

RetainOp RetainOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
  auto __res__ = ::llvm::dyn_cast<RetainOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

RetainOp RetainOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value));
}

void RetainOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

RetainOp RetainOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<RetainOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

RetainOp RetainOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult RetainOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult RetainOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult RetainOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void RetainOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::RetainOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::ShlOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
ShlOpAdaptor::ShlOpAdaptor(ShlOp op) : ShlOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ShlOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void ShlOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

ShlOp ShlOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<ShlOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ShlOp ShlOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void ShlOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ShlOp ShlOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<ShlOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ShlOp ShlOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void ShlOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ShlOp ShlOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<ShlOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ShlOp ShlOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult ShlOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ShlOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

void ShlOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ShlOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::ShrOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
ShrOpAdaptor::ShrOpAdaptor(ShrOp op) : ShrOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ShrOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void ShrOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

ShrOp ShrOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<ShrOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ShrOp ShrOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void ShrOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ShrOp ShrOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<ShrOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ShrOp ShrOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void ShrOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ShrOp ShrOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<ShrOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ShrOp ShrOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult ShrOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult ShrOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

void ShrOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ShrOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::SpawnOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr SpawnOpGenericAdaptorBase::getThreadKindAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::cast<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, SpawnOp::getThreadKindAttrName(*odsOpName)));
  return attr;
}

uint32_t SpawnOpGenericAdaptorBase::getThreadKind() {
  auto attr = getThreadKindAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
SpawnOpAdaptor::SpawnOpAdaptor(SpawnOp op) : SpawnOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult SpawnOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_threadKind;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitError(loc, "'moksha.spawn' op requires attribute 'threadKind'");
    if (namedAttrIt->getName() == SpawnOp::getThreadKindAttrName(*odsOpName)) {
      tblgen_threadKind = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (tblgen_threadKind && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_threadKind))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_threadKind).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.spawn' op attribute 'threadKind' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t SpawnOp::getThreadKind() {
  auto attr = getThreadKindAttr();
  return attr.getValue().getZExtValue();
}

void SpawnOp::setThreadKind(uint32_t attrValue) {
  (*this)->setAttr(getThreadKindAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void SpawnOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type promise, ::mlir::Value closure, ::mlir::IntegerAttr threadKind) {
  odsState.addOperands(closure);
  odsState.addAttribute(getThreadKindAttrName(odsState.name), threadKind);
  odsState.addTypes(promise);
}

SpawnOp SpawnOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type promise, ::mlir::Value closure, ::mlir::IntegerAttr threadKind) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(promise)>(promise), std::forward<decltype(closure)>(closure), std::forward<decltype(threadKind)>(threadKind));
  auto __res__ = ::llvm::dyn_cast<SpawnOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SpawnOp SpawnOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type promise, ::mlir::Value closure, ::mlir::IntegerAttr threadKind) {
  return create(builder, builder.getLoc(), std::forward<decltype(promise)>(promise), std::forward<decltype(closure)>(closure), std::forward<decltype(threadKind)>(threadKind));
}

void SpawnOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value closure, ::mlir::IntegerAttr threadKind) {
  odsState.addOperands(closure);
  odsState.addAttribute(getThreadKindAttrName(odsState.name), threadKind);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

SpawnOp SpawnOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value closure, ::mlir::IntegerAttr threadKind) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(closure)>(closure), std::forward<decltype(threadKind)>(threadKind));
  auto __res__ = ::llvm::dyn_cast<SpawnOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SpawnOp SpawnOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value closure, ::mlir::IntegerAttr threadKind) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(closure)>(closure), std::forward<decltype(threadKind)>(threadKind));
}

void SpawnOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type promise, ::mlir::Value closure, uint32_t threadKind) {
  odsState.addOperands(closure);
  odsState.addAttribute(getThreadKindAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), threadKind));
  odsState.addTypes(promise);
}

SpawnOp SpawnOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type promise, ::mlir::Value closure, uint32_t threadKind) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(promise)>(promise), std::forward<decltype(closure)>(closure), std::forward<decltype(threadKind)>(threadKind));
  auto __res__ = ::llvm::dyn_cast<SpawnOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SpawnOp SpawnOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type promise, ::mlir::Value closure, uint32_t threadKind) {
  return create(builder, builder.getLoc(), std::forward<decltype(promise)>(promise), std::forward<decltype(closure)>(closure), std::forward<decltype(threadKind)>(threadKind));
}

void SpawnOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value closure, uint32_t threadKind) {
  odsState.addOperands(closure);
  odsState.addAttribute(getThreadKindAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), threadKind));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

SpawnOp SpawnOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value closure, uint32_t threadKind) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(closure)>(closure), std::forward<decltype(threadKind)>(threadKind));
  auto __res__ = ::llvm::dyn_cast<SpawnOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SpawnOp SpawnOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value closure, uint32_t threadKind) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(closure)>(closure), std::forward<decltype(threadKind)>(threadKind));
}

void SpawnOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

SpawnOp SpawnOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<SpawnOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SpawnOp SpawnOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult SpawnOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_threadKind;

  while (true) {
    if (namedAttrIt == namedAttrRange.end())
      return emitOpError("requires attribute 'threadKind'");
    if (namedAttrIt->getName() == getThreadKindAttrName()) {
      tblgen_threadKind = namedAttrIt->getValue();
      break;
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_threadKind, "threadKind")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps5(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps4(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult SpawnOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult SpawnOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand closureRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> closureOperands(&closureRawOperand, 1);  ::llvm::SMLoc closureOperandsLoc;
  (void)closureOperandsLoc;
  ::mlir::IntegerAttr threadKindAttr;
  ::mlir::Type closureRawType{};
  ::llvm::ArrayRef<::mlir::Type> closureTypes(&closureRawType, 1);
  ::mlir::Type promiseRawType{};
  ::llvm::ArrayRef<::mlir::Type> promiseTypes(&promiseRawType, 1);

  closureOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(closureRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  if (parser.parseCustomAttributeWithFallback(threadKindAttr, parser.getBuilder().getIntegerType(32))) {
    return ::mlir::failure();
  }
  if (threadKindAttr) result.attributes.append("threadKind", threadKindAttr);
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    moksha::IR::ClosureType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    closureRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    moksha::IR::PromiseType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    promiseRawType = type;
  }
  result.addTypes(promiseTypes);
  if (parser.resolveOperands(closureOperands, closureTypes, closureOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void SpawnOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getClosure();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter.printAttributeWithoutType(getThreadKindAttr());
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  elidedAttrs.push_back("threadKind");
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getClosure().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::ClosureType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getPromise().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PromiseType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::SpawnOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::StoreOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
StoreOpAdaptor::StoreOpAdaptor(StoreOp op) : StoreOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult StoreOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void StoreOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value value, ::mlir::Value ptr) {
  odsState.addOperands(value);
  odsState.addOperands(ptr);
}

StoreOp StoreOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Value value, ::mlir::Value ptr) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr));
  auto __res__ = ::llvm::dyn_cast<StoreOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StoreOp StoreOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Value value, ::mlir::Value ptr) {
  return create(builder, builder.getLoc(), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr));
}

void StoreOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr) {
  odsState.addOperands(value);
  odsState.addOperands(ptr);
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

StoreOp StoreOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr));
  auto __res__ = ::llvm::dyn_cast<StoreOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StoreOp StoreOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr));
}

void StoreOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

StoreOp StoreOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<StoreOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StoreOp StoreOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult StoreOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult StoreOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult StoreOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand ptrRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> ptrOperands(&ptrRawOperand, 1);  ::llvm::SMLoc ptrOperandsLoc;
  (void)ptrOperandsLoc;
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);
  ::mlir::Type ptrRawType{};
  ::llvm::ArrayRef<::mlir::Type> ptrTypes(&ptrRawType, 1);

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  ptrOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(ptrRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    ptrRawType = type;
  }
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(ptrOperands, ptrTypes, ptrOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void StoreOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getPtr();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getPtr().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void StoreOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
  effects.emplace_back(::mlir::MemoryEffects::Write::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::StoreOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::StoreWeakOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
StoreWeakOpAdaptor::StoreWeakOpAdaptor(StoreWeakOp op) : StoreWeakOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult StoreWeakOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void StoreWeakOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value value, ::mlir::Value ptr) {
  odsState.addOperands(value);
  odsState.addOperands(ptr);
}

StoreWeakOp StoreWeakOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Value value, ::mlir::Value ptr) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr));
  auto __res__ = ::llvm::dyn_cast<StoreWeakOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StoreWeakOp StoreWeakOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Value value, ::mlir::Value ptr) {
  return create(builder, builder.getLoc(), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr));
}

void StoreWeakOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr) {
  odsState.addOperands(value);
  odsState.addOperands(ptr);
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

StoreWeakOp StoreWeakOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr));
  auto __res__ = ::llvm::dyn_cast<StoreWeakOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StoreWeakOp StoreWeakOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::Value ptr) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(ptr)>(ptr));
}

void StoreWeakOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

StoreWeakOp StoreWeakOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<StoreWeakOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

StoreWeakOp StoreWeakOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult StoreWeakOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps3(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult StoreWeakOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult StoreWeakOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand valueRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> valueOperands(&valueRawOperand, 1);  ::llvm::SMLoc valueOperandsLoc;
  (void)valueOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand ptrRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> ptrOperands(&ptrRawOperand, 1);  ::llvm::SMLoc ptrOperandsLoc;
  (void)ptrOperandsLoc;
  ::mlir::Type valueRawType{};
  ::llvm::ArrayRef<::mlir::Type> valueTypes(&valueRawType, 1);
  ::mlir::Type ptrRawType{};
  ::llvm::ArrayRef<::mlir::Type> ptrTypes(&ptrRawType, 1);

  valueOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(valueRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  ptrOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(ptrRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    valueRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    moksha::IR::PointerType type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    ptrRawType = type;
  }
  if (parser.resolveOperands(valueOperands, valueTypes, valueOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(ptrOperands, ptrTypes, ptrOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void StoreWeakOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getValue();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getPtr();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getValue().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getPtr().getType();
    if (auto validType = ::llvm::dyn_cast<moksha::IR::PointerType>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void StoreWeakOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
  effects.emplace_back(::mlir::MemoryEffects::Write::get(), 0, false, ::mlir::SideEffects::DefaultResource::get());
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::StoreWeakOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::SubOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
SubOpAdaptor::SubOpAdaptor(SubOp op) : SubOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult SubOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void SubOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

SubOp SubOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<SubOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SubOp SubOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void SubOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

SubOp SubOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<SubOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SubOp SubOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void SubOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

SubOp SubOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<SubOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

SubOp SubOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult SubOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult SubOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult SubOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand lhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> lhsOperands(&lhsRawOperand, 1);  ::llvm::SMLoc lhsOperandsLoc;
  (void)lhsOperandsLoc;
  ::mlir::OpAsmParser::UnresolvedOperand rhsRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> rhsOperands(&rhsRawOperand, 1);  ::llvm::SMLoc rhsOperandsLoc;
  (void)rhsOperandsLoc;
  ::mlir::Type lhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> lhsTypes(&lhsRawType, 1);
  ::mlir::Type rhsRawType{};
  ::llvm::ArrayRef<::mlir::Type> rhsTypes(&rhsRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  lhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(lhsRawOperand))
    return ::mlir::failure();
  if (parser.parseComma())
    return ::mlir::failure();

  rhsOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(rhsRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    lhsRawType = type;
  }
  if (parser.parseComma())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    rhsRawType = type;
  }
  if (parser.parseArrow())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(lhsOperands, lhsTypes, lhsOperandsLoc, result.operands))
    return ::mlir::failure();
  if (parser.resolveOperands(rhsOperands, rhsTypes, rhsOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void SubOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getLhs();
  _odsPrinter << ",";
  _odsPrinter << ' ';
  _odsPrinter << getRhs();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getLhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ",";
  _odsPrinter << ' ';
  {
    auto type = getRhs().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "->";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void SubOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::SubOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::ThrowOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
ThrowOpAdaptor::ThrowOpAdaptor(ThrowOp op) : ThrowOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult ThrowOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void ThrowOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Value value, ::mlir::BlockRange unwindDest) {
  odsState.addOperands(value);
  odsState.addSuccessors(unwindDest);
}

ThrowOp ThrowOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Value value, ::mlir::BlockRange unwindDest) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(value)>(value), std::forward<decltype(unwindDest)>(unwindDest));
  auto __res__ = ::llvm::dyn_cast<ThrowOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ThrowOp ThrowOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Value value, ::mlir::BlockRange unwindDest) {
  return create(builder, builder.getLoc(), std::forward<decltype(value)>(value), std::forward<decltype(unwindDest)>(unwindDest));
}

void ThrowOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::BlockRange unwindDest) {
  odsState.addOperands(value);
  odsState.addSuccessors(unwindDest);
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

ThrowOp ThrowOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::BlockRange unwindDest) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(unwindDest)>(unwindDest));
  auto __res__ = ::llvm::dyn_cast<ThrowOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ThrowOp ThrowOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value value, ::mlir::BlockRange unwindDest) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(value)>(value), std::forward<decltype(unwindDest)>(unwindDest));
}

void ThrowOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

ThrowOp ThrowOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<ThrowOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

ThrowOp ThrowOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult ThrowOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
  }
  return ::mlir::success();
}

::llvm::LogicalResult ThrowOp::verifyInvariants() {
  return verifyInvariantsImpl();
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::ThrowOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::UnreachableOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
UnreachableOpAdaptor::UnreachableOpAdaptor(UnreachableOp op) : UnreachableOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult UnreachableOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void UnreachableOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState) {
}

UnreachableOp UnreachableOp::create(::mlir::OpBuilder &builder, ::mlir::Location location) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__);
  auto __res__ = ::llvm::dyn_cast<UnreachableOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

UnreachableOp UnreachableOp::create(::mlir::ImplicitLocOpBuilder &builder) {
  return create(builder, builder.getLoc());
}

void UnreachableOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes) {
  assert(resultTypes.size() == 0u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

UnreachableOp UnreachableOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes));
  auto __res__ = ::llvm::dyn_cast<UnreachableOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

UnreachableOp UnreachableOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes));
}

void UnreachableOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 0u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 0u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

UnreachableOp UnreachableOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<UnreachableOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

UnreachableOp UnreachableOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult UnreachableOp::verifyInvariantsImpl() {
  return ::mlir::success();
}

::llvm::LogicalResult UnreachableOp::verifyInvariants() {
  return verifyInvariantsImpl();
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::UnreachableOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::UpcastOp definitions
//===----------------------------------------------------------------------===//

namespace detail {

::mlir::IntegerAttr UpcastOpGenericAdaptorBase::getOffsetAttr() {
  assert(odsAttrs && "no attributes when constructing adapter");
  auto attr = ::llvm::dyn_cast_or_null<::mlir::IntegerAttr>(::mlir::impl::getAttrFromSortedRange(odsAttrs.begin() + 0, odsAttrs.end() - 0, UpcastOp::getOffsetAttrName(*odsOpName)));
  return attr;
}

uint32_t UpcastOpGenericAdaptorBase::getOffset() {
  auto attr = getOffsetAttr();
  return attr.getValue().getZExtValue();
}


} // namespace detail
UpcastOpAdaptor::UpcastOpAdaptor(UpcastOp op) : UpcastOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult UpcastOpAdaptor::verify(::mlir::Location loc) {
  auto namedAttrRange = odsAttrs;
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_offset;
  while (true) {
    if (namedAttrIt == namedAttrRange.end()) {
      break;
    }
    else if (namedAttrIt->getName() == UpcastOp::getOffsetAttrName(*odsOpName)) {
      tblgen_offset = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }

  if (tblgen_offset && !(((::llvm::isa<::mlir::IntegerAttr>(tblgen_offset))) && ((::llvm::cast<::mlir::IntegerAttr>(tblgen_offset).getType().isSignlessInteger(32)))))
    return emitError(loc, "'moksha.upcast' op attribute 'offset' failed to satisfy constraint: 32-bit signless integer attribute");
  return ::mlir::success();
}

uint32_t UpcastOp::getOffset() {
  auto attr = getOffsetAttr();
  return attr.getValue().getZExtValue();
}

void UpcastOp::setOffset(uint32_t attrValue) {
  (*this)->setAttr(getOffsetAttrName(), ::mlir::Builder((*this)->getContext()).getIntegerAttr(::mlir::Builder((*this)->getContext()).getIntegerType(32), attrValue));
}

void UpcastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value operand, ::mlir::IntegerAttr offset) {
  odsState.addOperands(operand);
  if (offset) {
    odsState.addAttribute(getOffsetAttrName(odsState.name), offset);
  }
  odsState.addTypes(res);
}

UpcastOp UpcastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value operand, ::mlir::IntegerAttr offset) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(operand)>(operand), std::forward<decltype(offset)>(offset));
  auto __res__ = ::llvm::dyn_cast<UpcastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

UpcastOp UpcastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value operand, ::mlir::IntegerAttr offset) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(operand)>(operand), std::forward<decltype(offset)>(offset));
}

void UpcastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value operand, ::mlir::IntegerAttr offset) {
  odsState.addOperands(operand);
  if (offset) {
    odsState.addAttribute(getOffsetAttrName(odsState.name), offset);
  }
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

UpcastOp UpcastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value operand, ::mlir::IntegerAttr offset) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operand)>(operand), std::forward<decltype(offset)>(offset));
  auto __res__ = ::llvm::dyn_cast<UpcastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

UpcastOp UpcastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value operand, ::mlir::IntegerAttr offset) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operand)>(operand), std::forward<decltype(offset)>(offset));
}

void UpcastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value operand, uint32_t offset) {
  odsState.addOperands(operand);
  odsState.addAttribute(getOffsetAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), offset));
  odsState.addTypes(res);
}

UpcastOp UpcastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value operand, uint32_t offset) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(operand)>(operand), std::forward<decltype(offset)>(offset));
  auto __res__ = ::llvm::dyn_cast<UpcastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

UpcastOp UpcastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value operand, uint32_t offset) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(operand)>(operand), std::forward<decltype(offset)>(offset));
}

void UpcastOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value operand, uint32_t offset) {
  odsState.addOperands(operand);
  odsState.addAttribute(getOffsetAttrName(odsState.name), odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), offset));
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

UpcastOp UpcastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value operand, uint32_t offset) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operand)>(operand), std::forward<decltype(offset)>(offset));
  auto __res__ = ::llvm::dyn_cast<UpcastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

UpcastOp UpcastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value operand, uint32_t offset) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operand)>(operand), std::forward<decltype(offset)>(offset));
}

void UpcastOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 1u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

UpcastOp UpcastOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<UpcastOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

UpcastOp UpcastOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

void UpcastOp::populateDefaultAttrs(const ::mlir::OperationName &opName, ::mlir::NamedAttrList &attributes) {
  auto attrNames = opName.getAttributeNames();
  ::mlir::Builder odsBuilder(attrNames.front().getContext());
  if (!attributes.get(attrNames[0])) {
    attributes.append(attrNames[0], odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), 0));
  }
}

::llvm::LogicalResult UpcastOp::verifyInvariantsImpl() {
  auto namedAttrRange = (*this)->getAttrs();
  auto namedAttrIt = namedAttrRange.begin();
  ::mlir::Attribute tblgen_offset;
  while (true) {
    if (namedAttrIt == namedAttrRange.end()) {
      break;
    }
    else if (namedAttrIt->getName() == getOffsetAttrName()) {
      tblgen_offset = namedAttrIt->getValue();
    }
    ++namedAttrIt;
  }

  if (::mlir::failed(__mlir_ods_local_attr_constraint_MokshaOps3(*this, tblgen_offset, "offset")))
    return ::mlir::failure();
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult UpcastOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

::mlir::ParseResult UpcastOp::parse(::mlir::OpAsmParser &parser, ::mlir::OperationState &result) {
  ::mlir::OpAsmParser::UnresolvedOperand operandRawOperand{};
  ::llvm::ArrayRef<::mlir::OpAsmParser::UnresolvedOperand> operandOperands(&operandRawOperand, 1);  ::llvm::SMLoc operandOperandsLoc;
  (void)operandOperandsLoc;
  ::mlir::Type operandRawType{};
  ::llvm::ArrayRef<::mlir::Type> operandTypes(&operandRawType, 1);
  ::mlir::Type resRawType{};
  ::llvm::ArrayRef<::mlir::Type> resTypes(&resRawType, 1);

  operandOperandsLoc = parser.getCurrentLocation();
  if (parser.parseOperand(operandRawOperand))
    return ::mlir::failure();
  {
    auto loc = parser.getCurrentLocation();(void)loc;
    if (parser.parseOptionalAttrDict(result.attributes))
      return ::mlir::failure();
  }
  if (parser.parseColon())
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    operandRawType = type;
  }
  if (parser.parseKeyword("to"))
    return ::mlir::failure();

  {
    ::mlir::Type type;
    if (parser.parseCustomTypeWithFallback(type))
      return ::mlir::failure();
    resRawType = type;
  }
  result.addTypes(resTypes);
  if (parser.resolveOperands(operandOperands, operandTypes, operandOperandsLoc, result.operands))
    return ::mlir::failure();
  return ::mlir::success();
}

void UpcastOp::print(::mlir::OpAsmPrinter &_odsPrinter) {
  _odsPrinter << ' ';
  _odsPrinter << getOperand();
  ::llvm::SmallVector<::llvm::StringRef, 2> elidedAttrs;
  {
     ::mlir::Builder odsBuilder(getContext());
     ::mlir::Attribute attr = getOffsetAttr();
     if(attr && (attr == odsBuilder.getIntegerAttr(odsBuilder.getIntegerType(32), 0)))
       elidedAttrs.push_back("offset");
  }
  _odsPrinter.printOptionalAttrDict((*this)->getAttrs(), elidedAttrs);
  _odsPrinter << ' ' << ":";
  _odsPrinter << ' ';
  {
    auto type = getOperand().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
  _odsPrinter << ' ' << "to";
  _odsPrinter << ' ';
  {
    auto type = getRes().getType();
    if (auto validType = ::llvm::dyn_cast<::mlir::Type>(type))
      _odsPrinter.printStrippedAttrOrType(validType);
   else
     _odsPrinter << type;
  }
}

void UpcastOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::UpcastOp)

namespace moksha::IR {


//===----------------------------------------------------------------------===//
// moksha::IR::XorOp definitions
//===----------------------------------------------------------------------===//

namespace detail {


} // namespace detail
XorOpAdaptor::XorOpAdaptor(XorOp op) : XorOpGenericAdaptor(op->getOperands(), op) {}

::llvm::LogicalResult XorOpAdaptor::verify(::mlir::Location loc) {
  return ::mlir::success();
}

void XorOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  odsState.addTypes(res);
}

XorOp XorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<XorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

XorOp XorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::Type res, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(res)>(res), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void XorOp::build(::mlir::OpBuilder &odsBuilder, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  odsState.addOperands(lhs);
  odsState.addOperands(rhs);
  assert(resultTypes.size() == 1u && "mismatched number of results");
  odsState.addTypes(resultTypes);
}

XorOp XorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
  auto __res__ = ::llvm::dyn_cast<XorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

XorOp XorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::Value lhs, ::mlir::Value rhs) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(lhs)>(lhs), std::forward<decltype(rhs)>(rhs));
}

void XorOp::build(::mlir::OpBuilder &, ::mlir::OperationState &odsState, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  assert(operands.size() == 2u && "mismatched number of parameters");
  odsState.addOperands(operands);
  odsState.addAttributes(attributes);
  assert(resultTypes.size() == 1u && "mismatched number of return types");
  odsState.addTypes(resultTypes);
}

XorOp XorOp::create(::mlir::OpBuilder &builder, ::mlir::Location location, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  ::mlir::OperationState __state__(location, getOperationName());
  build(builder, __state__, std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
  auto __res__ = ::llvm::dyn_cast<XorOp>(builder.create(__state__));
  assert(__res__ && "builder didn't return the right type");
  return __res__;
}

XorOp XorOp::create(::mlir::ImplicitLocOpBuilder &builder, ::mlir::TypeRange resultTypes, ::mlir::ValueRange operands, ::llvm::ArrayRef<::mlir::NamedAttribute> attributes) {
  return create(builder, builder.getLoc(), std::forward<decltype(resultTypes)>(resultTypes), std::forward<decltype(operands)>(operands), std::forward<decltype(attributes)>(attributes));
}

::llvm::LogicalResult XorOp::verifyInvariantsImpl() {
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSOperands(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
    auto valueGroup1 = getODSOperands(1);

    for (auto v : valueGroup1) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "operand", index++)))
        return ::mlir::failure();
    }
  }
  {
    unsigned index = 0; (void)index;
    auto valueGroup0 = getODSResults(0);

    for (auto v : valueGroup0) {
      if (::mlir::failed(__mlir_ods_local_type_constraint_MokshaOps1(*this, v.getType(), "result", index++)))
        return ::mlir::failure();
    }
  }
  return ::mlir::success();
}

::llvm::LogicalResult XorOp::verifyInvariants() {
  return verifyInvariantsImpl();
}

void XorOp::getEffects(::llvm::SmallVectorImpl<::mlir::SideEffects::EffectInstance<::mlir::MemoryEffects::Effect>> &effects) {
}


} // namespace moksha::IR
MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::XorOp)


#endif // GET_OP_CLASSES

