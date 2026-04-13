#include "moksha/Backend/MLIR/Passes/CanonicalizeARC.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Interfaces/CallInterfaces.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/Dialect/MokshaOps.h"

namespace moksha {
namespace backend {
namespace mlir {

namespace {

struct RemoveRedundantARCPairs
    : public ::mlir::OpRewritePattern<::moksha::IR::ReleaseOp> {
  using OpRewritePattern<::moksha::IR::ReleaseOp>::OpRewritePattern;

  ::mlir::LogicalResult
  matchAndRewrite(::moksha::IR::ReleaseOp releaseOp,
                  ::mlir::PatternRewriter &rewriter) const override {
    auto ptr = releaseOp.getValue();
    ::mlir::Operation *prev = releaseOp->getPrevNode();

    while (prev) {
      if (auto retainOp = ::mlir::dyn_cast<::moksha::IR::RetainOp>(prev)) {
        if (retainOp.getValue() == ptr) {
          rewriter.eraseOp(releaseOp);
          rewriter.eraseOp(retainOp);
          return ::mlir::success();
        }
      }
      if (!prev->hasTrait<::mlir::OpTrait::AlwaysSpeculatableImplTrait>()) {
        break;
      }
      prev = prev->getPrevNode();
    }
    return ::mlir::failure();
  }
};

class CanonicalizeARCPass
    : public ::mlir::PassWrapper<CanonicalizeARCPass,
                                 ::mlir::OperationPass<::mlir::ModuleOp>> {
public:
  MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(CanonicalizeARCPass)
  ::llvm::StringRef getArgument() const final { return "canonicalize-arc"; }
  ::llvm::StringRef getDescription() const final {
    return "Canonicalize ARC operations";
  }

  void runOnOperation() override {
    ::mlir::RewritePatternSet patterns(&getContext());
    patterns.add<RemoveRedundantARCPairs>(&getContext());
    if (::mlir::failed(::mlir::applyPatternsGreedily(getOperation(),
                                                     std::move(patterns)))) {
      signalPassFailure();
    }
  }
};

} // namespace

std::unique_ptr<::mlir::Pass> createCanonicalizeARCPass() {
  return std::make_unique<CanonicalizeARCPass>();
}

} // namespace mlir
} // namespace backend
} // namespace moksha
