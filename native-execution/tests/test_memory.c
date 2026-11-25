// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/memory.h"
#include <stdint.h>
int main(void) {
    sk_arena a; sk_arena_mark m; void *p;
    /* Do not put calls inside assert(): Release -DNDEBUG compiles them away; that left the arena uninitialized and segfaulted. */
    if (sk_arena_init(&a,256u)!=SK_OK) return 1;
    m=sk_arena_get_mark(&a);
    p=sk_arena_alloc(&a,32u,16u);
    if (p == 0 || ((uintptr_t)p % 16u)!=0u) return 1;
    if (sk_arena_rewind(&a,m)!=SK_OK) return 1;
    if (sk_arena_remaining(&a)!=256u) return 1;
    sk_arena_destroy(&a);
    return 0;
}
