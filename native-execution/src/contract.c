// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/contract.h"
sk_status sk_contract_check(const sk_contract *c, const sk_contract_claim *claim) {
    if (!c || !claim) return SK_ERR_INVALID_ARGUMENT;
    if ((claim->capabilities & c->required_capabilities) != c->required_capabilities)
        return SK_ERR_CONTRACT;
    if ((claim->capabilities & c->forbidden_capabilities) != 0u)
        return SK_ERR_CONTRACT;
    if (claim->input_bytes > c->max_input_bytes ||
        claim->output_bytes > c->max_output_bytes)
        return SK_ERR_CAPACITY;
    return SK_OK;
}
