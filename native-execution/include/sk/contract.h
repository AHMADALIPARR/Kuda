// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef SK_CONTRACT_H
#define SK_CONTRACT_H
#include <stddef.h>
#include <stdint.h>
#include "status.h"
typedef struct sk_contract {
    uint32_t contract_id;
    uint32_t version;
    uint64_t required_capabilities;
    uint64_t forbidden_capabilities;
    size_t max_input_bytes;
    size_t max_output_bytes;
} sk_contract;
typedef struct sk_contract_claim {
    uint64_t capabilities;
    size_t input_bytes;
    size_t output_bytes;
} sk_contract_claim;
sk_status sk_contract_check(const sk_contract *contract,
                            const sk_contract_claim *claim);
#endif
