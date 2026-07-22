#include "moksha/Builtins/BuiltinRegistry.h"
#include "moksha/AST/Stmt.h"
#include "moksha/AST/Type.h"

namespace moksha {

void BuiltinRegistry::registerBuiltins(ASTContext &ctx, SymbolTable &sym) {
  registerMathBuiltins(ctx, sym);
  registerGenericArrayBuiltins(ctx, sym);
  registerStringBuiltins(ctx, sym);
  registerMapBuiltins(ctx, sym);
  registerStandardIO(ctx, sym);
  registerAtomics(ctx, sym);
  registerAsyncBuiltins(ctx, sym);
  registerFileBuiltins(ctx, sym);
  registerAllocators(ctx, sym);

  // Register Intrinsics
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
        "message", nullptr, false, false, false, Visibility::Public, loc));

    auto classDecl = std::make_unique<ClassDecl>(
        "Exception", std::vector<std::string>{}, std::move(members), true,
        AggregateKind::Class, Visibility::Public, loc);

    sym.addSymbol("Exception",
                  Symbol(SymbolKind::Class, "Exception",
                         ctx.createNamedType("Exception"), classDecl.get()),
                  loc);

    ctx.registerClass(classDecl.get());
    ctx.takeOwnership(std::move(classDecl));
  }
}

void BuiltinRegistry::registerMathBuiltins(ASTContext &ctx, SymbolTable &sym) {
  SourceLocation loc;
  auto mkF64 = [&]() -> std::unique_ptr<Type> {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::F64, loc);
  };
  auto mkI32 = [&]() -> std::unique_ptr<Type> {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc);
  };
  auto mkBool = [&]() -> std::unique_ptr<Type> {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Bool, loc);
  };
  auto mkVoid = [&]() -> std::unique_ptr<Type> {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc);
  };

  auto addConst = [&](const std::string &name) {
    auto decl = std::make_unique<VariableDecl>(
        mkF64(), name, nullptr, true, true, false, Visibility::Public, loc);

    decl->setExtern(true);

    sym.addSymbol(
        name, Symbol(SymbolKind::Variable, name, decl->getType(), decl.get()));
    ctx.takeOwnership(std::move(decl));
  };

  addConst("PI");
  addConst("E");
  addConst("TAU");
  addConst("INF");
  addConst("NAN");

  auto addMath = [&](const std::string &name,
                     std::vector<FunctionDecl::Param> params,
                     std::unique_ptr<Type> ret) {
    auto fn = std::make_unique<FunctionDecl>(
        name, std::move(params), std::move(ret), nullptr, false, false, false,
        false, Visibility::Public, loc);
    fn->setBuiltin(true);
    sym.addSymbol(name, Symbol(SymbolKind::Function, name, nullptr, fn.get()));
    ctx.takeOwnership(std::move(fn));
  };

  // Standard F64(F64) operations
  const char *f64_1[] = {"sqrt", "cbrt", "round", "floor", "ceil", "trunc",
                         "sign", "sin",  "cos",   "tan",   "asin", "acos",
                         "atan", "exp",  "log",   "log10", "log2", "abs"};
  for (const char *op : f64_1) {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"x", mkF64(), loc});
    addMath(op, std::move(p), mkF64());
  }

  // Standard F64(F64, F64) operations
  const char *f64_2[] = {"min", "max", "atan2", "mod", "fmod", "hypot"};
  for (const char *op : f64_2) {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"a", mkF64(), loc});
    p.push_back({"b", mkF64(), loc});
    addMath(op, std::move(p), mkF64());
  }

  // clamp(F64, F64, F64) -> F64
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"x", mkF64(), loc});
    p.push_back({"low", mkF64(), loc});
    p.push_back({"high", mkF64(), loc});
    addMath("clamp", std::move(p), mkF64());
  }

  // lerp(F64, F64, F64) -> F64
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"a", mkF64(), loc});
    p.push_back({"b", mkF64(), loc});
    p.push_back({"t", mkF64(), loc});
    addMath("lerp", std::move(p), mkF64());
  }

  // is_close(a: F64, b: F64, epsilon: F64) -> Bool
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"a", mkF64(), loc});
    p.push_back({"b", mkF64(), loc});
    p.push_back({"epsilon", mkF64(), loc});
    addMath("is_close", std::move(p), mkBool());
  }

  // Int/Bool Operations
  addMath("random", {}, mkF64());

  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"min", mkI32(), loc});
    p.push_back({"max", mkI32(), loc});
    addMath("randint", std::move(p), mkI32());
  }
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"value", mkI32(), loc});
    addMath("seed", std::move(p), mkVoid());
  }
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"x", mkI32(), loc});
    addMath("popcount", std::move(p), mkI32());
  }
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"x", mkI32(), loc});
    addMath("clz", std::move(p), mkI32());
  }
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"x", mkI32(), loc});
    addMath("ctz", std::move(p), mkI32());
  }
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"x", mkI32(), loc});
    addMath("isPowerOf2", std::move(p), mkBool());
  }

  // F64 checks -> Bool
  const char *checks[] = {"isnan", "isinf", "isfinite"};
  for (const char *op : checks) {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"x", mkF64(), loc});
    addMath(op, std::move(p), mkBool());
  }
}

void BuiltinRegistry::registerGenericArrayBuiltins(ASTContext &ctx,
                                                   SymbolTable &sym) {
  SourceLocation loc;

  // 1. push<T>(arr: *T[], val: T) -> void
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back(
        {"arr",
         std::make_unique<PointerType>(
             std::make_unique<SliceType>(
                 std::make_unique<NamedType>(
                     "T", std::vector<NamedType::GenericArg>{}, loc),
                 loc),
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

  // 2. pop<T>(arr: *T[]) -> T
  {
    std::vector<FunctionDecl::Param> params;
    params.push_back(
        {"arr",
         std::make_unique<PointerType>(
             std::make_unique<SliceType>(
                 std::make_unique<NamedType>(
                     "T", std::vector<NamedType::GenericArg>{}, loc),
                 loc),
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

  auto registerArrayBuiltin = [&](std::string name,
                                  std::vector<FunctionDecl::Param> params,
                                  std::unique_ptr<Type> retType) {
    auto funcDecl = std::make_unique<FunctionDecl>(
        name, std::move(params), std::move(retType), nullptr, false, false,
        false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    auto genericDecl = std::make_unique<GenericDecl>(
        name, std::vector<GenericDecl::GenericParam>{{"T", false, loc}},
        std::move(funcDecl), loc);
    sym.addOverload(
        name, Symbol(SymbolKind::Function, name, nullptr, genericDecl.get()));
    ctx.takeOwnership(std::move(genericDecl));
  };

  auto makeGenericT = [&]() {
    return std::make_unique<NamedType>(
        "T", std::vector<NamedType::GenericArg>{}, loc);
  };
  auto makeSliceT = [&]() {
    return std::make_unique<SliceType>(makeGenericT(), loc);
  };
  auto makeMutSliceT = [&]() {
    return std::make_unique<PointerType>(makeSliceT(), loc);
  };
  auto makeIntT = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc);
  };
  auto makeBoolT = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Bool, loc);
  };
  auto makeVoidT = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc);
  };

  /** @brief UNIVERSAL METHODS (Slice / View / Properties) */

  // is_empty<T>(arr: T[]) -> bool
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeSliceT(), loc});
    registerArrayBuiltin("is_empty", std::move(p), makeBoolT());
  }

  // copy<T>(dest: T[], src: T[]) -> void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"dest", makeSliceT(), loc});
    p.push_back({"src", makeSliceT(), loc});
    registerArrayBuiltin("copy", std::move(p), makeVoidT());
  }

  // slice<T>(arr: T[], start: int, end: int) -> T[]
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeSliceT(), loc});
    p.push_back({"start", makeIntT(), loc});
    p.push_back({"end", makeIntT(), loc});
    registerArrayBuiltin("slice", std::move(p), makeSliceT());
  }

  // contains<T>(arr: T[], element: T) -> bool
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeSliceT(), loc});
    p.push_back({"element", makeGenericT(), loc});
    registerArrayBuiltin("contains", std::move(p), makeBoolT());
  }

  // index<T>(arr: T[], element: T) -> int
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeSliceT(), loc});
    p.push_back({"element", makeGenericT(), loc});
    registerArrayBuiltin("index", std::move(p), makeIntT());
  }

  // fill<T>(arr: T[], value: T) -> void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeSliceT(), loc});
    p.push_back({"value", makeGenericT(), loc});
    registerArrayBuiltin("fill", std::move(p), makeVoidT());
  }

  // reverse<T>(arr: T[]) -> void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeSliceT(), loc});
    registerArrayBuiltin("reverse", std::move(p), makeVoidT());
  }

  // sort<T>(arr: T[]) -> void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeSliceT(), loc});
    registerArrayBuiltin("sort", std::move(p), makeVoidT());
  }

  /** @brief DYNAMIC-ONLY METHODS (Heap modifying) */

  // clone<T>(arr: T[]) -> T[]
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeSliceT(), loc});
    registerArrayBuiltin("clone", std::move(p), makeSliceT());
  }

  // insert<T>(arr: *T[], index: int, element: T) -> void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeMutSliceT(), loc});
    p.push_back({"index", makeIntT(), loc});
    p.push_back({"element", makeGenericT(), loc});
    registerArrayBuiltin("insert", std::move(p), makeVoidT());
  }

  // remove<T>(arr: *T[], index: int) -> T
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeMutSliceT(), loc});
    p.push_back({"index", makeIntT(), loc});
    registerArrayBuiltin("remove", std::move(p), makeGenericT());
  }

  // clear<T>(arr: *T[]) -> void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeMutSliceT(), loc});
    registerArrayBuiltin("clear", std::move(p), makeVoidT());
  }

  // capacity<T>(arr: T[]) -> int
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeSliceT(), loc});
    registerArrayBuiltin("capacity", std::move(p), makeIntT());
  }

  // resize<T>(arr: *T[], new_length: int) -> void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", makeMutSliceT(), loc});
    p.push_back({"new_length", makeIntT(), loc});
    registerArrayBuiltin("resize", std::move(p), makeVoidT());
  }
}

void BuiltinRegistry::registerStringBuiltins(ASTContext &ctx,
                                             SymbolTable &sym) {
  SourceLocation loc;

  // Helper to quickly register a builtin function
  auto addStringBuiltin = [&](const std::string &name,
                              std::vector<FunctionDecl::Param> params,
                              std::unique_ptr<Type> retType) {
    auto funcDecl = std::make_unique<FunctionDecl>(
        name, std::move(params), std::move(retType), nullptr, false, false,
        false, false, Visibility::Public, loc);

    funcDecl->setBuiltin(true);
    sym.addSymbol(name,
                  Symbol(SymbolKind::Function, name, nullptr, funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  };

  // Type-generation helpers
  auto mkString = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String, loc);
  };
  auto mkInt = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc);
  };
  auto mkBool = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Bool, loc);
  };
  auto mkChar = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Char, loc);
  };
  auto mkStringSlice = [&]() {
    return std::make_unique<SliceType>(mkString(), loc);
  };

  // 1. substring(str: string, start: int, end: int) -> string
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    p.push_back({"start", mkInt(), loc});
    p.push_back({"end", mkInt(), loc});
    addStringBuiltin("substring", std::move(p), mkString());
  }

  // 2. contains(str: string, sub: string) -> bool
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    p.push_back({"sub", mkString(), loc});
    addStringBuiltin("contains", std::move(p), mkBool());
  }

  // 3. index(str: string, sub: string) -> int
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    p.push_back({"sub", mkString(), loc});
    addStringBuiltin("index", std::move(p), mkInt());
  }

  // 4. starts_with(str: string, prefix: string) -> bool
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    p.push_back({"prefix", mkString(), loc});
    addStringBuiltin("starts_with", std::move(p), mkBool());
  }

  // 5. ends_with(str: string, suffix: string) -> bool
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    p.push_back({"suffix", mkString(), loc});
    addStringBuiltin("ends_with", std::move(p), mkBool());
  }

  // 6. slice(str: string, start: int, end: int) -> string
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    p.push_back({"start", mkInt(), loc});
    p.push_back({"end", mkInt(), loc});
    addStringBuiltin("slice", std::move(p), mkString());
  }

  // 7. to_upper(str: string) -> string
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    addStringBuiltin("to_upper", std::move(p), mkString());
  }

  // 8. to_lower(str: string) -> string
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    addStringBuiltin("to_lower", std::move(p), mkString());
  }

  // 9. trim(str: string) -> string
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    addStringBuiltin("trim", std::move(p), mkString());
  }

  // 10. replace(str: string, old_str: string, new_str: string) -> string
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    p.push_back({"old_str", mkString(), loc});
    p.push_back({"new_str", mkString(), loc});
    addStringBuiltin("replace", std::move(p), mkString());
  }

  // 11. split(str: string, delim: string) -> string[]
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"str", mkString(), loc});
    p.push_back({"delim", mkString(), loc});
    addStringBuiltin("split", std::move(p), mkStringSlice());
  }

  // 12. join(arr: string[], delim: string) -> string
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"arr", mkStringSlice(), loc});
    p.push_back({"delim", mkString(), loc});
    addStringBuiltin("join", std::move(p), mkString());
  }

  // 15. is_digit(ch: char) -> bool
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"ch", mkChar(), loc});
    addStringBuiltin("is_digit", std::move(p), mkBool());
  }

  // 16. is_alpha(ch: char) -> bool
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"ch", mkChar(), loc});
    addStringBuiltin("is_alpha", std::move(p), mkBool());
  }

  // 17. is_whitespace(ch: char) -> bool
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"ch", mkChar(), loc});
    addStringBuiltin("is_whitespace", std::move(p), mkBool());
  }
}

void BuiltinRegistry::registerMapBuiltins(ASTContext &ctx, SymbolTable &sym) {
  SourceLocation loc;
  auto mkAny = [&]() { return std::make_unique<AnyType>(loc); };
  auto mkInt = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc);
  };
  auto mkBool = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Bool, loc);
  };
  auto mkVoid = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc);
  };

  auto addMapBuiltin = [&](const std::string &name,
                           std::vector<FunctionDecl::Param> params,
                           std::unique_ptr<Type> retType) {
    auto funcDecl = std::make_unique<FunctionDecl>(
        name, std::move(params), std::move(retType), nullptr, false, false,
        false, false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);
    sym.addSymbol(name,
                  Symbol(SymbolKind::Function, name, nullptr, funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  };

  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"map", mkAny(), loc});
    addMapBuiltin("length", std::move(p), mkInt());
  }

  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"map", mkAny(), loc});
    p.push_back({"key", mkAny(), loc});
    addMapBuiltin("has", std::move(p), mkBool());
  }
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"map", mkAny(), loc});
    p.push_back({"key", mkAny(), loc});
    addMapBuiltin("remove", std::move(p), mkVoid());
  }
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"map", mkAny(), loc});
    addMapBuiltin("clear", std::move(p), mkVoid());
  }
}

void BuiltinRegistry::registerStandardIO(ASTContext &ctx, SymbolTable &sym) {
  SourceLocation loc;

  // length(str: string) -> int
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

  // at(str: string, index: i32) -> char
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
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Char, loc),
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
        false, false, true, false, Visibility::Public, loc);
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
        nullptr, false, false, true, false, Visibility::Public, loc);
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

void BuiltinRegistry::registerFileBuiltins(ASTContext &ctx, SymbolTable &sym) {
  SourceLocation loc;

  // 1. File Mode Constants (Registered as Read-Only Extern Globals)
  auto addConst = [&](const std::string &name) {
    auto decl = std::make_unique<VariableDecl>(
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc), name,
        nullptr, true, true, false, Visibility::Public, loc);
    decl->setExtern(true);
    sym.addSymbol(
        name, Symbol(SymbolKind::Variable, name, decl->getType(), decl.get()));
    ctx.takeOwnership(std::move(decl));
  };

  addConst("READ");
  addConst("WRITE");
  addConst("APPEND");
  addConst("BINARY");
  addConst("CREATE");
  addConst("TRUNCATE");

  // 2. Optimized Function Registration Helper
  enum class BType { Str, I32, I64, Bool, Any, Void };

  auto getTy = [&](BType t) -> std::unique_ptr<Type> {
    switch (t) {
    case BType::Str:
      return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::String,
                                             loc);
    case BType::I32:
      return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I32, loc);
    case BType::I64:
      return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::I64, loc);
    case BType::Bool:
      return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Bool, loc);
    case BType::Any:
      return std::make_unique<AnyType>(loc);
    case BType::Void:
      return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc);
    }
    return nullptr;
  };

  auto regFn = [&](const std::string &name,
                   const std::vector<std::pair<std::string, BType>> &args,
                   BType ret) {
    std::vector<FunctionDecl::Param> params;
    for (const auto &arg : args) {
      params.push_back({arg.first, getTy(arg.second), loc});
    }
    auto funcDecl = std::make_unique<FunctionDecl>(
        name, std::move(params), getTy(ret), nullptr, false, false, false,
        false, Visibility::Public, loc);
    funcDecl->setBuiltin(true);

    sym.addOverload(
        name, Symbol(SymbolKind::Function, name, nullptr, funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  };

  // 3. Core File Operations
  regFn("open", {{"path", BType::Str}, {"mode", BType::I32}}, BType::Any);
  regFn("close", {{"file", BType::Any}}, BType::Void);
  regFn("read", {{"file", BType::Any}}, BType::Any);
  regFn("write", {{"file", BType::Any}, {"data", BType::Any}}, BType::Void);

  // 4. Convenience Text I/O
  regFn("readText", {{"path", BType::Str}}, BType::Str);
  regFn("writeText", {{"path", BType::Str}, {"text", BType::Str}}, BType::Void);
  regFn("appendText", {{"path", BType::Str}, {"text", BType::Str}},
        BType::Void);

  // 5. Raw Byte I/O
  regFn("readBytes", {{"path", BType::Str}}, BType::Any);
  regFn("writeBytes", {{"path", BType::Str}, {"bytes", BType::Any}},
        BType::Void);
  regFn("appendBytes", {{"path", BType::Str}, {"bytes", BType::Any}},
        BType::Void);

  // 6. Structured Data (JSON/YAML/CSV)
  regFn("readJson", {{"path", BType::Str}}, BType::Any);
  regFn("writeJson", {{"path", BType::Str}, {"data", BType::Any}}, BType::Void);
  regFn("readYaml", {{"path", BType::Str}}, BType::Any);
  regFn("writeYaml", {{"path", BType::Str}, {"data", BType::Any}}, BType::Void);
  regFn("readCsv", {{"path", BType::Str}}, BType::Any);
  regFn("writeCsv", {{"path", BType::Str}, {"data", BType::Any}}, BType::Void);

  // 7. PDF Utilities
  regFn("openPdf", {{"path", BType::Str}}, BType::Any);
  regFn("createPdf", {{"path", BType::Str}}, BType::Any);
  regFn("extractText", {{"pdf", BType::Any}}, BType::Str);
  regFn("writePdfText", {{"pdf", BType::Any}, {"text", BType::Str}},
        BType::Void);
  regFn("savePdf", {{"pdf", BType::Any}}, BType::Void);

  // 8. Line Operations
  regFn("readLine", {{"file", BType::Any}}, BType::Str);
  regFn("writeLine", {{"file", BType::Any}, {"text", BType::Str}}, BType::Void);
  regFn("readLines", {{"file", BType::Any}}, BType::Any);

  // 9. Navigation and File Metadata
  regFn("seek", {{"file", BType::Any}, {"position", BType::I64}}, BType::Void);
  regFn("tell", {{"file", BType::Any}}, BType::I64);
  regFn("flush", {{"file", BType::Any}}, BType::Void);
  regFn("eof", {{"file", BType::Any}}, BType::Bool);
  regFn("size", {{"file", BType::Any}}, BType::I64);
  regFn("truncate", {{"file", BType::Any}, {"size", BType::I64}}, BType::Void);

  // 10. File System & Directory Management
  regFn("exists", {{"path", BType::Str}}, BType::Bool);
  regFn("isFile", {{"path", BType::Str}}, BType::Bool);
  regFn("isDir", {{"path", BType::Str}}, BType::Bool);
  regFn("createDir", {{"path", BType::Str}}, BType::Bool);
  regFn("remove", {{"path", BType::Str}}, BType::Bool);
  regFn("removeDir", {{"path", BType::Str}}, BType::Bool);
  regFn("copy", {{"src", BType::Str}, {"dst", BType::Str}}, BType::Bool);
  regFn("move_file", {{"src", BType::Str}, {"dst", BType::Str}}, BType::Bool);
  regFn("listDir", {{"path", BType::Str}}, BType::Any);
}

void BuiltinRegistry::registerAllocators(ASTContext &ctx, SymbolTable &sym) {
  SourceLocation loc;

  auto mkU64 = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::U64, loc);
  };
  auto mkVoidPtr = [&]() {
    return std::make_unique<PointerType>(
        std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc), loc);
  };
  auto mkVoid = [&]() {
    return std::make_unique<PrimitiveType>(PrimitiveType::Scalar::Void, loc);
  };

  // Helper to register Unsafe C-style memory functions
  auto registerMemFunc = [&](const std::string &name,
                             std::vector<FunctionDecl::Param> params,
                             std::unique_ptr<Type> retType) {
    auto funcDecl = std::make_unique<FunctionDecl>(
        name, std::move(params), std::move(retType), nullptr, false, false,
        false, false, Visibility::Public, loc);

    funcDecl->setBuiltin(true);
    funcDecl->setExtern(true); // Treat as external C functions
    funcDecl->setUnsafe(true); // ENFORCE UNSAFE BLOCK REQUIREMENT

    sym.addSymbol(name,
                  Symbol(SymbolKind::Function, name, nullptr, funcDecl.get()));
    ctx.takeOwnership(std::move(funcDecl));
  };

  // 1. malloc(size: u64) -> *void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"size", mkU64(), loc});
    registerMemFunc("malloc", std::move(p), mkVoidPtr());
  }

  // 2. calloc(num: u64, size: u64) -> *void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"num", mkU64(), loc});
    p.push_back({"size", mkU64(), loc});
    registerMemFunc("calloc", std::move(p), mkVoidPtr());
  }

  // 3. realloc(ptr: *void, new_size: u64) -> *void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"ptr", mkVoidPtr(), loc});
    p.push_back({"new_size", mkU64(), loc});
    registerMemFunc("realloc", std::move(p), mkVoidPtr());
  }

  // 4. free(ptr: *void) -> void
  {
    std::vector<FunctionDecl::Param> p;
    p.push_back({"ptr", mkVoidPtr(), loc});
    registerMemFunc("free", std::move(p), mkVoid());
  }
}

} // namespace moksha
