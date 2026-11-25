// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* runtime/include/sk_types.h */

#ifndef SK_TYPES_H
#define SK_TYPES_H

#include <stddef.h>
#include <stdint.h>

#define SK_MAX_DEVICES 32U
#define SK_NAME_LENGTH 128U

typedef enum {
    SK_OK = 0,
    SK_ERR_ARGUMENT = -1,
    SK_ERR_SYSTEM = -2,
    SK_ERR_DRIVER = -3,
    SK_ERR_DEVICE = -4,
    SK_ERR_MEMORY = -5,
    SK_ERR_UNSUPPORTED = -6,
    SK_ERR_STATE = -7
} sk_status;

typedef enum {
    SK_VENDOR_NONE = 0,
    SK_VENDOR_NVIDIA,
    SK_VENDOR_AMD
} sk_vendor;

typedef enum {
    SK_BACKEND_NONE = 0,
    SK_BACKEND_NVIDIA_DRIVER,
    SK_BACKEND_AMD_HIP
} sk_backend;

typedef struct {
    uint32_t index;
    uint32_t vendor_id;
    uint32_t device_id;

    sk_vendor vendor;
    sk_backend backend;

    char name[SK_NAME_LENGTH];
    char architecture[SK_NAME_LENGTH];

    uint64_t global_memory;
    uint64_t shared_memory;

    uint32_t compute_units;
    uint32_t max_threads_per_block;

    uint8_t available;
    uint8_t initialized;
} sk_device;

typedef struct {
    sk_device devices[SK_MAX_DEVICES];
    size_t count;
} sk_device_registry;

typedef struct {
    uint64_t bytes;
    uint32_t alignment;
    uint32_t flags;
    void *host_address;
    void *device_address;
    uint32_t device_index;
} sk_buffer;

typedef struct {
    uint32_t device_index;
    uint32_t grid_x;
    uint32_t grid_y;
    uint32_t grid_z;
    uint32_t block_x;
    uint32_t block_y;
    uint32_t block_z;
} sk_launch_config;

#endif
