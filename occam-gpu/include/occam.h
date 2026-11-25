// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef OCCAM_H
#define OCCAM_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OCCAM_MAX_PROCS 256
#define OCCAM_MAX_CHANS 512
#define OCCAM_MAX_PARAMS 32

typedef enum {
    OCC_SKIP = 0,
    OCC_STOP,
    OCC_ASSIGN,
    OCC_INPUT,
    OCC_OUTPUT,
    OCC_SEQ,
    OCC_PAR,
    OCC_ALT,
    OCC_WHILE,
    OCC_IF,
    OCC_CALL,
    OCC_REPL_PAR
} occam_op_t;

typedef struct occam_expr {
    uint32_t kind;
    int32_t val;
    uint32_t var_id;
    struct occam_expr *lhs;
    struct occam_expr *rhs;
} occam_expr_t;

typedef struct occam_proc {
    occam_op_t op;
    uint32_t id;
    uint32_t n_children;
    struct occam_proc **children;
    occam_expr_t *expr;
    uint32_t chan_id;
    uint32_t var_id;
    int32_t const_val;
    uint32_t repl_count;
} occam_proc_t;

typedef struct csp_chan {
    uint32_t id;
    uint32_t elem_type;
    uint32_t capacity;
    uint32_t flags;
} csp_chan_t;

typedef struct occam_module {
    uint32_t n_procs;
    occam_proc_t *procs[OCCAM_MAX_PROCS];
    uint32_t n_chans;
    csp_chan_t chans[OCCAM_MAX_CHANS];
    uint32_t entry_id;
} occam_module_t;

occam_module_t *occam_parse(const char *src, size_t len);
void occam_free(occam_module_t *m);
int occam_validate_csp(const occam_module_t *m);

#ifdef __cplusplus
}
#endif

#endif
