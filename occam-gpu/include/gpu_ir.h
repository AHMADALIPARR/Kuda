// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// FILE: include/gpu_ir.h
#ifndef GPU_IR_H
#define GPU_IR_H

#include <stdint.h>
#include <stddef.h>
#include "occam.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPU_IR_MAGIC 0x47505549u /* 'GPUI' */
#define GPU_IR_VERSION 0x00010000u
#define GPU_MAX_VALUES 4096
#define GPU_MAX_INSTRS 8192
#define GPU_MAX_BLOCKS 512
#define GPU_MAX_KERNELS 64
#define GPU_MAX_WG_DIM 1024

typedef enum {
    GTY_VOID = 0,
    GTY_I32,
    GTY_I64,
    GTY_F32,
    GTY_F64,
    GTY_PTR,
    GTY_VEC4_F32,
    GTY_VEC4_I32,
    GTY_CHAN
} gpu_type_t;

typedef enum {
    GOP_NOP = 0,
    GOP_CONST,
    GOP_LOAD,
    GOP_STORE,
    GOP_ADD,
    GOP_SUB,
    GOP_MUL,
    GOP_DIV,
    GOP_MAD,
    GOP_CMP,
    GOP_SELECT,
    GOP_PHI,
    GOP_BR,
    GOP_COND_BR,
    GOP_CALL,
    GOP_RET,
    GOP_BARRIER,
    GOP_CHAN_SEND,
    GOP_CHAN_RECV,
    GOP_WORKITEM_ID,
    GOP_WORKGROUP_ID,
    GOP_GRID_DIM
} gpu_opcode_t;

typedef struct gpu_value {
    uint32_t id;
    gpu_type_t ty;
    uint32_t def_instr;
    uint32_t n_uses;
    uint32_t flags;
} gpu_value_t;

typedef struct gpu_instr {
    uint32_t id;
    gpu_opcode_t op;
    gpu_type_t ty;
    uint32_t dst;
    uint32_t src0;
    uint32_t src1;
    uint32_t src2;
    int64_t imm;
    uint32_t flags;
} gpu_instr_t;

typedef struct gpu_block {
    uint32_t id;
    uint32_t n_instrs;
    uint32_t instr_start;
    uint32_t pred0;
    uint32_t pred1;
    uint32_t succ0;
    uint32_t succ1;
    uint32_t flags;
} gpu_block_t;

typedef struct gpu_kernel {
    uint32_t id;
    uint32_t name_hash;
    uint32_t n_blocks;
    uint32_t block_start;
    uint32_t n_params;
    uint32_t param_types[8];
    uint32_t wg_x, wg_y, wg_z;
    uint32_t grid_x, grid_y, grid_z;
    uint32_t shared_bytes;
    uint32_t flags;
} gpu_kernel_t;

typedef struct gpu_module {
    uint32_t magic;
    uint32_t version;
    uint32_t n_values;
    gpu_value_t *values;
    uint32_t n_instrs;
    gpu_instr_t *instrs;
    uint32_t n_blocks;
    gpu_block_t *blocks;
    uint32_t n_kernels;
    gpu_kernel_t *kernels;
    uint64_t abi_hash;
    uint64_t proof_hash;
} gpu_module_t;

gpu_module_t *gpu_ir_create(void);
void gpu_ir_destroy(gpu_module_t *m);
int gpu_ir_from_occam(gpu_module_t *m, const occam_module_t *oc);
int gpu_ir_verify(const gpu_module_t *m);
uint64_t gpu_ir_hash(const gpu_module_t *m);

#ifdef __cplusplus
}
#endif

#endif
