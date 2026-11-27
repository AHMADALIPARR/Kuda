// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../../ffi/bridge.h"

int main(void) {
    rf_machine_t vm;
    int32_t target = 0;

    rf_init(&vm, 5);
    /* Inadmissible Pending -> Committed must be rejected */
    assert(rf_register_edge(&vm, 10, 20, RF_PHASE_PENDING, RF_PHASE_COMMITTED) == RF_ERR_INADMISSIBLE);

    assert(rf_register_edge(&vm, 5, 10, RF_PHASE_PENDING, RF_PHASE_VALIDATED) == RF_OK);
    assert(rf_step(&vm, 5, &target) == RF_OK);
    assert(vm.version == 1);
    assert(vm.fuel == 4);

    /* Unmatched source label leaves state and version intact */
    assert(rf_step(&vm, 99, &target) == RF_ERR_NO_EDGE);
    assert(vm.version == 1);
    assert(vm.fuel == 4);
    assert(vm.phase == RF_PHASE_VALIDATED);

    printf("persistence: PASS\n");
    return 0;
}
