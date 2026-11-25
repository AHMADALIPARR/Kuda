// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef SK_KERNEL_H
#define SK_KERNEL_H
#include <stddef.h>
#include <stdint.h>
#include "status.h"
#include "value.h"
#include "contract.h"
struct sk_context;
typedef sk_status (*sk_kernel_fn)(struct sk_context *context,
                                  const sk_value *args, size_t arg_count,
                                  sk_value *result);
typedef struct sk_kernel_desc {
    uint32_t kernel_id;
    uint32_t version;
    const char *name;
    sk_kernel_fn function;
    sk_contract contract;
} sk_kernel_desc;
#define SK_MAX_KERNELS 128u
typedef struct sk_kernel_registry {
    sk_kernel_desc entries[SK_MAX_KERNELS];
    size_t count;
} sk_kernel_registry;
void sk_kernel_registry_init(sk_kernel_registry *registry);
sk_status sk_kernel_register(sk_kernel_registry *registry,
                             const sk_kernel_desc *descriptor);
const sk_kernel_desc *sk_kernel_find(const sk_kernel_registry *registry,
                                     uint32_t kernel_id);
#endif
