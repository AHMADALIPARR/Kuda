// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include <assert.h>
#include <stdio.h>
#include "../../ffi/bridge.h"

int main(void) {
    rf_machine_t vm;
    int32_t target = 0;
    rf_phase_t replayed_phase = RF_PHASE_PENDING;
    int32_t replayed_fuel = 0;

    rf_init(&vm, 100);
    assert(rf_register_edge(&vm, 5, 10, RF_PHASE_PENDING, RF_PHASE_VALIDATED) == RF_OK);
    assert(rf_register_edge(&vm, 10, 20, RF_PHASE_VALIDATED, RF_PHASE_COMMITTED) == RF_OK);

    assert(rf_step(&vm, 5, &target) == RF_OK);
    assert(rf_step(&vm, 10, &target) == RF_OK);
    assert(vm.trace_count == 2);

    assert(rf_replay_verify(&vm, &replayed_phase, &replayed_fuel) == RF_OK);
    assert(replayed_phase == RF_PHASE_COMMITTED);
    assert(replayed_fuel == 98);

    printf("replay: PASS\n");
    return 0;
}
