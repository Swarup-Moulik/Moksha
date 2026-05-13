/*===- TableGen'erated file -------------------------------------*- C++ -*-===*\
|*                                                                            *|
|* Interface Definitions                                                      *|
|*                                                                            *|
|* Automatically generated file, do not edit!                                 *|
|*                                                                            *|
\*===----------------------------------------------------------------------===*/


/// Returns the underlying pointer/object being managed by this ARC operation.
::mlir::Value moksha::IR::ARCOpInterface::getARCManagedValue() {
      return getImpl()->getARCManagedValue(getImpl(), getOperation());
  }

/// Returns true if this operation increments the reference count.
bool moksha::IR::ARCOpInterface::isRetain() {
      return getImpl()->isRetain(getImpl(), getOperation());
  }

/// Returns true if this operation decrements the reference count.
bool moksha::IR::ARCOpInterface::isRelease() {
      return getImpl()->isRelease(getImpl(), getOperation());
  }

/// Returns true if this operation blocks the current thread/task.
bool moksha::IR::AsyncOpInterface::isBlocking() {
      return getImpl()->isBlocking(getImpl(), getOperation());
  }
