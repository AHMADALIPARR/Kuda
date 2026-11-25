// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// mlir/include/snapkitty/Passes.h

#ifndef SNAPKITTY_PASSES_H
#define SNAPKITTY_PASSES_H

#include "mlir/Pass/Pass.h"

#include <memory>

namespace snapkitty {

std::unique_ptr<mlir::Pass> createFusionPass();

}

#endif
