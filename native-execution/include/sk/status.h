// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef SK_STATUS_H
#define SK_STATUS_H
typedef enum sk_status {
    SK_OK = 0,
    SK_ERR_INVALID_ARGUMENT,
    SK_ERR_OUT_OF_MEMORY,
    SK_ERR_OUT_OF_BOUNDS,
    SK_ERR_TYPE_MISMATCH,
    SK_ERR_DUPLICATE,
    SK_ERR_NOT_FOUND,
    SK_ERR_CONTRACT,
    SK_ERR_CAPACITY,
    SK_ERR_INTERNAL
} sk_status;
const char *sk_status_string(sk_status status);
#endif
