// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef SK_DISPATCH_H
#define SK_DISPATCH_H
#include <stddef.h>
#include <stdint.h>
#include "context.h"
sk_status sk_dispatch(sk_context *context, uint32_t kernel_id,
                      const sk_value *args, size_t arg_count,
                      size_t input_bytes, size_t output_bytes,
                      sk_value *result);
#endif
