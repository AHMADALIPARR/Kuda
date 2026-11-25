// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef VM_ABI_H
#define VM_ABI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VM_ABI_VERSION 0x00010001u
#define VM_MODULE_MAGIC 0x564D4D4Fu /* 'VMMO' */
#define VM_PAGE_SIZE 4096u
#define VM_ALIGN16 16u
#define VM_MAX_REGIONS 64u
#define VM_MAX_CTX 32u
#define VM_TRACE_RING_SIZE 4096u
#define VM_CAP_RING0 (1ull << 0)
#define VM_CAP_MEM (1ull << 1)
#define VM_CAP_C11 (1ull << 2)
#define VM_CAP_JIT (1ull << 3)
#define VM_CAP_TRACE (1ull << 4)
#define VM_CAP_REPL (1ull << 5)
#define VM_CAP_ALL (0x3Full)

typedef uint64_t vm_cap_mask_t;
typedef int32_t vm_err_t;

enum {
    VM_OK = 0,
    VM_ERR_INVAL = -1,
    VM_ERR_NOMEM = -2,
    VM_ERR_PERM = -3,
    VM_ERR_ALIGN = -4,
    VM_ERR_HASH = -5,
    VM_ERR_MAGIC = -6,
    VM_ERR_VERSION = -7,
    VM_ERR_CAP = -8,
    VM_ERR_STATE = -9,
    VM_ERR_FAULT = -10
};

enum vm_state {
    VM_ST_INIT = 0,
    VM_ST_ADMITTED = 1,
    VM_ST_RUNNING = 2,
    VM_ST_HALTED = 3,
    VM_ST_FAULT = 4
};

typedef struct vm_region {
    uint8_t *base;
    uint64_t size;
    uint64_t used;
    uint32_t flags;
    uint32_t _pad;
} vm_region_t;

typedef struct vm_module_hdr {
    uint32_t magic;
    uint32_t version;
    uint64_t abi_hash;
    uint64_t proof_hash;
    vm_cap_mask_t caps;
    uint64_t entry_off;
    uint64_t code_size;
    uint32_t n_regions;
    uint32_t _pad;
} vm_module_hdr_t;

typedef struct vm_exec_ctx {
    uint64_t rax, rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip;
    uint64_t rflags;
    uint32_t state;
    uint32_t priv_ring;
    vm_cap_mask_t caps;
    uint64_t _pad;
} vm_exec_ctx_t;

typedef struct vm_trace_rec {
    uint64_t tsc;
    uint64_t rip;
    uint32_t op;
    uint32_t ctx_id;
    uint64_t data0;
    uint64_t data1;
} vm_trace_rec_t;

_Static_assert(sizeof(vm_region_t) == 32, "vm_region_t size");
_Static_assert(sizeof(vm_module_hdr_t) == 64, "vm_module_hdr_t size");
_Static_assert(sizeof(vm_exec_ctx_t) == 128, "vm_exec_ctx_t size");
_Static_assert(sizeof(vm_trace_rec_t) == 48, "vm_trace_rec_t size");
_Static_assert(_Alignof(vm_region_t) >= 16, "vm_region_t align");
_Static_assert(_Alignof(vm_module_hdr_t) >= 16, "vm_module_hdr_t align");
_Static_assert(_Alignof(vm_exec_ctx_t) >= 16, "vm_exec_ctx_t align");
_Static_assert(_Alignof(vm_trace_rec_t) >= 16, "vm_trace_rec_t align");

#ifdef __cplusplus
}
#endif

#endif /* VM_ABI_H */
