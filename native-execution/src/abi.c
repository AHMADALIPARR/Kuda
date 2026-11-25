// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "sk/abi.h"
#include <limits.h>
sk_abi_descriptor sk_abi_current(void) {
    const uint16_t one = 1u;
    const unsigned char *bytes = (const unsigned char *)&one;
    sk_abi_descriptor d;
    d.magic = SK_ABI_MAGIC;
    d.abi_version = SK_ABI_VERSION;
    d.descriptor_size = (uint32_t)sizeof(sk_abi_descriptor);
    d.pointer_width = (uint8_t)(sizeof(void *) * CHAR_BIT);
    d.little_endian = (uint8_t)(bytes[0] == 1u);
#ifdef __STDC_NO_ATOMICS__
    d.c11_atomics = 0u;
#else
    d.c11_atomics = 1u;
#endif
    d.reserved = 0u;
    return d;
}
int sk_abi_compatible(const sk_abi_descriptor *candidate) {
    sk_abi_descriptor current = sk_abi_current();
    return candidate != 0 &&
        candidate->magic == current.magic &&
        candidate->abi_version == current.abi_version &&
        candidate->descriptor_size == current.descriptor_size &&
        candidate->pointer_width == current.pointer_width &&
        candidate->little_endian == current.little_endian;
}
