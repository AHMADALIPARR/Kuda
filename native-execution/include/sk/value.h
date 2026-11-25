// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef SK_VALUE_H
#define SK_VALUE_H
#include <stdint.h>
#include "status.h"
typedef enum sk_value_kind {
    SK_VALUE_UNIT = 0, SK_VALUE_I64, SK_VALUE_U64, SK_VALUE_F64,
    SK_VALUE_BOOL, SK_VALUE_PTR
} sk_value_kind;
typedef struct sk_value {
    sk_value_kind kind;
    union {
        int64_t i64;
        uint64_t u64;
        double f64;
        int boolean;
        void *ptr;
    } as;
} sk_value;
sk_value sk_value_unit(void);
sk_value sk_value_i64(int64_t value);
sk_value sk_value_u64(uint64_t value);
sk_value sk_value_f64(double value);
sk_value sk_value_bool(int value);
sk_value sk_value_ptr(void *value);
sk_status sk_value_as_i64(sk_value value, int64_t *out);
sk_status sk_value_as_u64(sk_value value, uint64_t *out);
#endif
