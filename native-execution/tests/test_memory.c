// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/memory.h"
#include <assert.h>
#include <stdint.h>
int main(void) {
    sk_arena a; sk_arena_mark m; void *p;
    assert(sk_arena_init(&a,256u)==SK_OK);
    m=sk_arena_get_mark(&a);
    p=sk_arena_alloc(&a,32u,16u);
    assert(p != 0 && ((uintptr_t)p % 16u)==0u);
    assert(sk_arena_rewind(&a,m)==SK_OK);
    assert(sk_arena_remaining(&a)==256u);
    sk_arena_destroy(&a);
    return 0;
}
