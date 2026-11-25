// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
// FILE: tests/test_c11_abi.c
#include "../include/vm_abi.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

extern vm_err_t vm_mem_init(void *arena, uint64_t size);
extern void *vm_mem_alloc(uint32_t reg, uint64_t size, uint64_t align);
extern vm_err_t vm_mem_seal(uint32_t reg);
extern vm_err_t vm_mem_free_region(uint32_t reg);
extern vm_err_t vm_verify_module(const vm_module_hdr_t *hdr, const void *code, size_t code_len);
extern uint64_t vm_abi_hash(const void *hdr, size_t len);
extern void vm_sha256(const void *data, size_t len, uint8_t out[32]);
extern uint64_t vm_call6(void *fn, uint64_t a0, uint64_t a1, uint64_t a2,
                         uint64_t a3, uint64_t a4, uint64_t a5);

static uint64_t test_add(uint64_t a, uint64_t b, uint64_t c,
                         uint64_t d, uint64_t e, uint64_t f)
{
    return a + b + c + d + e + f;
}

static uint8_t arena[1 << 20] __attribute__((aligned(16)));

int main(void)
{
    vm_err_t e;
    void *p;
    vm_module_hdr_t hdr;
    uint8_t digest[32];
    uint64_t h;

    /* Layout assertions already in header via _Static_assert */

    e = vm_mem_init(arena, sizeof(arena));
    assert(e == VM_OK);

    p = vm_mem_alloc(0, 64, 16);
    assert(p != NULL);
    assert(((uintptr_t)p & 15) == 0);

    p = vm_mem_alloc(0, 128, 32);
    assert(p != NULL);

    e = vm_mem_seal(0);
    assert(e == VM_OK);

    p = vm_mem_alloc(0, 16, 16);
    assert(p == NULL); /* sealed */

    e = vm_mem_free_region(0);
    assert(e == VM_OK);

    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = VM_MODULE_MAGIC;
    hdr.version = VM_ABI_VERSION;
    hdr.caps = VM_CAP_MEM | VM_CAP_C11;
    hdr.abi_hash = vm_abi_hash(&hdr, sizeof(hdr) - sizeof(hdr.proof_hash));
    vm_sha256("testcode", 8, digest);
    memcpy(&hdr.proof_hash, digest, 8);

    e = vm_verify_module(&hdr, "testcode", 8);
    assert(e == VM_OK);

    hdr.magic = 0xDEADBEEF;
    e = vm_verify_module(&hdr, NULL, 0);
    assert(e == VM_ERR_MAGIC);

    hdr.magic = VM_MODULE_MAGIC;
    hdr.version = 0;
    e = vm_verify_module(&hdr, NULL, 0);
    assert(e == VM_ERR_VERSION);

    h = vm_call6((void *)test_add, 1, 2, 3, 4, 5, 6);
    assert(h == 21);

    printf("C11 ABI tests OK\n");
    return 0;
}
