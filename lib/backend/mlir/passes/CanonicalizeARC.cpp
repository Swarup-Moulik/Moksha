#include "moksha/Backend/MLIR/Passes/CanonicalizeARC.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "moksha/Dialect/MokshaDialect.h"
#include "moksha/Dialect/MokshaOps.h"

namespace moksha {
namespace backend {
namespace mlir {

namespace {

struct RemoveRedundantARCPair
    : public ::mlir::OpRewritePattern<::moksha::ReleaseOp> {
  using OpRewritePattern<::moksha::ReleaseOp>::OpRewritePattern;

  ::mlir::LogicalResult
  matchAndRewrite(::moksha::ReleaseOp releaseOp,
                  ::mlir::PatternRewriter &rewriter) const override {
    ::mlir::Value releasedValue = releaseOp.getOperand();

    // [FIX] Use safe reverse iteration on the Block's instruction list
    ::mlir::Block *block = releaseOp->getBlock();
    auto &ops = block->getOperations();

    // Create a reverse iterator starting *after* the releaseOp (going
    // backwards)
    auto it = ::mlir::Block::reverse_iterator(releaseOp);
    auto end = ops.rend();

    // Skip the release op itself
    if (it != end)
      ++it;

    for (; it != end; ++it) {
      ::mlir::Operation &prevOp = *it;

      if (auto retainOp = ::llvm::dyn_cast<::moksha::RetainOp>(prevOp)) {
        if (retainOp.getOperand() == releasedValue) {
          rewriter.eraseOp(releaseOp);
          rewriter.eraseOp(retainOp);
          return ::mlir::success();
        }
      }

      if (prevOp.hasTrait<::mlir::OpTrait::IsTerminator>() ||
          !prevOp.hasTrait<::mlir::OpTrait::ConstantLike>()) {
        break;
      }
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
    ::mlir::MLIRContext *context = &getContext();
    ::mlir::RewritePatternSet patterns(context);
    patterns.add<RemoveRedundantARCPair>(context);

    if (::mlir::failed(::mlir::applyPatternsAndFoldGreedily(
            getOperation(), std::move(patterns)))) {
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
