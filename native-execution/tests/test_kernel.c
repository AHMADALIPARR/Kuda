// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/kernel.h"
#include <assert.h>
static sk_status noop(struct sk_context *c,const sk_value *a,size_t n,sk_value *r) {
    (void)c;(void)a;(void)n;*r=sk_value_unit();return SK_OK;
}
int main(void) {
    sk_kernel_registry r; sk_kernel_desc d={0};
    sk_kernel_registry_init(&r);
    d.kernel_id=7u; d.version=1u; d.name="noop"; d.function=noop;
    assert(sk_kernel_register(&r,&d)==SK_OK);
    assert(sk_kernel_find(&r,7u)!=0);
    assert(sk_kernel_register(&r,&d)==SK_ERR_DUPLICATE);
    return 0;
}
