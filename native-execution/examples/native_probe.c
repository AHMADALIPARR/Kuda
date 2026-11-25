// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include <stdio.h>
#include "sk/dispatch.h"
#include "sk/abi.h"
static sk_status probe(struct sk_context *c,const sk_value *a,size_t n,sk_value *r) {
    (void)c; (void)a;
    if (n != 0u) return SK_ERR_INVALID_ARGUMENT;
    *r=sk_value_u64(UINT64_C(0xC011)); return SK_OK;
}
int main(void) {
    sk_context c; sk_kernel_desc d={0}; sk_value result;
    sk_abi_descriptor abi=sk_abi_current();
    if (sk_context_init(&c,4096u,0u)!=SK_OK) return 1;
    d.kernel_id=1u; d.version=1u; d.name="native_probe"; d.function=probe;
    d.contract.max_input_bytes=0u; d.contract.max_output_bytes=sizeof(uint64_t);
    if (sk_kernel_register(&c.kernels,&d)!=SK_OK ||
        sk_dispatch(&c,1u,0,0u,0u,sizeof(uint64_t),&result)!=SK_OK) {
        sk_context_destroy(&c); return 2;
    }
    printf("ABI v%u, pointer width %u, probe=0x%llx\n",
           abi.abi_version,(unsigned)abi.pointer_width,
           (unsigned long long)result.as.u64);
    sk_context_destroy(&c);
    return 0;
}
