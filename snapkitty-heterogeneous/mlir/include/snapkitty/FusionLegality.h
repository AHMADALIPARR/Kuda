// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// mlir/include/snapkitty/FusionLegality.h

#ifndef SNAPKITTY_FUSION_LEGALITY_H
#define SNAPKITTY_FUSION_LEGALITY_H

#include "mlir/IR/Operation.h"

namespace snapkitty {

bool hasMemoryEffects(mlir::Operation *operation);

bool fusionCandidate(
    mlir::Operation *producer,
    mlir::Operation *consumer
);

}

#endif
