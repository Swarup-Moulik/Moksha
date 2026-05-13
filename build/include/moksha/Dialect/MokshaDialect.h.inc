/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Dialect Declarations                                                       *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|* From: MokshaOps.td                                                         *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

namespace moksha::IR {

/// The Moksha Language Dialect.
/// The Moksha dialect serves as the bridge between ownership-resolved MIR
///         and final LLVM lowering. It provides managed types and ARC operations.
class MokshaDialect : public ::mlir::Dialect {
  explicit MokshaDialect(::mlir::MLIRContext *context);

  void initialize();
  friend class ::mlir::MLIRContext;
public:
  ~MokshaDialect() override;
  static constexpr ::llvm::StringLiteral getDialectNamespace() {
    return ::llvm::StringLiteral("moksha");
  }

  /// Parse a type registered to this dialect.
  ::mlir::Type parseType(::mlir::DialectAsmParser &parser) const override;

  /// Print a type registered to this dialect.
  void printType(::mlir::Type type,
                 ::mlir::DialectAsmPrinter &os) const override;

  /// Materialize a single constant operation from a given attribute value with
  /// the desired resultant type.
  ::mlir::Operation *materializeConstant(::mlir::OpBuilder &builder,
                                         ::mlir::Attribute value,
                                         ::mlir::Type type,
                                         ::mlir::Location loc) override;
};

} // namespace moksha::IR
MLIR_DECLARE_EXPLICIT_TYPE_ID(moksha::IR::MokshaDialect)
