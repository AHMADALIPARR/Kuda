// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// mlir/fusion/fusion_legality.cpp

#include "snapkitty/FusionLegality.h"

#include "mlir/Interfaces/SideEffectInterfaces.h"

namespace snapkitty {

bool hasMemoryEffects(mlir::Operation *operation)
{
    if (operation == nullptr)
        return true;

    auto interface =
        mlir::dyn_cast<mlir::MemoryEffectOpInterface>(operation);

    if (!interface)
        return true;

    return !interface.hasNoEffect();
}

bool fusionCandidate(
    mlir::Operation *producer,
    mlir::Operation *consumer
)
{
    if (producer == nullptr || consumer == nullptr)
        return false;

    if (producer->getBlock() != consumer->getBlock())
        return false;

    if (producer->getNumRegions() != 0 ||
        consumer->getNumRegions() != 0)
        return false;

    if (producer->hasTrait<mlir::OpTrait::IsTerminator>() ||
        consumer->hasTrait<mlir::OpTrait::IsTerminator>())
        return false;

    if (hasMemoryEffects(producer) ||
        hasMemoryEffects(consumer))
        return false;

    for (mlir::Value result : producer->getResults()) {
        for (mlir::Operation *user : result.getUsers()) {
            if (user != consumer)
                return false;
        }
    }

    return true;
}

}
