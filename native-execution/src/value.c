// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/value.h"
sk_value sk_value_unit(void) { sk_value v = {0}; v.kind = SK_VALUE_UNIT; return v; }
sk_value sk_value_i64(int64_t x) { sk_value v = {0}; v.kind=SK_VALUE_I64; v.as.i64=x; return v; }
sk_value sk_value_u64(uint64_t x) { sk_value v = {0}; v.kind=SK_VALUE_U64; v.as.u64=x; return v; }
sk_value sk_value_f64(double x) { sk_value v = {0}; v.kind=SK_VALUE_F64; v.as.f64=x; return v; }
sk_value sk_value_bool(int x) { sk_value v = {0}; v.kind=SK_VALUE_BOOL; v.as.boolean=(x != 0); return v; }
sk_value sk_value_ptr(void *x) { sk_value v = {0}; v.kind=SK_VALUE_PTR; v.as.ptr=x; return v; }
sk_status sk_value_as_i64(sk_value v, int64_t *out) {
    if (!out) return SK_ERR_INVALID_ARGUMENT;
    if (v.kind != SK_VALUE_I64) return SK_ERR_TYPE_MISMATCH;
    *out = v.as.i64; return SK_OK;
}
sk_status sk_value_as_u64(sk_value v, uint64_t *out) {
    if (!out) return SK_ERR_INVALID_ARGUMENT;
    if (v.kind != SK_VALUE_U64) return SK_ERR_TYPE_MISMATCH;
    *out = v.as.u64; return SK_OK;
}
