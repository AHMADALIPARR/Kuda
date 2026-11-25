// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* runtime/discovery/nvidia_discovery.c */

#include "sk_types.h"

#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

typedef int CUresult;
typedef int CUdevice;

typedef CUresult (*cuInit_fn)(unsigned int);
typedef CUresult (*cuDeviceGetCount_fn)(int *);
typedef CUresult (*cuDeviceGet_fn)(CUdevice *, int);
typedef CUresult (*cuDeviceGetName_fn)(char *, int, CUdevice);
typedef CUresult (*cuDeviceComputeCapability_fn)(
    int *, int *, CUdevice
);

typedef struct {
    void *library;
    cuInit_fn init;
    cuDeviceGetCount_fn count;
    cuDeviceGet_fn get;
    cuDeviceGetName_fn name;
    cuDeviceComputeCapability_fn capability;
} sk_nv_api;

static sk_nv_api nv;

#define SK_LOAD_NV(symbol, member) \
    do { \
        void *address = dlsym(nv.library, symbol); \
        if (address == NULL || sizeof(address) != sizeof(nv.member))\
            return SK_ERR_DRIVER; \
        memcpy(&nv.member, &address, sizeof(address)); \
    } while (0)

sk_status sk_nvidia_discover(sk_device_registry *registry)
{
    int count = 0;

    if (registry == NULL)
        return SK_ERR_ARGUMENT;

    nv.library = dlopen("libcuda.so.1", RTLD_NOW | RTLD_LOCAL);

    if (nv.library == NULL)
        return SK_ERR_UNSUPPORTED;

    SK_LOAD_NV("cuInit", init);
    SK_LOAD_NV("cuDeviceGetCount", count);
    SK_LOAD_NV("cuDeviceGet", get);
    SK_LOAD_NV("cuDeviceGetName", name);
    SK_LOAD_NV("cuDeviceComputeCapability", capability);

    if (nv.init(0U) != 0)
        return SK_ERR_DRIVER;

    if (nv.count(&count) != 0)
        return SK_ERR_DRIVER;

    for (int i = 0; i < count; ++i) {
        CUdevice handle;
        int major = 0;
        int minor = 0;

        sk_device device = {0};

        if (nv.get(&handle, i) != 0)
            continue;

        if (nv.name(
                device.name,
                (int)sizeof(device.name),
                handle) != 0)
            continue;

        if (nv.capability(&major, &minor, handle) != 0)
            continue;

        device.vendor = SK_VENDOR_NVIDIA;
        device.vendor_id = 0x10DEU;
        device.backend = SK_BACKEND_NVIDIA_DRIVER;
        device.available = 1U;

        (void)snprintf(
            device.architecture,
            sizeof(device.architecture),
            "sm_%d%d",
            major,
            minor
        );

        if (sk_registry_add(registry, &device) != SK_OK)
            break;
    }

    return SK_OK;
}
