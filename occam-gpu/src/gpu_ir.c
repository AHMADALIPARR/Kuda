// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "gpu_ir.h"
#include <stdlib.h>
#include <string.h>

#define FNV_OFFSET 14695981039346656037ull
#define FNV_PRIME 1099511628211ull

static uint64_t fnv1a(const void *data, size_t len)
{
    const uint8_t *p = data;
    uint64_t h = FNV_OFFSET;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= FNV_PRIME;
    }
    return h;
}

gpu_module_t *gpu_ir_create(void)
{
    gpu_module_t *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    m->magic = GPU_IR_MAGIC;
    m->version = GPU_IR_VERSION;
    m->values = calloc(GPU_MAX_VALUES, sizeof(gpu_value_t));
    m->instrs = calloc(GPU_MAX_INSTRS, sizeof(gpu_instr_t));
    m->blocks = calloc(GPU_MAX_BLOCKS, sizeof(gpu_block_t));
    m->kernels = calloc(GPU_MAX_KERNELS, sizeof(gpu_kernel_t));
    if (!m->values || !m->instrs || !m->blocks || !m->kernels) {
        gpu_ir_destroy(m);
        return NULL;
    }
    return m;
}

void gpu_ir_destroy(gpu_module_t *m)
{
    if (!m) return;
    free(m->values);
    free(m->instrs);
    free(m->blocks);
    free(m->kernels);
    free(m);
}

static uint32_t add_value(gpu_module_t *m, gpu_type_t ty)
{
    if (m->n_values >= GPU_MAX_VALUES) return UINT32_MAX;
    uint32_t id = m->n_values++;
    m->values[id].id = id;
    m->values[id].ty = ty;
    m->values[id].def_instr = UINT32_MAX;
    return id;
}

static uint32_t add_instr(gpu_module_t *m, gpu_opcode_t op, gpu_type_t ty,
                          uint32_t dst, uint32_t s0, uint32_t s1, int64_t imm)
{
    if (m->n_instrs >= GPU_MAX_INSTRS) return UINT32_MAX;
    uint32_t id = m->n_instrs++;
    m->instrs[id].id = id;
    m->instrs[id].op = op;
    m->instrs[id].ty = ty;
    m->instrs[id].dst = dst;
    m->instrs[id].src0 = s0;
    m->instrs[id].src1 = s1;
    m->instrs[id].imm = imm;
    if (dst != UINT32_MAX && dst < m->n_values)
        m->values[dst].def_instr = id;
    return id;
}

static int lower_proc(gpu_module_t *m, const occam_proc_t *p, uint32_t *entry_block)
{
    if (!p) return -1;
    switch (p->op) {
    case OCC_SKIP:
        *entry_block = m->n_blocks;
        if (m->n_blocks >= GPU_MAX_BLOCKS) return -1;
        m->blocks[m->n_blocks].id = m->n_blocks;
        m->blocks[m->n_blocks].instr_start = m->n_instrs;
        add_instr(m, GOP_NOP, GTY_VOID, UINT32_MAX, 0, 0, 0);
        m->blocks[m->n_blocks].n_instrs = 1;
        m->n_blocks++;
        return 0;
    case OCC_PAR: {
        /* Map PAR to concurrent workitems / workgroups */
        uint32_t kid = m->n_kernels;
        if (kid >= GPU_MAX_KERNELS) return -1;
        m->kernels[kid].id = kid;
        m->kernels[kid].wg_x = p->n_children ? p->n_children : 1;
        m->kernels[kid].wg_y = 1;
        m->kernels[kid].wg_z = 1;
        m->kernels[kid].block_start = m->n_blocks;
        for (uint32_t i = 0; i < p->n_children; i++) {
            uint32_t b;
            if (lower_proc(m, p->children[i], &b) != 0) return -1;
        }
        m->kernels[kid].n_blocks = m->n_blocks - m->kernels[kid].block_start;
        m->n_kernels++;
        *entry_block = m->kernels[kid].block_start;
        return 0;
    }
    case OCC_SEQ: {
        uint32_t first = UINT32_MAX;
        for (uint32_t i = 0; i < p->n_children; i++) {
            uint32_t b;
            if (lower_proc(m, p->children[i], &b) != 0) return -1;
            if (first == UINT32_MAX) first = b;
        }
        *entry_block = first;
        return 0;
    }
    case OCC_INPUT:
    case OCC_OUTPUT: {
        *entry_block = m->n_blocks;
        if (m->n_blocks >= GPU_MAX_BLOCKS) return -1;
        m->blocks[m->n_blocks].id = m->n_blocks;
        m->blocks[m->n_blocks].instr_start = m->n_instrs;
        uint32_t v = add_value(m, GTY_I32);
        if (p->op == OCC_INPUT)
            add_instr(m, GOP_CHAN_RECV, GTY_I32, v, p->chan_id, 0, 0);
        else
            add_instr(m, GOP_CHAN_SEND, GTY_VOID, UINT32_MAX, p->chan_id, v, 0);
        m->blocks[m->n_blocks].n_instrs = 1;
        m->n_blocks++;
        return 0;
    }
    default:
        *entry_block = m->n_blocks;
        if (m->n_blocks >= GPU_MAX_BLOCKS) return -1;
        m->blocks[m->n_blocks].id = m->n_blocks;
        m->blocks[m->n_blocks].instr_start = m->n_instrs;
        add_instr(m, GOP_NOP, GTY_VOID, UINT32_MAX, 0, 0, 0);
        m->blocks[m->n_blocks].n_instrs = 1;
        m->n_blocks++;
        return 0;
    }
}

int gpu_ir_from_occam(gpu_module_t *m, const occam_module_t *oc)
{
    if (!m || !oc || oc->n_procs == 0) return -1;
    uint32_t entry;
    if (lower_proc(m, oc->procs[oc->entry_id], &entry) != 0) return -1;
    m->abi_hash = gpu_ir_hash(m);
    return 0;
}

int gpu_ir_verify(const gpu_module_t *m)
{
    if (!m || m->magic != GPU_IR_MAGIC || m->version != GPU_IR_VERSION)
        return -1;
    if (m->n_values > GPU_MAX_VALUES || m->n_instrs > GPU_MAX_INSTRS)
        return -1;
    for (uint32_t i = 0; i < m->n_kernels; i++) {
        const gpu_kernel_t *k = &m->kernels[i];
        if (k->wg_x == 0 || k->wg_x > GPU_MAX_WG_DIM) return -1;
        if (k->wg_y > GPU_MAX_WG_DIM || k->wg_z > GPU_MAX_WG_DIM) return -1;
    }
    /* SSA & type checks simplified */
    for (uint32_t i = 0; i < m->n_instrs; i++) {
        const gpu_instr_t *in = &m->instrs[i];
        if (in->dst != UINT32_MAX && in->dst >= m->n_values) return -1;
    }
    return 0;
}

uint64_t gpu_ir_hash(const gpu_module_t *m)
{
    if (!m) return 0;
    uint64_t h = fnv1a(&m->magic, sizeof(m->magic));
    h ^= fnv1a(&m->n_values, sizeof(m->n_values));
    h ^= fnv1a(&m->n_instrs, sizeof(m->n_instrs));
    h ^= fnv1a(&m->n_blocks, sizeof(m->n_blocks));
    h ^= fnv1a(&m->n_kernels, sizeof(m->n_kernels));
    return h;
}
