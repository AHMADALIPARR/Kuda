// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "bridge.h"

void rf_init(rf_machine_t *vm, int32_t initial_fuel) {
    if (!vm) return;
    vm->phase = RF_PHASE_PENDING;
    vm->fuel = initial_fuel < 0 ? 0 : initial_fuel;
    vm->version = 0;
    vm->edge_count = 0;
    vm->abstain_count = 0;
    vm->trace_count = 0;
}

int rf_phase_admissible(rf_phase_t from, rf_phase_t to) {
    if (from == RF_PHASE_PENDING && to == RF_PHASE_VALIDATED) return 1;
    if (from == RF_PHASE_VALIDATED && to == RF_PHASE_COMMITTED) return 1;
    if (from == RF_PHASE_VALIDATED && to == RF_PHASE_REJECTED) return 1;
    return 0;
}

rf_status_t rf_register_edge(rf_machine_t *vm, int32_t src, int32_t tgt,
                             rf_phase_t req, rf_phase_t next) {
    if (!vm) return RF_ERR_INADMISSIBLE;
    if (!rf_phase_admissible(req, next)) return RF_ERR_INADMISSIBLE;
    if (vm->edge_count >= RF_MAX_EDGES) return RF_ERR_FULL;

    rf_edge_t *e = &vm->edges[vm->edge_count++];
    e->source_label = src;
    e->target_label = tgt;
    e->required_phase = req;
    e->next_phase = next;
    return RF_OK;
}

int rf_is_abstained(const rf_machine_t *vm, int32_t target_label) {
    if (!vm) return 0;
    for (size_t i = 0; i < vm->abstain_count; ++i) {
        if (vm->abstained[i] == target_label) return 1;
    }
    return 0;
}

rf_status_t rf_abstain(rf_machine_t *vm, int32_t target_label) {
    if (!vm) return RF_ERR_INADMISSIBLE;
    if (rf_is_abstained(vm, target_label)) return RF_OK;
    if (vm->abstain_count >= RF_MAX_ABSTAIN) return RF_ERR_FULL;
    vm->abstained[vm->abstain_count++] = target_label;
    return RF_OK;
}

rf_status_t rf_reinstate(rf_machine_t *vm, int32_t target_label) {
    if (!vm) return RF_ERR_INADMISSIBLE;
    for (size_t i = 0; i < vm->abstain_count; ++i) {
        if (vm->abstained[i] == target_label) {
            vm->abstained[i] = vm->abstained[vm->abstain_count - 1];
            vm->abstain_count--;
            return RF_OK;
        }
    }
    return RF_OK;
}

rf_status_t rf_step(rf_machine_t *vm, int32_t src_label, int32_t *out_target) {
    if (!vm) return RF_ERR_INADMISSIBLE;
    if (vm->fuel <= 0) return RF_ERR_NO_FUEL;

    const rf_edge_t *matched = NULL;
    size_t match_count = 0;

    for (size_t i = 0; i < vm->edge_count; ++i) {
        const rf_edge_t *e = &vm->edges[i];
        if (e->source_label != src_label) continue;
        if (rf_is_abstained(vm, e->target_label)) continue;
        if (e->required_phase != vm->phase) continue;
        matched = e;
        match_count++;
    }

    if (match_count == 0) return RF_ERR_NO_EDGE;
    if (match_count > 1) return RF_ERR_AMBIGUOUS;
    if (!rf_phase_admissible(matched->required_phase, matched->next_phase)) {
        return RF_ERR_INADMISSIBLE;
    }
    if (vm->trace_count >= RF_MAX_TRACE) return RF_ERR_FULL;

    /* Atomic commit mirroring MUMPS TSTART / TCOMMIT */
    vm->phase = matched->next_phase;
    vm->fuel -= 1;
    vm->version += 1;

    rf_trace_entry_t *t = &vm->trace[vm->trace_count++];
    t->seq = (uint32_t)vm->trace_count;
    t->source_label = matched->source_label;
    t->target_label = matched->target_label;
    t->phase = vm->phase;
    t->fuel = vm->fuel;

    if (out_target) *out_target = matched->target_label;
    return RF_OK;
}

rf_status_t rf_replay_verify(const rf_machine_t *vm, rf_phase_t *out_phase,
                             int32_t *out_fuel) {
    if (!vm) return RF_ERR_INADMISSIBLE;
    rf_phase_t cur_phase = RF_PHASE_PENDING;
    int32_t prev_fuel = INT32_MAX;

    for (size_t i = 0; i < vm->trace_count; ++i) {
        const rf_trace_entry_t *t = &vm->trace[i];
        if (!rf_phase_admissible(cur_phase, t->phase)) {
            return RF_ERR_REPLAY_MISMATCH;
        }
        if (t->fuel < 0 || (i > 0 && t->fuel >= prev_fuel)) {
            return RF_ERR_REPLAY_MISMATCH;
        }
        cur_phase = t->phase;
        prev_fuel = t->fuel;
    }

    if (out_phase) *out_phase = cur_phase;
    if (out_fuel) *out_fuel = (vm->trace_count > 0) ? prev_fuel : vm->fuel;
    return RF_OK;
}

const char *rf_phase_name(rf_phase_t phase) {
    switch (phase) {
        case RF_PHASE_PENDING:   return "PENDING";
        case RF_PHASE_VALIDATED: return "VALIDATED";
        case RF_PHASE_COMMITTED: return "COMMITTED";
        case RF_PHASE_REJECTED:  return "REJECTED";
        default:                 return "UNKNOWN";
    }
}
