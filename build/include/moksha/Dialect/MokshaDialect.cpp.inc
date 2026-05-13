/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Dialect Definitions                                                        *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: MokshaOps.td                                                         *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

MLIR_DEFINE_EXPLICIT_TYPE_ID(moksha::IR::MokshaDialect)
namespace moksha::IR {


MokshaDialect::MokshaDialect(::mlir::MLIRContext *context)
    : ::mlir::Dialect(getDialectNamespace(), context, ::mlir::TypeID::get<MokshaDialect>())
    
     {
  
  initialize();
}

MokshaDialect::~MokshaDialect() = default;


} // namespace moksha::IR
