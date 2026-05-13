/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Interface Declarations                                                     *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/

namespace moksha::IR {

/// An interface for operations that interact with Moksha's Automatic
///         Reference Counting (ARC) system. Passes can use this to universally
///         query and optimize retain/release traffic.
class ARCOpInterface;

} // namespace moksha::IR
namespace moksha::IR {

/// Identifies operations that yield or spawn execution (like spawn/await).
class AsyncOpInterface;

} // namespace moksha::IR
namespace moksha::IR {

/// An interface for operations that interact with Moksha's Automatic
///         Reference Counting (ARC) system. Passes can use this to universally
///         query and optimize retain/release traffic.
namespace detail {
struct ARCOpInterfaceInterfaceTraits {
  struct Concept {
    /// The methods defined by the interface.
    ::mlir::Value (*getARCManagedValue)(const Concept *impl, ::mlir::Operation *);
    bool (*isRetain)(const Concept *impl, ::mlir::Operation *);
    bool (*isRelease)(const Concept *impl, ::mlir::Operation *);
  };
  template<typename ConcreteOp>
  class Model : public Concept {
  public:
    using Interface = moksha::IR::ARCOpInterface;
    Model() : Concept{getARCManagedValue, isRetain, isRelease} {}

    static inline ::mlir::Value getARCManagedValue(const Concept *impl, ::mlir::Operation *tablegen_opaque_val);
    static inline bool isRetain(const Concept *impl, ::mlir::Operation *tablegen_opaque_val);
    static inline bool isRelease(const Concept *impl, ::mlir::Operation *tablegen_opaque_val);
  };
  template<typename ConcreteOp>
  class FallbackModel : public Concept {
  public:
    using Interface = moksha::IR::ARCOpInterface;
    FallbackModel() : Concept{getARCManagedValue, isRetain, isRelease} {}

    static inline ::mlir::Value getARCManagedValue(const Concept *impl, ::mlir::Operation *tablegen_opaque_val);
    static inline bool isRetain(const Concept *impl, ::mlir::Operation *tablegen_opaque_val);
    static inline bool isRelease(const Concept *impl, ::mlir::Operation *tablegen_opaque_val);
  };
  template<typename ConcreteModel, typename ConcreteOp>
  class ExternalModel : public FallbackModel<ConcreteModel> {
  public:
    using ConcreteEntity = ConcreteOp;
  };
};
template <typename ConcreteOp>
struct ARCOpInterfaceTrait;

} // namespace detail
class ARCOpInterface : public ::mlir::OpInterface<ARCOpInterface, detail::ARCOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<ARCOpInterface, detail::ARCOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::ARCOpInterfaceTrait<ConcreteOp> {};

  /// Returns the underlying pointer/object being managed by this ARC operation.
  ::mlir::Value getARCManagedValue();

  /// Returns true if this operation increments the reference count.
  bool isRetain();

  /// Returns true if this operation decrements the reference count.
  bool isRelease();
};

} // namespace moksha::IR
namespace moksha::IR {

/// Identifies operations that yield or spawn execution (like spawn/await).
namespace detail {
struct AsyncOpInterfaceInterfaceTraits {
  struct Concept {
    /// The methods defined by the interface.
    bool (*isBlocking)(const Concept *impl, ::mlir::Operation *);
  };
  template<typename ConcreteOp>
  class Model : public Concept {
  public:
    using Interface = moksha::IR::AsyncOpInterface;
    Model() : Concept{isBlocking} {}

    static inline bool isBlocking(const Concept *impl, ::mlir::Operation *tablegen_opaque_val);
  };
  template<typename ConcreteOp>
  class FallbackModel : public Concept {
  public:
    using Interface = moksha::IR::AsyncOpInterface;
    FallbackModel() : Concept{isBlocking} {}

    static inline bool isBlocking(const Concept *impl, ::mlir::Operation *tablegen_opaque_val);
  };
  template<typename ConcreteModel, typename ConcreteOp>
  class ExternalModel : public FallbackModel<ConcreteModel> {
  public:
    using ConcreteEntity = ConcreteOp;
  };
};
template <typename ConcreteOp>
struct AsyncOpInterfaceTrait;

} // namespace detail
class AsyncOpInterface : public ::mlir::OpInterface<AsyncOpInterface, detail::AsyncOpInterfaceInterfaceTraits> {
public:
  using ::mlir::OpInterface<AsyncOpInterface, detail::AsyncOpInterfaceInterfaceTraits>::OpInterface;
  template <typename ConcreteOp>
  struct Trait : public detail::AsyncOpInterfaceTrait<ConcreteOp> {};

  /// Returns true if this operation blocks the current thread/task.
  bool isBlocking();
};

} // namespace moksha::IR
namespace moksha::IR::detail {

  template <typename ConcreteOp>
  struct ARCOpInterfaceTrait : public ::mlir::OpInterface<ARCOpInterface, detail::ARCOpInterfaceInterfaceTraits>::Trait<ConcreteOp> {
  };

} // namespace moksha::IR::detail
namespace moksha::IR::detail {

  template <typename ConcreteOp>
  struct AsyncOpInterfaceTrait : public ::mlir::OpInterface<AsyncOpInterface, detail::AsyncOpInterfaceInterfaceTraits>::Trait<ConcreteOp> {
  };

} // namespace moksha::IR::detail
namespace moksha::IR {

template<typename ConcreteOp>
::mlir::Value detail::ARCOpInterfaceInterfaceTraits::Model<ConcreteOp>::getARCManagedValue(const Concept *impl, ::mlir::Operation *tablegen_opaque_val) {
  return (llvm::cast<ConcreteOp>(tablegen_opaque_val)).getARCManagedValue();
}
template<typename ConcreteOp>
bool detail::ARCOpInterfaceInterfaceTraits::Model<ConcreteOp>::isRetain(const Concept *impl, ::mlir::Operation *tablegen_opaque_val) {
  return (llvm::cast<ConcreteOp>(tablegen_opaque_val)).isRetain();
}
template<typename ConcreteOp>
bool detail::ARCOpInterfaceInterfaceTraits::Model<ConcreteOp>::isRelease(const Concept *impl, ::mlir::Operation *tablegen_opaque_val) {
  return (llvm::cast<ConcreteOp>(tablegen_opaque_val)).isRelease();
}
template<typename ConcreteOp>
::mlir::Value detail::ARCOpInterfaceInterfaceTraits::FallbackModel<ConcreteOp>::getARCManagedValue(const Concept *impl, ::mlir::Operation *tablegen_opaque_val) {
  return static_cast<const ConcreteOp *>(impl)->getARCManagedValue(tablegen_opaque_val);
}
template<typename ConcreteOp>
bool detail::ARCOpInterfaceInterfaceTraits::FallbackModel<ConcreteOp>::isRetain(const Concept *impl, ::mlir::Operation *tablegen_opaque_val) {
  return static_cast<const ConcreteOp *>(impl)->isRetain(tablegen_opaque_val);
}
template<typename ConcreteOp>
bool detail::ARCOpInterfaceInterfaceTraits::FallbackModel<ConcreteOp>::isRelease(const Concept *impl, ::mlir::Operation *tablegen_opaque_val) {
  return static_cast<const ConcreteOp *>(impl)->isRelease(tablegen_opaque_val);
}

} // namespace moksha::IR
namespace moksha::IR {

template<typename ConcreteOp>
bool detail::AsyncOpInterfaceInterfaceTraits::Model<ConcreteOp>::isBlocking(const Concept *impl, ::mlir::Operation *tablegen_opaque_val) {
  return (llvm::cast<ConcreteOp>(tablegen_opaque_val)).isBlocking();
}
template<typename ConcreteOp>
bool detail::AsyncOpInterfaceInterfaceTraits::FallbackModel<ConcreteOp>::isBlocking(const Concept *impl, ::mlir::Operation *tablegen_opaque_val) {
  return static_cast<const ConcreteOp *>(impl)->isBlocking(tablegen_opaque_val);
}

} // namespace moksha::IR
