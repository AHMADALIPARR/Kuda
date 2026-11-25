// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/dispatch.h"
#include <stdint.h>
static sk_status add(struct sk_context *c,const sk_value *a,size_t n,sk_value *r) {
    int64_t x,y; (void)c;
    if (n!=2u || sk_value_as_i64(a[0],&x)!=SK_OK ||
        sk_value_as_i64(a[1],&y)!=SK_OK) return SK_ERR_TYPE_MISMATCH;
    *r=sk_value_i64(x+y); return SK_OK;
}
int main(void) {
    sk_context c; sk_kernel_desc d={0}; sk_value args[2],out;
    /* NOTE: do not put calls with side effects inside assert(): Release
       builds define NDEBUG, which compiles assert() away -- including the
       call itself -- leaving the context uninitialized. Results are
       checked explicitly instead. */
    if (sk_context_init(&c,1024u,1u)!=SK_OK) return 1;
    d.kernel_id=1u; d.version=1u; d.name="add"; d.function=add;
    d.contract.contract_id=1u; d.contract.version=1u;
    d.contract.required_capabilities=1u; d.contract.max_input_bytes=64u;
    d.contract.max_output_bytes=64u;
    if (sk_kernel_register(&c.kernels,&d)!=SK_OK) return 1;
    args[0]=sk_value_i64(19); args[1]=sk_value_i64(23);
    if (sk_dispatch(&c,1u,args,2u,16u,8u,&out)!=SK_OK) return 1;
    if (out.kind!=SK_VALUE_I64 || out.as.i64!=42) return 1;
    if (c.dispatch_count!=1u) return 1;
    if (sk_dispatch(&c,1u,args,2u,128u,8u,&out)!=SK_ERR_CAPACITY) return 1;
    sk_context_destroy(&c);
    return 0;
}
