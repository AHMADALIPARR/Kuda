// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef SK_MEMORY_H
#define SK_MEMORY_H
#include <stddef.h>
#include <stdint.h>
#include "status.h"
typedef struct sk_arena {
    unsigned char *base;
    size_t capacity;
    size_t used;
} sk_arena;
typedef struct sk_arena_mark { size_t used; } sk_arena_mark;
sk_status sk_arena_init(sk_arena *arena, size_t capacity);
void sk_arena_destroy(sk_arena *arena);
void *sk_arena_alloc(sk_arena *arena, size_t size, size_t alignment);
sk_arena_mark sk_arena_get_mark(const sk_arena *arena);
sk_status sk_arena_rewind(sk_arena *arena, sk_arena_mark mark);
size_t sk_arena_remaining(const sk_arena *arena);
#endif
