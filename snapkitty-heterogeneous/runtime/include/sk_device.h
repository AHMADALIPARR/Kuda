// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* runtime/include/sk_device.h */

#ifndef SK_DEVICE_H
#define SK_DEVICE_H

#include "sk_types.h"

sk_status sk_discover_devices(sk_device_registry *registry);

/* Device registry (runtime/discovery/target_registry.c): the
 * discovery backends add each device they find through
 * sk_registry_add. Declared here, with the device API, so the
 * discovery sources do not call it without a prototype. */
sk_status sk_registry_add(
    sk_device_registry *registry,
    const sk_device *device
);

void sk_registry_clear(sk_device_registry *registry);

sk_status sk_initialize_device(
    sk_device_registry *registry,
    uint32_t device_index
);

sk_status sk_shutdown_device(
    sk_device_registry *registry,
    uint32_t device_index
);

#endif
