// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef SK_CONTEXT_H
#define SK_CONTEXT_H
#include <stddef.h>
#include <stdint.h>
#include "memory.h"
#include "kernel.h"
typedef struct sk_context {
    sk_arena arena;
    sk_kernel_registry kernels;
    uint64_t capabilities;
    uint64_t dispatch_count;
    uint32_t state;
} sk_context;
sk_status sk_context_init(sk_context *context, size_t arena_capacity,
                          uint64_t capabilities);
void sk_context_destroy(sk_context *context);
#endif
