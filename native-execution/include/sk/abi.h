// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#ifndef SK_ABI_H
#define SK_ABI_H
#include <stddef.h>
#include <stdint.h>
#define SK_ABI_VERSION 1u
#define SK_ABI_MAGIC UINT64_C(0x534B4E4154495645) /* "SKNATIVE" */
typedef struct sk_abi_descriptor {
    uint64_t magic;
    uint32_t abi_version;
    uint32_t descriptor_size;
    uint8_t pointer_width;
    uint8_t little_endian;
    uint8_t c11_atomics;
    uint8_t reserved;
} sk_abi_descriptor;
sk_abi_descriptor sk_abi_current(void);
int sk_abi_compatible(const sk_abi_descriptor *candidate);
#endif
