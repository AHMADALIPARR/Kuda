// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/memory.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static int is_power_of_two(size_t x) { return x != 0u && (x & (x - 1u)) == 0u; }
sk_status sk_arena_init(sk_arena *a, size_t capacity) {
    if (!a || capacity == 0u) return SK_ERR_INVALID_ARGUMENT;
    a->base = (unsigned char *)malloc(capacity);
    if (!a->base) { a->capacity=0u; a->used=0u; return SK_ERR_OUT_OF_MEMORY; }
    a->capacity=capacity; a->used=0u; return SK_OK;
}
void sk_arena_destroy(sk_arena *a) {
    if (!a) return;
    free(a->base); a->base=0; a->capacity=0u; a->used=0u;
}
void *sk_arena_alloc(sk_arena *a, size_t size, size_t alignment) {
    uintptr_t address, aligned;
    size_t padding;
    if (!a || !a->base || size == 0u || !is_power_of_two(alignment)) return 0;
    address = (uintptr_t)(a->base + a->used);
    aligned = (address + (uintptr_t)(alignment - 1u)) & ~(uintptr_t)(alignment - 1u);
    padding = (size_t)(aligned - address);
    if (padding > a->capacity - a->used) return 0;
    if (size > a->capacity - a->used - padding) return 0;
    a->used += padding + size;
    return (void *)aligned;
}
sk_arena_mark sk_arena_get_mark(const sk_arena *a) {
    sk_arena_mark m = {0u}; if (a) m.used=a->used; return m;
}
sk_status sk_arena_rewind(sk_arena *a, sk_arena_mark m) {
    if (!a || m.used > a->used) return SK_ERR_INVALID_ARGUMENT;
    if (a->base && a->used > m.used) memset(a->base + m.used, 0, a->used - m.used);
    a->used=m.used; return SK_OK;
}
size_t sk_arena_remaining(const sk_arena *a) {
    return (!a || a->used > a->capacity) ? 0u : a->capacity-a->used;
}
