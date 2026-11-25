// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/dispatch.h"
sk_status sk_dispatch(sk_context *c, uint32_t id, const sk_value *args,
                      size_t argc, size_t in_bytes, size_t out_bytes,
                      sk_value *result) {
    const sk_kernel_desc *d;
    sk_contract_claim claim;
    sk_status s;
    if (!c || !result || (argc != 0u && !args) || c->state != 1u)
        return SK_ERR_INVALID_ARGUMENT;
    d=sk_kernel_find(&c->kernels,id);
    if (!d) return SK_ERR_NOT_FOUND;
    claim.capabilities=c->capabilities;
    claim.input_bytes=in_bytes; claim.output_bytes=out_bytes;
    s=sk_contract_check(&d->contract,&claim);
    if (s != SK_OK) return s;
    s=d->function(c,args,argc,result);
    if (s == SK_OK) ++c->dispatch_count;
    return s;
}
