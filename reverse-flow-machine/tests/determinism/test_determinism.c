// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../../ffi/bridge.h"

int main(void) {
    rf_machine_t vm;
    int32_t target = 0;

    rf_init(&vm, 10);
    assert(rf_register_edge(&vm, 5, 10, RF_PHASE_PENDING, RF_PHASE_VALIDATED) == RF_OK);
    assert(rf_register_edge(&vm, 10, 20, RF_PHASE_VALIDATED, RF_PHASE_COMMITTED) == RF_OK);
    assert(rf_register_edge(&vm, 10, 30, RF_PHASE_VALIDATED, RF_PHASE_REJECTED) == RF_OK);

    /* Step 1: 5 -> 10 (unique enabled edge) */
    assert(rf_step(&vm, 5, &target) == RF_OK);
    assert(target == 10);
    assert(vm.phase == RF_PHASE_VALIDATED);

    /* Step 2: 10 has both 20 and 30 enabled -> |E(10, sigma)| == 2 -> ambiguous */
    assert(rf_step(&vm, 10, &target) == RF_ERR_AMBIGUOUS);
    assert(vm.phase == RF_PHASE_VALIDATED);

    /* Abstain 30 -> |E(10, sigma)| == 1 -> deterministic dispatch to 20 */
    assert(rf_abstain(&vm, 30) == RF_OK);
    assert(rf_step(&vm, 10, &target) == RF_OK);
    assert(target == 20);
    assert(vm.phase == RF_PHASE_COMMITTED);

    printf("determinism: PASS\n");
    return 0;
}
