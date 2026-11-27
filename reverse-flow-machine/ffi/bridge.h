// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef REVERSE_FLOW_BRIDGE_H
#define REVERSE_FLOW_BRIDGE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RF_MAX_EDGES 64
#define RF_MAX_TRACE 256
#define RF_MAX_ABSTAIN 64

typedef enum {
    RF_PHASE_PENDING = 0,
    RF_PHASE_VALIDATED = 1,
    RF_PHASE_COMMITTED = 2,
    RF_PHASE_REJECTED = 3
} rf_phase_t;

typedef enum {
    RF_OK = 0,
    RF_ERR_INADMISSIBLE = -1,
    RF_ERR_AMBIGUOUS = -2,
    RF_ERR_NO_EDGE = -3,
    RF_ERR_NO_FUEL = -4,
    RF_ERR_FULL = -5,
    RF_ERR_REPLAY_MISMATCH = -6
} rf_status_t;

typedef struct {
    int32_t source_label;
    int32_t target_label;
    rf_phase_t required_phase;
    rf_phase_t next_phase;
} rf_edge_t;

typedef struct {
    uint32_t seq;
    int32_t source_label;
    int32_t target_label;
    rf_phase_t phase;
    int32_t fuel;
} rf_trace_entry_t;

typedef struct {
    rf_phase_t phase;
    int32_t fuel;
    uint32_t version;
    rf_edge_t edges[RF_MAX_EDGES];
    size_t edge_count;
    int32_t abstained[RF_MAX_ABSTAIN];
    size_t abstain_count;
    rf_trace_entry_t trace[RF_MAX_TRACE];
    size_t trace_count;
} rf_machine_t;

void rf_init(rf_machine_t *vm, int32_t initial_fuel);
int rf_phase_admissible(rf_phase_t from, rf_phase_t to);
rf_status_t rf_register_edge(rf_machine_t *vm, int32_t src, int32_t tgt,
                             rf_phase_t req, rf_phase_t next);
rf_status_t rf_abstain(rf_machine_t *vm, int32_t target_label);
rf_status_t rf_reinstate(rf_machine_t *vm, int32_t target_label);
int rf_is_abstained(const rf_machine_t *vm, int32_t target_label);
rf_status_t rf_step(rf_machine_t *vm, int32_t src_label, int32_t *out_target);
rf_status_t rf_replay_verify(const rf_machine_t *vm, rf_phase_t *out_phase,
                             int32_t *out_fuel);
const char *rf_phase_name(rf_phase_t phase);

#ifdef __cplusplus
}
#endif

#endif /* REVERSE_FLOW_BRIDGE_H */
