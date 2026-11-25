// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* tests/unit/runtime/test_runtime.c */

#include "sk_device.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    sk_device_registry registry = {0};

    assert(sk_discover_devices(&registry) == SK_SUCCESS ||
           registry.count == 0U);

    assert(registry.count <= SK_MAX_DEVICES);

    for (size_t i = 0; i < registry.count; ++i) {
        assert(registry.devices[i].available == 1U);
        assert(registry.devices[i].index == i);
    }

    puts("SnapKitty runtime tests passed.");

    return 0;
}
