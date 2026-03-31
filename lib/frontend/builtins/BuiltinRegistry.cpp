#include "moksha/Builtins/BuiltinRegistry.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"

namespace moksha {

void BuiltinRegistry::registerBuiltins(ASTContext &ctx, SymbolTable &sym) {
  registerGenericArrayBuiltins(ctx, sym);
  registerStandardIO(ctx, sym);
  registerAtomics(ctx, sym);

  // --- Register Math / Bitwise Intrinsics ---
  SourceLocation loc;

  // 1. bswap32(val: unsigned int) -> unsigned int
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back(
        {"val",
         std::make_unique<PrimitiveType>(PrimitiveType::Scalar::U32, loc),
         loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "bswap32", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::U32, loc),
        nullptr, false, false, false, false, Visibility::Public, loc);

    funcDecl->setBuiltin(true);
    funcDecl->setIntrinsicKind(IntrinsicKind::Bswap32);

    sym.addSymbol("bswap32", Symbol(SymbolKind::Function, "bswap32", nullptr,
                                    funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // 2. clz(val: unsigned int) -> int (Count Leading Zeros)
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back(
        {"val",
         std::make_unique<PrimitiveType>(PrimitiveType::Scalar::U32, loc),
         loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "clz", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32,
                                        loc), // returns standard int
        nullptr, false, false, false, false, Visibility::Public, loc);

    funcDecl->setBuiltin(true);
    funcDecl->setIntrinsicKind(IntrinsicKind::Clz);

    sym.addSymbol("clz",
                  Symbol(SymbolKind::Function, "clz", nullptr, funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }
}

void BuiltinRegistry::registerGenericArrayBuiltins(ASTContext &ctx,
                                                   SymbolTable &sym) {
  SourceLocation loc;

  // 1. push<T>(arr: T[], val: T) -> void
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"arr",
                      std::make_unique<ArrayType>(
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          nullptr, loc),
                      loc});
    params.push_back({"val",
                      std::make_unique<NamedType>(
                          "T", std::vector<NamedType::GenericArg>{}, loc),
                      loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "push", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    auto genericDecl = std::make_unique<GenericDecl>(
        "push", std::vector<GenericDecl::GenericParam>{{"T", false, loc}},
        std::move(funcDecl), loc);

    // [FIX] Register the GenericDecl directly. No synthetic FunctionType!
    sym.addSymbol("push", Symbol(SymbolKind::Function, "push", nullptr,
                                 genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  }

  // 2. pop<T>(arr: T[]) -> T
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"arr",
                      std::make_unique<ArrayType>(
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          nullptr, loc),
                      loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "pop", std::move(params),
        std::make_unique<NamedType>("T", std::vector<NamedType::GenericArg>{},
                                    loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    auto genericDecl = std::make_unique<GenericDecl>(
        "pop", std::vector<GenericDecl::GenericParam>{{"T", false, loc}},
        std::move(funcDecl), loc);

    sym.addSymbol(
        "pop", Symbol(SymbolKind::Function, "pop", nullptr, genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  }

  // 3. length<T>(arr: T[]) -> int
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"arr",
                      std::make_unique<ArrayType>(
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          nullptr, loc),
                      loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "length", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    auto genericDecl = std::make_unique<GenericDecl>(
        "length", std::vector<GenericDecl::GenericParam>{{"T", false, loc}},
        std::move(funcDecl), loc);

    sym.addSymbol("length", Symbol(SymbolKind::Function, "length", nullptr,
                                   genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  }

  // 4. at<T>(arr: T[], index: int) -> T
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"arr",
                      std::make_unique<ArrayType>(
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          nullptr, loc),
                      loc});
    params.push_back(
        {"index",
         std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc),
         loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "at", std::move(params),
        std::make_unique<NamedType>("T", std::vector<NamedType::GenericArg>{},
                                    loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    auto genericDecl = std::make_unique<GenericDecl>(
        "at", std::vector<GenericDecl::GenericParam>{{"T", false, loc}},
        std::move(funcDecl), loc);

    sym.addSymbol(
        "at", Symbol(SymbolKind::Function, "at", nullptr, genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  }
}

void BuiltinRegistry::registerStandardIO(ASTContext &ctx, SymbolTable &sym) {
  SourceLocation loc;

  // --- length(str: string) -> int ---
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back(
        {"str",
         std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String, loc),
         loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "length", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);

    sym.addSymbol("length", Symbol(SymbolKind::Function, "length", nullptr,
                                   funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // --- at(str: string, index: i32) -> char ---
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back(
        {"str",
         std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String, loc),
         loc});
    params.push_back(
        {"index",
         std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc),
         loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "at", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Char,
                                        loc), // Returns a char!
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);

    sym.addSymbol("at",
                  Symbol(SymbolKind::Function, "at", nullptr, funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // print(msg: any...) -> void (Variadic)
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"msg", std::make_unique<AnyType>(loc), loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "print", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, true, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);

    // [FIX] Use Decl-backed symbol. Type pointer is nullptr.
    sym.addSymbol("print", Symbol(SymbolKind::Function, "print", nullptr,
                                  funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // println(msg: any...) -> void (Variadic)
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"msg", std::make_unique<AnyType>(loc), loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "println", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, true, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);

    // [FIX] Use Decl-backed symbol
    sym.addSymbol("println", Symbol(SymbolKind::Function, "println", nullptr,
                                    funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // readFile(file: any) -> string
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"file", std::make_unique<AnyType>(loc), loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "readFile", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String, loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);

    sym.addSymbol("readFile", Symbol(SymbolKind::Function, "readFile", nullptr,
                                     funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // close(file: any) -> void
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"file", std::make_unique<AnyType>(loc), loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "close", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);

    sym.addSymbol("close", Symbol(SymbolKind::Function, "close", nullptr,
                                  funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }
}

void BuiltinRegistry::registerAtomics(ASTContext &ctx, SymbolTable &sym) {
  SourceLocation loc;

  // Helper for registering Generic Atomic Unary/Binary Ops
  auto registerGenericAtomic = [&](std::string name, IntrinsicKind kind,
                                   bool returnsValue, int extraArgs) {
    std::vector<FunctionDecl::Param> params;
    // Arg 0: ptr: *T
    params.push_back({"ptr",
                      std::make_unique<PointerType>(
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          loc),
                      loc});

    // Extra Args: val: T, etc.
    for (int i = 0; i < extraArgs; ++i) {
      std::string pName = (extraArgs == 2 && i == 0)
                              ? "expected"
                              : (i == 1 ? "desired" : "val");
      params.push_back({pName,
                        std::make_unique<NamedType>(
                            "T", std::vector<NamedType::GenericArg>{}, loc),
                        loc});
    }

    TypePtr retTy = returnsValue
                        ? std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc)
                        : ctx.getVoidType()->clone();

    auto funcDecl = std::make_unique<FunctionDecl>(
        name, std::move(params), std::move(retTy), nullptr, false, false, false,
        false, Visibility::Public, loc);

    funcDecl->setBuiltin(true);
    funcDecl->setIntrinsicKind(kind);

    auto genericDecl = std::make_unique<GenericDecl>(
        name, std::vector<GenericDecl::GenericParam>{{"T", false, loc}},
        std::move(funcDecl), loc);
    sym.addSymbol(
        name, Symbol(SymbolKind::Function, name, nullptr, genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  };

  registerGenericAtomic("atomic_load", IntrinsicKind::AtomicLoad, true, 0);
  registerGenericAtomic("atomic_store", IntrinsicKind::AtomicStore, false, 1);
  registerGenericAtomic("atomic_add", IntrinsicKind::AtomicAdd, true, 1);
  registerGenericAtomic("atomic_cas", IntrinsicKind::AtomicCAS, true, 2);

  // Fences (Non-Generic)
  auto registerFence = [&](std::string name) {
    auto funcDecl = std::make_unique<FunctionDecl>(
        name, std::vector<FunctionDecl::Param>{}, ctx.getVoidType()->clone(),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    funcDecl->setIntrinsicKind(IntrinsicKind::AtomicFence);
    sym.addSymbol(name,
                  Symbol(SymbolKind::Function, name, nullptr, funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  };

  // 1. Existing Parameterless Fences
  registerFence("atomic_fence_acquire");
  registerFence("atomic_fence_release");
  registerFence("atomic_fence_seqcst");

  // 2. New atomic_thread_fence(order: string)
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back(
        {"order",
         std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String, loc),
         loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "atomic_thread_fence", std::move(params), ctx.getVoidType()->clone(),
        nullptr, false, false, false, false, Visibility::Public, loc);

    funcDecl->setBuiltin(true);
    funcDecl->setIntrinsicKind(IntrinsicKind::AtomicFence);

    sym.addSymbol("atomic_thread_fence",
                  Symbol(SymbolKind::Function, "atomic_thread_fence", nullptr,
                         funcDecl.get()));

    ctx.takeOwnership(std::move(funcDecl));
  }
}

} // namespace moksha
