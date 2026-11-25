// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* runtime/device/device.c */

#include "sk_device.h"

extern sk_status sk_nvidia_discover(sk_device_registry *);
extern sk_status sk_amd_discover(sk_device_registry *);

extern void sk_registry_clear(sk_device_registry *);

sk_status sk_discover_devices(sk_device_registry *registry)
{
    sk_status nvidia_status;
    sk_status amd_status;

    if (registry == NULL)
        return SK_ERR_ARGUMENT;

    sk_registry_clear(registry);

    nvidia_status = sk_nvidia_discover(registry);
    amd_status = sk_amd_discover(registry);

    if (nvidia_status != SK_OK && amd_status != SK_OK)
        return SK_ERR_UNSUPPORTED;

    return SK_OK;
}

sk_status sk_initialize_device(
    sk_device_registry *registry,
    uint32_t device_index
)
{
    sk_device *device;

    if (registry == NULL)
        return SK_ERR_ARGUMENT;

    if (device_index >= registry->count)
        return SK_ERR_DEVICE;

    device = &registry->devices[device_index];

    if (!device->available)
        return SK_ERR_DEVICE;

    if (device->initialized)
        return SK_OK;

    device->initialized = 1U;

    return SK_OK;
}

sk_status sk_shutdown_device(
    sk_device_registry *registry,
    uint32_t device_index
)
{
    if (registry == NULL)
        return SK_ERR_ARGUMENT;

    if (device_index >= registry->count)
        return SK_ERR_DEVICE;

    registry->devices[device_index].initialized = 0U;

    return SK_OK;
}
