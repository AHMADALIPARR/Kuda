// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* runtime/discovery/target_registry.c */

#include "sk_types.h"

#include <string.h>

sk_status sk_registry_add(
    sk_device_registry *registry,
    const sk_device *device
)
{
    if (registry == NULL || device == NULL)
        return SK_ERR_ARGUMENT;

    if (registry->count >= SK_MAX_DEVICES)
        return SK_ERR_DEVICE;

    registry->devices[registry->count] = *device;

    registry->devices[registry->count].index =
        (uint32_t)registry->count;

    registry->count++;

    return SK_OK;
}

void sk_registry_clear(sk_device_registry *registry)
{
    if (registry != NULL)
        memset(registry, 0, sizeof(*registry));
}
