// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/abi.h"
int main(void) {
    sk_abi_descriptor d=sk_abi_current();
    /* NOTE: checked explicitly, not via assert(): Release builds define
       NDEBUG, which compiles assert() away and would make this test pass
       vacuously without checking anything. */
    if (!sk_abi_compatible(&d)) return 1;
    d.abi_version++;
    if (sk_abi_compatible(&d)) return 1;
    return 0;
}
