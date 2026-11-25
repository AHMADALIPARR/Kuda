// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/kernel.h"
#include <string.h>
void sk_kernel_registry_init(sk_kernel_registry *r) { if (r) { memset(r,0,sizeof(*r)); } }
sk_status sk_kernel_register(sk_kernel_registry *r, const sk_kernel_desc *d) {
    size_t i;
    if (!r || !d || !d->name || !d->function || d->kernel_id == 0u)
        return SK_ERR_INVALID_ARGUMENT;
    for (i=0u; i<r->count; ++i)
        if (r->entries[i].kernel_id == d->kernel_id) return SK_ERR_DUPLICATE;
    if (r->count >= SK_MAX_KERNELS) return SK_ERR_CAPACITY;
    r->entries[r->count++] = *d;
    return SK_OK;
}
const sk_kernel_desc *sk_kernel_find(const sk_kernel_registry *r, uint32_t id) {
    size_t i;
    if (!r) return 0;
    for (i=0u; i<r->count; ++i)
        if (r->entries[i].kernel_id == id) return &r->entries[i];
    return 0;
}
