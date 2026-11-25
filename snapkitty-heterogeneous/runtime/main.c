// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* runtime/main.c */

#include "sk_device.h"

#include <stdio.h>

extern void sk_linux_discover(void);

int main(void)
{
    sk_device_registry registry = {0};

    sk_linux_discover();

    if (sk_discover_devices(&registry) != SK_OK) {
        puts("No compatible GPU runtime detected.");
        return 0;
    }

    printf("GPU devices discovered: %zu\n", registry.count);

    for (size_t i = 0; i < registry.count; ++i) {
        const sk_device *device = &registry.devices[i];

        printf(
            "%u | %s | %s | %s\n",
            device->index,
            device->name,
            device->vendor == SK_VENDOR_NVIDIA
                ? "NVIDIA"
                : "AMD",
            device->architecture
        );
    }

    return 0;
}
