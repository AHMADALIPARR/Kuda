// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../../ffi/bridge.h"

int main(void) {
    rf_machine_t vm;
    int32_t target = 0;

    /* Fuel = 1 allows exactly one transition, then terminates */
    rf_init(&vm, 1);
    assert(rf_register_edge(&vm, 5, 10, RF_PHASE_PENDING, RF_PHASE_VALIDATED) == RF_OK);
    assert(rf_register_edge(&vm, 10, 20, RF_PHASE_VALIDATED, RF_PHASE_COMMITTED) == RF_OK);

    assert(rf_step(&vm, 5, &target) == RF_OK);
    assert(vm.fuel == 0);

    /* Second transition must fail with RF_ERR_NO_FUEL */
    assert(rf_step(&vm, 10, &target) == RF_ERR_NO_FUEL);
    assert(vm.phase == RF_PHASE_VALIDATED);

    printf("termination: PASS\n");
    return 0;
}
