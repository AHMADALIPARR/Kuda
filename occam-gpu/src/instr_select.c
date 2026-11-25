// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
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

int backend_select_and_emit(const gpu_module_t *m, const backend_opts_t *opt,
                            void *buf, size_t bufsz, size_t *out_len)
{
    if (!m || !opt || !buf || !out_len) return -1;
    if (gpu_ir_verify(m) != 0) return -1;

    switch (opt->kind) {
    case BACKEND_PTX:
    case BACKEND_SASS:
        return backend_emit_ptx(m, opt, (char *)buf, bufsz, out_len);
    case BACKEND_AMDGPU:
        return backend_emit_amdgpu(m, opt, (uint8_t *)buf, bufsz, out_len);
    case BACKEND_SPIRV:
        return backend_emit_spirv(m, opt, (uint32_t *)buf, bufsz / 4, out_len);
    default:
        return -1;
    }
}
