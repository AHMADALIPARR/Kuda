// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/abi.h"
#include <assert.h>
int main(void) {
    sk_abi_descriptor d=sk_abi_current();
    assert(sk_abi_compatible(&d));
    d.abi_version++;
    assert(!sk_abi_compatible(&d));
    return 0;
}
