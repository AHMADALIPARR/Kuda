# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# mojo/kernels/memory_transform.mojo

from std.simd import SIMD

def memory_transform[
    width: Int
](
    values: SIMD[DType.uint32, width],
    multiplier: UInt32,
    offset: UInt32
) -> SIMD[DType.uint32, width]:

    return (
        values * SIMD[DType.uint32, width](multiplier)
    ) + SIMD[DType.uint32, width](offset)
