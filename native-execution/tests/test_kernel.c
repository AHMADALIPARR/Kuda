// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/kernel.h"
static sk_status noop(struct sk_context *c,const sk_value *a,size_t n,sk_value *r) {
    (void)c;(void)a;(void)n;*r=sk_value_unit();return SK_OK;
}
int main(void) {
    sk_kernel_registry r; sk_kernel_desc d={0};
    /* NOTE: do not put calls with side effects inside assert(): Release
       builds define NDEBUG, which compiles assert() away -- including the
       call itself -- so no kernel would ever be registered and the test
       would pass vacuously. Results are checked explicitly instead. */
    sk_kernel_registry_init(&r);
    d.kernel_id=7u; d.version=1u; d.name="noop"; d.function=noop;
    if (sk_kernel_register(&r,&d)!=SK_OK) return 1;
    if (sk_kernel_find(&r,7u)==0) return 1;
    if (sk_kernel_register(&r,&d)!=SK_ERR_DUPLICATE) return 1;
    return 0;
}
