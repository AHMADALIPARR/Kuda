// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// FILE: src/instr_select.c
#include "backend.h"
#include "gpu_ir.h"
#include <string.h>

/* Instruction selection: GPU IR -> target-specific patterns.
   Produces selection decisions consumed by the emitters. */

typedef struct {
    gpu_opcode_t src_op;
    uint32_t target_opcode; /* backend-specific */
    uint32_t flags;
} isel_rule_t;

static const isel_rule_t ptx_rules[] = {
    { GOP_ADD, 0x01, 0 }, /* add.s32 / add.f32 */
    { GOP_MUL, 0x02, 0 },
    { GOP_MAD, 0x03, 0 },
    { GOP_LOAD, 0x10, 0 },
    { GOP_STORE, 0x11, 0 },
    { GOP_BARRIER, 0x20, 0 },
    { GOP_CHAN_SEND,0x30, 0 },
    { GOP_CHAN_RECV,0x31, 0 },
    { GOP_WORKITEM_ID, 0x40, 0 },
    { GOP_NOP, 0x00, 0 },
};

static const isel_rule_t amdgpu_rules[] = {
    { GOP_ADD, 0x100, 0 }, /* v_add_* */
    { GOP_MUL, 0x101, 0 },
    { GOP_MAD, 0x102, 0 },
    { GOP_LOAD, 0x110, 0 },
    { GOP_STORE, 0x111, 0 },
    { GOP_BARRIER, 0x120, 0 },
    { GOP_WORKITEM_ID, 0x140, 0 },
    { GOP_NOP, 0x000, 0 },
};

/* Deterministic rule lookup: linear scan over the backend's table.
   The tables are small and unsorted, so a scan in table order is the
   deterministic mapping from a GPU IR opcode to its target opcode.
   Returns NULL when the backend has no rule for the opcode. */
static const isel_rule_t *isel_lookup(const isel_rule_t *rules,
                                      size_t n_rules, gpu_opcode_t op)
{
    for (size_t i = 0; i < n_rules; i++) {
        if (rules[i].src_op == op)
            return &rules[i];
    }
    return NULL;
}

/* A value operand is valid when it names a value that exists in the
   module. UINT32_MAX is the "no value" sentinel, never a value id. */
static int isel_value_ok(const gpu_module_t *m, uint32_t id)
{
    return id != UINT32_MAX && id < m->n_values;
}

/* Validate the operands of one instruction against the rule selected
   for it. Operand roles are backend-independent and follow gpu_ir.h /
   the lowering in gpu_ir.c:
   - binary arithmetic (ADD/MUL): dst, src0, src1 are value ids;
     MAD additionally takes src2.
   - LOAD: dst is the loaded value, src0 the address value.
   - STORE: no result; src0 is the address value, src1 the stored value.
   - BARRIER / NOP: no operands; dst must be the "no value" sentinel.
   - CHAN_SEND: no result; src0 is a channel id (not a value id),
     src1 is the value being sent.
   - CHAN_RECV: dst is the received value; src0 is a channel id
     (not a value id).
   - WORKITEM_ID: dst is the id value; no source operands. */
static int isel_validate_operands(const gpu_module_t *m,
                                  const gpu_instr_t *in,
                                  const isel_rule_t *rule)
{
    if (rule->src_op != in->op)
        return -1;

    switch (in->op) {
    case GOP_ADD:
    case GOP_MUL:
        return (isel_value_ok(m, in->dst) &&
                isel_value_ok(m, in->src0) &&
                isel_value_ok(m, in->src1)) ? 0 : -1;
    case GOP_MAD:
        return (isel_value_ok(m, in->dst) &&
                isel_value_ok(m, in->src0) &&
                isel_value_ok(m, in->src1) &&
                isel_value_ok(m, in->src2)) ? 0 : -1;
    case GOP_LOAD:
        return (isel_value_ok(m, in->dst) &&
                isel_value_ok(m, in->src0)) ? 0 : -1;
    case GOP_STORE:
        return (in->dst == UINT32_MAX &&
                isel_value_ok(m, in->src0) &&
                isel_value_ok(m, in->src1)) ? 0 : -1;
    case GOP_BARRIER:
    case GOP_NOP:
        return (in->dst == UINT32_MAX) ? 0 : -1;
    case GOP_CHAN_SEND:
        return (in->dst == UINT32_MAX &&
                isel_value_ok(m, in->src1)) ? 0 : -1;
    case GOP_CHAN_RECV:
        return isel_value_ok(m, in->dst) ? 0 : -1;
    case GOP_WORKITEM_ID:
        return isel_value_ok(m, in->dst) ? 0 : -1;
    default:
        return -1;
    }
}

/* Select every instruction in the module against the backend's rule
   table. An opcode with no rule, an opcode outside the gpu_opcode_t
   range, or an instruction whose operands fail validation is rejected
   (-1) instead of being silently skipped or silently emitted as a
   nop/comment by the backend emitter. */
static int isel_select_module(const gpu_module_t *m,
                              const isel_rule_t *rules, size_t n_rules)
{
    if (!m || !rules || n_rules == 0)
        return -1;
    if (m->n_instrs > 0 && !m->instrs)
        return -1;

    for (uint32_t i = 0; i < m->n_instrs; i++) {
        const gpu_instr_t *in = &m->instrs[i];
        const isel_rule_t *rule;

        if (in->op < GOP_NOP || in->op > GOP_GRID_DIM)
            return -1;
        rule = isel_lookup(rules, n_rules, in->op);
        if (!rule)
            return -1;
        if (isel_validate_operands(m, in, rule) != 0)
            return -1;
    }
    return 0;
}

int backend_select_and_emit(const gpu_module_t *m, const backend_opts_t *opt,
                            void *buf, size_t bufsz, size_t *out_len)
{
    if (!m || !opt || !buf || !out_len) return -1;
    if (gpu_ir_verify(m) != 0) return -1;

    switch (opt->kind) {
    case BACKEND_PTX:
    case BACKEND_SASS:
        /* SASS targets the same NVIDIA ISA family as PTX, so it
           selects through the PTX rule table. */
        if (isel_select_module(m, ptx_rules,
                               sizeof(ptx_rules) / sizeof(ptx_rules[0])) != 0)
            return -1;
        return backend_emit_ptx(m, opt, (char *)buf, bufsz, out_len);
    case BACKEND_AMDGPU:
        if (isel_select_module(m, amdgpu_rules,
                               sizeof(amdgpu_rules) / sizeof(amdgpu_rules[0])) != 0)
            return -1;
        return backend_emit_amdgpu(m, opt, (uint8_t *)buf, bufsz, out_len);
    case BACKEND_SPIRV:
        return backend_emit_spirv(m, opt, (uint32_t *)buf, bufsz / 4, out_len);
    default:
        return -1;
    }
}
