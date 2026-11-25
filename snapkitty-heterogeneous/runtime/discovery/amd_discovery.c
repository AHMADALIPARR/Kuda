// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* runtime/discovery/amd_discovery.c */

#include "sk_device.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

typedef int hip_error_t;

typedef hip_error_t (*hipGetDeviceCount_fn)(int *);

typedef struct {
    void *library;
    hipGetDeviceCount_fn get_device_count;
} sk_amd_api;

static sk_amd_api amd;

sk_status sk_amd_discover(sk_device_registry *registry)
{
    int count = 0;
    void *symbol;

    if (registry == NULL)
        return SK_ERR_ARGUMENT;

    amd.library = dlopen(
        "libamdhip64.so",
        RTLD_NOW | RTLD_LOCAL
    );

    if (amd.library == NULL)
        return SK_ERR_UNSUPPORTED;

    symbol = dlsym(amd.library, "hipGetDeviceCount");

    if (symbol == NULL ||
        sizeof(symbol) != sizeof(amd.get_device_count))
        return SK_ERR_DRIVER;

    memcpy(
        &amd.get_device_count,
        &symbol,
        sizeof(symbol)
    );

    if (amd.get_device_count(&count) != 0)
        return SK_ERR_DRIVER;

    for (int i = 0; i < count; ++i) {
        sk_device device = {0};

        device.vendor = SK_VENDOR_AMD;
        device.vendor_id = 0x1002U;
        device.backend = SK_BACKEND_AMD_HIP;
        device.available = 1U;

        (void)snprintf(
            device.name,
            sizeof(device.name),
            "AMD HIP device %d",
            i
        );

        if (sk_registry_add(registry, &device) != SK_OK)
            break;
    }

    return SK_OK;
}
