// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// mlir/fusion/fusion_pass.cpp

#include "snapkitty/Passes.h"
#include "snapkitty/FusionLegality.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Pass/Pass.h"

namespace snapkitty {

class FusionPass final
    : public mlir::PassWrapper<
          FusionPass,
          mlir::OperationPass<mlir::ModuleOp>> {

public:
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(FusionPass)

    llvm::StringRef getArgument() const final
    {
        return "snapkitty-fusion";
    }

    llvm::StringRef getDescription() const final
    {
        return "SnapKitty dependency-safe kernel fusion";
    }

    void runOnOperation() final
    {
        mlir::ModuleOp module = getOperation();

        module.walk([&](mlir::Operation *operation) {
            if (operation->getNumResults() == 0)
                return;

            for (mlir::Value result : operation->getResults()) {
                for (mlir::Operation *user : result.getUsers()) {
                    (void)fusionCandidate(operation, user);
                }
            }
        });
    }
};

std::unique_ptr<mlir::Pass> createFusionPass()
{
    return std::make_unique<FusionPass>();
}

}
