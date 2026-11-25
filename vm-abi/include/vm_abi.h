// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// FILE: include/vm_abi.h
#ifndef VM_ABI_H
#define VM_ABI_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * VM ABI v2 -- VM_ABI_VERSION 0x00020000u. Sizes 32/64/176/48,
 * alignment 16 on all four (v1's 128-byte ctx is impossible: the 16
 * GPRs alone fill 128 bytes). v1 modules and v1-sized context arrays
 * are NOT wire-compatible with v2.
 *
 * Arithmetic (all fields preserved, no field deleted):
 *   region: 8 (base) + 8 (size) + 8 (used) + 4 (flags) + 4 (_pad) = 32.
 *   hdr:    4 + 4 + 8 (abi_hash) + 8 (proof_hash) + 8 (caps) + 8 (entry_off)
 *           + 8 (code_size) + 4 (n_regions) + 4 (_pad) = 56, + 8 _reserved
 *           = 64.
 *   ctx:    16 GPRs x 8 = 128, + 8 (rip) + 8 (rflags) + 4 (state)
 *           + 4 (priv_ring) + 8 (caps) + 8 (_pad) = 168. 168 is not a
 *           multiple of 16 (168 = 16*10 + 8), so + 8 _reserved = 176.
 *   trace:  8 (tsc) + 8 (rip) + 4 (op) + 4 (ctx_id) + 8 (data0)
 *           + 8 (data1) = 40, + 8 _reserved = 48.
 *
 * Alignment is forced with _Alignas(16) on the first member of each
 * struct (_Alignas on the struct tag or a typedef is rejected by gcc).
 * Field order and every pre-existing offset are unchanged; only a
 * trailing `_reserved` (must be zero) was appended where the size fell
 * short of a multiple of 16.
 *
 * Hash domain (substrate-side): c11_substrate.c verifies by copying
 * the header, zeroing both abi_hash (offset 8) and proof_hash
 * (offset 16) in the copy, and hashing the copy over the first
 * sizeof(*hdr) - sizeof(proof_hash) bytes, i.e. v2 bytes 0..55; only
 * the trailing _reserved (56..63) is outside the range.
 */

#define VM_ABI_VERSION 0x00020000u
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
    _Alignas(16) uint8_t *base;
    uint64_t size;
    uint64_t used;
    uint32_t flags;
    uint32_t _pad;
} vm_region_t;

typedef struct vm_module_hdr {
    _Alignas(16) uint32_t magic;
    uint32_t version;
    uint64_t abi_hash;
    uint64_t proof_hash;
    vm_cap_mask_t caps;
    uint64_t entry_off;
    uint64_t code_size;
    uint32_t n_regions;
    uint32_t _pad;
    uint64_t _reserved; /* must be zero; outside the abi_hash input range */
} vm_module_hdr_t;

typedef struct vm_exec_ctx {
    _Alignas(16) uint64_t rax;
    uint64_t rbx, rcx, rdx;
    uint64_t rsi, rdi, rbp, rsp;
    uint64_t r8, r9, r10, r11;
    uint64_t r12, r13, r14, r15;
    uint64_t rip;
    uint64_t rflags;
    uint32_t state;
    uint32_t priv_ring;
    vm_cap_mask_t caps;
    uint64_t _pad;
    uint64_t _reserved; /* must be zero; pads 168 -> 176 (multiple of 16) */
} vm_exec_ctx_t;

typedef struct vm_trace_rec {
    _Alignas(16) uint64_t tsc;
    uint64_t rip;
    uint32_t op;
    uint32_t ctx_id;
    uint64_t data0;
    uint64_t data1;
    uint64_t _reserved; /* must be zero; pads 40 -> 48 (multiple of 16) */
} vm_trace_rec_t;

/* Sizes: v2 contract (ctx was 128 in v1; 128 is impossible, see above). */
_Static_assert(sizeof(vm_region_t) == 32, "vm_region_t size");
_Static_assert(sizeof(vm_module_hdr_t) == 64, "vm_module_hdr_t size");
_Static_assert(sizeof(vm_exec_ctx_t) == 176, "vm_exec_ctx_t size");
_Static_assert(sizeof(vm_trace_rec_t) == 48, "vm_trace_rec_t size");
_Static_assert(_Alignof(vm_region_t) >= 16, "vm_region_t align");
_Static_assert(_Alignof(vm_module_hdr_t) >= 16, "vm_module_hdr_t align");
_Static_assert(_Alignof(vm_exec_ctx_t) >= 16, "vm_exec_ctx_t align");
_Static_assert(_Alignof(vm_trace_rec_t) >= 16, "vm_trace_rec_t align");
_Static_assert(_Alignof(vm_region_t) == 16, "vm_region_t align is 16");
_Static_assert(_Alignof(vm_module_hdr_t) == 16, "vm_module_hdr_t align is 16");
_Static_assert(_Alignof(vm_exec_ctx_t) == 16, "vm_exec_ctx_t align is 16");
_Static_assert(_Alignof(vm_trace_rec_t) == 16, "vm_trace_rec_t align is 16");

/* Field offsets: every field, pinned to the v2 layout. */
_Static_assert(offsetof(vm_region_t, base) == 0, "vm_region_t.base off");
_Static_assert(offsetof(vm_region_t, size) == 8, "vm_region_t.size off");
_Static_assert(offsetof(vm_region_t, used) == 16, "vm_region_t.used off");
_Static_assert(offsetof(vm_region_t, flags) == 24, "vm_region_t.flags off");
_Static_assert(offsetof(vm_region_t, _pad) == 28, "vm_region_t._pad off");

_Static_assert(offsetof(vm_module_hdr_t, magic) == 0, "vm_module_hdr_t.magic off");
_Static_assert(offsetof(vm_module_hdr_t, version) == 4, "vm_module_hdr_t.version off");
_Static_assert(offsetof(vm_module_hdr_t, abi_hash) == 8, "vm_module_hdr_t.abi_hash off");
_Static_assert(offsetof(vm_module_hdr_t, proof_hash) == 16, "vm_module_hdr_t.proof_hash off");
_Static_assert(offsetof(vm_module_hdr_t, caps) == 24, "vm_module_hdr_t.caps off");
_Static_assert(offsetof(vm_module_hdr_t, entry_off) == 32, "vm_module_hdr_t.entry_off off");
_Static_assert(offsetof(vm_module_hdr_t, code_size) == 40, "vm_module_hdr_t.code_size off");
_Static_assert(offsetof(vm_module_hdr_t, n_regions) == 48, "vm_module_hdr_t.n_regions off");
_Static_assert(offsetof(vm_module_hdr_t, _pad) == 52, "vm_module_hdr_t._pad off");
_Static_assert(offsetof(vm_module_hdr_t, _reserved) == 56, "vm_module_hdr_t._reserved off");

_Static_assert(offsetof(vm_exec_ctx_t, rax) == 0, "vm_exec_ctx_t.rax off");
_Static_assert(offsetof(vm_exec_ctx_t, rbx) == 8, "vm_exec_ctx_t.rbx off");
_Static_assert(offsetof(vm_exec_ctx_t, rcx) == 16, "vm_exec_ctx_t.rcx off");
_Static_assert(offsetof(vm_exec_ctx_t, rdx) == 24, "vm_exec_ctx_t.rdx off");
_Static_assert(offsetof(vm_exec_ctx_t, rsi) == 32, "vm_exec_ctx_t.rsi off");
_Static_assert(offsetof(vm_exec_ctx_t, rdi) == 40, "vm_exec_ctx_t.rdi off");
_Static_assert(offsetof(vm_exec_ctx_t, rbp) == 48, "vm_exec_ctx_t.rbp off");
_Static_assert(offsetof(vm_exec_ctx_t, rsp) == 56, "vm_exec_ctx_t.rsp off");
_Static_assert(offsetof(vm_exec_ctx_t, r8) == 64, "vm_exec_ctx_t.r8 off");
_Static_assert(offsetof(vm_exec_ctx_t, r9) == 72, "vm_exec_ctx_t.r9 off");
_Static_assert(offsetof(vm_exec_ctx_t, r10) == 80, "vm_exec_ctx_t.r10 off");
_Static_assert(offsetof(vm_exec_ctx_t, r11) == 88, "vm_exec_ctx_t.r11 off");
_Static_assert(offsetof(vm_exec_ctx_t, r12) == 96, "vm_exec_ctx_t.r12 off");
_Static_assert(offsetof(vm_exec_ctx_t, r13) == 104, "vm_exec_ctx_t.r13 off");
_Static_assert(offsetof(vm_exec_ctx_t, r14) == 112, "vm_exec_ctx_t.r14 off");
_Static_assert(offsetof(vm_exec_ctx_t, r15) == 120, "vm_exec_ctx_t.r15 off");
_Static_assert(offsetof(vm_exec_ctx_t, rip) == 128, "vm_exec_ctx_t.rip off");
_Static_assert(offsetof(vm_exec_ctx_t, rflags) == 136, "vm_exec_ctx_t.rflags off");
_Static_assert(offsetof(vm_exec_ctx_t, state) == 144, "vm_exec_ctx_t.state off");
_Static_assert(offsetof(vm_exec_ctx_t, priv_ring) == 148, "vm_exec_ctx_t.priv_ring off");
_Static_assert(offsetof(vm_exec_ctx_t, caps) == 152, "vm_exec_ctx_t.caps off");
_Static_assert(offsetof(vm_exec_ctx_t, _pad) == 160, "vm_exec_ctx_t._pad off");
_Static_assert(offsetof(vm_exec_ctx_t, _reserved) == 168, "vm_exec_ctx_t._reserved off");

_Static_assert(offsetof(vm_trace_rec_t, tsc) == 0, "vm_trace_rec_t.tsc off");
_Static_assert(offsetof(vm_trace_rec_t, rip) == 8, "vm_trace_rec_t.rip off");
_Static_assert(offsetof(vm_trace_rec_t, op) == 16, "vm_trace_rec_t.op off");
_Static_assert(offsetof(vm_trace_rec_t, ctx_id) == 20, "vm_trace_rec_t.ctx_id off");
_Static_assert(offsetof(vm_trace_rec_t, data0) == 24, "vm_trace_rec_t.data0 off");
_Static_assert(offsetof(vm_trace_rec_t, data1) == 32, "vm_trace_rec_t.data1 off");
_Static_assert(offsetof(vm_trace_rec_t, _reserved) == 40, "vm_trace_rec_t._reserved off");

#ifdef __cplusplus
}
#endif

#endif /* VM_ABI_H */
