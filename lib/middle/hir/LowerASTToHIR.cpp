#include "moksha/HIR/HIRGen.h"
#include "moksha/HIR/HIRModule.h"
#include "moksha/AST/Stmt.h"
#include <cassert>

namespace moksha {

// Main entry point for AST -> HIR lowering.
std::unique_ptr<hir::HIRModule> lowerASTToHIR(const ModuleDecl *astModule, ASTContext &ctx) {
    assert(astModule && "AST Module cannot be null");

    auto hirModule = std::make_unique<hir::HIRModule>(astModule->getName());
    HIRGen generator(ctx);

    // 1. Convert AST to HIR (Populates generator internal state)
    generator.lowerModule(astModule);

    // 2. Transfer Functions
    for (auto &func : generator.takeFunctions()) {
        assert(func && "Function from HIRGen cannot be null");
        hirModule->addFunction(std::move(func));
    }

    // 3. Transfer Global Variables
    for (auto &global : generator.takeGlobals()) {
        assert(global && "Global variable from HIRGen cannot be null");
        hirModule->addGlobal(std::move(global));
    }

    return hirModule;
}

} // namespace moksha
