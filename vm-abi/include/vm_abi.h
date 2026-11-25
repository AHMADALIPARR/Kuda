// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef VM_ABI_H
#define VM_ABI_H

#include <stdint.h>
#include <stddef.h>
#include <stdalign.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t vm_cap_mask_t;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint64_t entry;
    uint64_t size;
    uint32_t flags;
    vm_cap_mask_t caps;
    uint32_t reserved[2];
} vm_module_hdr_t;

typedef struct {
    uint64_t pc;
    uint64_t sp;
    uint64_t regs[16];
    vm_cap_mask_t caps;
    uint32_t status;
    uint64_t scratch;
} vm_exec_ctx_t;

typedef struct {
    uint64_t timestamp;
    uint32_t event;
    uint32_t arg;
    uint64_t pc;
} vm_trace_rec_t;

typedef struct {
    uint64_t base;
    uint64_t size;
    uint32_t prot;
    uint32_t flags;
} vm_region_t;

_Static_assert(sizeof(vm_module_hdr_t) == 40, "vm_module_hdr_t size");
_Static_assert(sizeof(vm_exec_ctx_t) == 160, "vm_exec_ctx_t size");
_Static_assert(sizeof(vm_trace_rec_t) == 24, "vm_trace_rec_t size");
_Static_assert(alignof(vm_region_t) == 8, "vm_region_t align");
_Static_assert(alignof(vm_module_hdr_t) == 8, "vm_module_hdr_t align");
_Static_assert(alignof(vm_exec_ctx_t) == 8, "vm_exec_ctx_t align");
_Static_assert(alignof(vm_trace_rec_t) == 8, "vm_trace_rec_t align");

#ifdef __cplusplus
}
#endif

#endif
