// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/context.h"
#include <string.h>
sk_status sk_context_init(sk_context *c, size_t capacity, uint64_t capabilities) {
    sk_status s;
    if (!c) return SK_ERR_INVALID_ARGUMENT;
    memset(c,0,sizeof(*c));
    s=sk_arena_init(&c->arena,capacity);
    if (s != SK_OK) return s;
    sk_kernel_registry_init(&c->kernels);
    c->capabilities=capabilities; c->state=1u;
    return SK_OK;
}
void sk_context_destroy(sk_context *c) {
    if (!c) return;
    sk_arena_destroy(&c->arena); memset(c,0,sizeof(*c));
}
