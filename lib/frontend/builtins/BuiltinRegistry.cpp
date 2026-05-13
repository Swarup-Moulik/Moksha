#include "moksha/Builtins/BuiltinRegistry.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"

namespace moksha {

void BuiltinRegistry::registerBuiltins(ASTContext &ctx, SymbolTable &sym) {
  registerGenericArrayBuiltins(ctx, sym);
  registerStandardIO(ctx, sym);
  registerAtomics(ctx, sym);
  registerAsyncBuiltins(ctx, sym);

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

  // Register Exception Class
  {
    std::vector<DeclPtr> members;
    SourceLocation loc;

    // constructor(msg: string)
    std::vector<FunctionDecl::Param> ctorParams;
    ctorParams.push_back(
        {"msg",
         std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String, loc),
         loc});
    members.push_back(std::make_unique<FunctionDecl>(
        "constructor", std::move(ctorParams),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, false, false, Visibility::Public, loc));

    // message: string
    members.push_back(std::make_unique<VariableDecl>(
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String, loc),
        "message", nullptr, false, /* isConst */
        false,                     /* isThreadLocal */
        false,                     /* isStatic */
        Visibility::Public, loc));

    auto classDecl = std::make_unique<ClassDecl>(
        "Exception", std::vector<std::string>{}, std::move(members),
        true, // isRefClass = true (heap allocated)
        AggregateKind::Class, Visibility::Public, loc);

    sym.addSymbol("Exception",
                  Symbol(SymbolKind::Class, "Exception",
                         ctx.createNamedType("Exception"), classDecl.get()),
                  loc);

    ctx.registerClass(classDecl.get());
    ctx.takeOwnership(std::move(classDecl));
  }
}

void BuiltinRegistry::registerGenericArrayBuiltins(ASTContext &ctx,
                                                   SymbolTable &sym) {
  SourceLocation loc;

  // 1. push<T>(arr: T[], val: T) -> void
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"arr",
                      std::make_unique<SliceType>(
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          loc),
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

    sym.addOverload("push", Symbol(SymbolKind::Function, "push", nullptr,
                                   genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  }

  // 2. pop<T>(arr: T[]) -> T
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"arr",
                      std::make_unique<SliceType>(
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          loc),
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

    sym.addOverload(
        "pop", Symbol(SymbolKind::Function, "pop", nullptr, genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  }

  // 3. length<T>(arr: T[]) -> int
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"arr",
                      std::make_unique<SliceType>(
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          loc),
                      loc});

    auto funcDecl = std::make_unique<FunctionDecl>(
        "length", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    auto genericDecl = std::make_unique<GenericDecl>(
        "length", std::vector<GenericDecl::GenericParam>{{"T", false, loc}},
        std::move(funcDecl), loc);

    sym.addOverload("length", Symbol(SymbolKind::Function, "length", nullptr,
                                     genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  }

  // 4. at<T>(arr: T[], index: int) -> T
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"arr",
                      std::make_unique<SliceType>(
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          loc),
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

    sym.addOverload(
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

    sym.addOverload("length", Symbol(SymbolKind::Function, "length", nullptr,
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

    sym.addOverload(
        "at", Symbol(SymbolKind::Function, "at", nullptr, funcDecl.get()));
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

void BuiltinRegistry::registerAsyncBuiltins(ASTContext &ctx, SymbolTable &sym) {
  SourceLocation loc;

  // 1. spawn(task: any, ...args: any) -> promise<any>
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"task", std::make_unique<AnyType>(loc), loc});
    params.push_back({"args", std::make_unique<AnyType>(loc), loc});
    auto funcDecl = std::make_unique<FunctionDecl>(
        "spawn", std::move(params),
        std::make_unique<PromiseType>(std::make_unique<AnyType>(loc), loc),
        nullptr, false, false, true /* isVariadic */, false, Visibility::Public,
        loc);
    funcDecl->setBuiltin(true);
    sym.addSymbol("spawn", Symbol(SymbolKind::Function, "spawn", nullptr,
                                  funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // 2. cancel(task: any) -> void
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"task", std::make_unique<AnyType>(loc), loc});
    auto funcDecl = std::make_unique<FunctionDecl>(
        "cancel", std::move(params),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    sym.addSymbol("cancel", Symbol(SymbolKind::Function, "cancel", nullptr,
                                   funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // 3. timeout(task: any, ms: i32) -> promise<promise<any>>
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"task", std::make_unique<AnyType>(loc), loc});
    params.push_back(
        {"ms", std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc),
         loc});
    auto funcDecl = std::make_unique<FunctionDecl>(
        "timeout", std::move(params),
        std::make_unique<PromiseType>(
            std::make_unique<PromiseType>(std::make_unique<AnyType>(loc), loc),
            loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    sym.addSymbol("timeout", Symbol(SymbolKind::Function, "timeout", nullptr,
                                    funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // 4. join(...tasks: any) -> promise<any[]>
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"tasks", std::make_unique<AnyType>(loc), loc});
    auto sliceType =
        std::make_unique<SliceType>(std::make_unique<AnyType>(loc), loc);
    auto funcDecl = std::make_unique<FunctionDecl>(
        "join", std::move(params),
        std::make_unique<PromiseType>(std::move(sliceType), loc), nullptr,
        false, false, true /* isVariadic */, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    sym.addSymbol(
        "join", Symbol(SymbolKind::Function, "join", nullptr, funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // 5. select(...tasks: any) -> promise<any>
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back({"tasks", std::make_unique<AnyType>(loc), loc});
    auto funcDecl = std::make_unique<FunctionDecl>(
        "select", std::move(params),
        std::make_unique<PromiseType>(std::make_unique<AnyType>(loc), loc),
        nullptr, false, false, true /* isVariadic */, false, Visibility::Public,
        loc);
    funcDecl->setBuiltin(true);
    sym.addSymbol("select", Symbol(SymbolKind::Function, "select", nullptr,
                                   funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // 6. yield() -> promise<void>
  {
    std::vector<FunctionDecl::Param> params;
    auto funcDecl = std::make_unique<FunctionDecl>(
        "yield", std::move(params),
        std::make_unique<PromiseType>(
            std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
            loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    sym.addSymbol("yield", Symbol(SymbolKind::Function, "yield", nullptr,
                                  funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // 7. sleep(ms: i32) -> promise<void>
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back(
        {"ms", std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc),
         loc});
    auto funcDecl = std::make_unique<FunctionDecl>(
        "sleep", std::move(params),
        std::make_unique<PromiseType>(
            std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
            loc),
        nullptr, false, false, false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    sym.addSymbol("sleep", Symbol(SymbolKind::Function, "sleep", nullptr,
                                  funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  }

  // 8. Channel<T> Generic Class
  {
    std::vector<std::unique_ptr<Decl>> members;

    // constructor(capacity: i32)
    std::vector<FunctionDecl::Param> ctorParams;
    ctorParams.push_back(
        {"capacity",
         std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc),
         loc});
    members.push_back(std::make_unique<FunctionDecl>(
        "constructor", std::move(ctorParams),
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, false, false, Visibility::Public, loc));

    // send(val: T) -> promise<void>
    std::vector<FunctionDecl::Param> sendParams;
    sendParams.push_back({"val",
                          std::make_unique<NamedType>(
                              "T", std::vector<NamedType::GenericArg>{}, loc),
                          loc});
    members.push_back(std::make_unique<FunctionDecl>(
        "send", std::move(sendParams),
        std::make_unique<PromiseType>(
            std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
            loc),
        nullptr, false, false, false, false, Visibility::Public, loc));

    // recv() -> promise<T>
    members.push_back(std::make_unique<FunctionDecl>(
        "recv", std::vector<FunctionDecl::Param>{},
        std::make_unique<PromiseType>(
            std::make_unique<NamedType>(
                "T", std::vector<NamedType::GenericArg>{}, loc),
            loc),
        nullptr, false, false, false, false, Visibility::Public, loc));

    // close() -> void
    members.push_back(std::make_unique<FunctionDecl>(
        "close", std::vector<FunctionDecl::Param>{},
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, false, false, Visibility::Public, loc));

    auto classDecl = std::make_unique<ClassDecl>(
        "Channel", std::vector<std::string>{}, std::move(members),
        true /* isRefClass */, AggregateKind::Class, Visibility::Public, loc);

    auto genericDecl = std::make_unique<GenericDecl>(
        "Channel", std::vector<GenericDecl::GenericParam>{{"T", false, loc}},
        std::move(classDecl), loc);

    sym.addSymbol("Channel", Symbol(SymbolKind::Type, "Channel", nullptr,
                                    genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  }

  // 9. AsyncMutex Class
  {
    std::vector<std::unique_ptr<Decl>> members;

    // constructor()
    members.push_back(std::make_unique<FunctionDecl>(
        "constructor", std::vector<FunctionDecl::Param>{},
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, false, false, Visibility::Public, loc));

    // lock() -> promise<void>
    members.push_back(std::make_unique<FunctionDecl>(
        "lock", std::vector<FunctionDecl::Param>{},
        std::make_unique<PromiseType>(
            std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
            loc),
        nullptr, false, false, false, false, Visibility::Public, loc));

    // unlock() -> void
    members.push_back(std::make_unique<FunctionDecl>(
        "unlock", std::vector<FunctionDecl::Param>{},
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, false, false, Visibility::Public, loc));

    auto classDecl = std::make_unique<ClassDecl>(
        "AsyncMutex", std::vector<std::string>{}, std::move(members),
        true /* isRefClass */, AggregateKind::Class, Visibility::Public, loc);

    // Register class symbol in the Symbol Table
    sym.addSymbol("AsyncMutex",
                  Symbol(SymbolKind::Class, "AsyncMutex",
                         ctx.createNamedType("AsyncMutex"), classDecl.get()),
                  loc);

    // Register in ASTContext so member lookup succeeds
    ctx.registerClass(classDecl.get());
    ctx.takeOwnership(std::move(classDecl));
  }

  // 10. ChannelClosedException Class
  {
    std::vector<std::unique_ptr<Decl>> members;

    // constructor()
    members.push_back(std::make_unique<FunctionDecl>(
        "constructor", std::vector<FunctionDecl::Param>{},
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc),
        nullptr, false, false, false, false, Visibility::Public, loc));

    auto classDecl = std::make_unique<ClassDecl>(
        "ChannelClosedException", std::vector<std::string>{},
        std::move(members), true /* isRefClass */, AggregateKind::Class,
        Visibility::Public, loc);

    sym.addSymbol("ChannelClosedException",
                  Symbol(SymbolKind::Class, "ChannelClosedException",
                         ctx.createNamedType("ChannelClosedException"),
                         classDecl.get()),
                  loc);

    ctx.registerClass(classDecl.get());
    ctx.takeOwnership(std::move(classDecl));
  }
}

} // namespace moksha
