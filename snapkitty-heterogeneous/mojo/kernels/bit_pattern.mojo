# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# mojo/kernels/bit_pattern.mojo

from std.simd import SIMD

comptime VECTOR_WIDTH = 4

def bit_pattern_match[
    width: Int
](
    values: SIMD[DType.uint32, width],
    pattern: SIMD[DType.uint32, width],
    mask: SIMD[DType.uint32, width]
) -> SIMD[DType.bool, width]:

    return (values & mask) == (pattern & mask)

def bit_pattern_fold[
    width: Int
](
    values: SIMD[DType.uint32, width],
    mask: SIMD[DType.uint32, width]
) -> SIMD[DType.uint32, width]:

    return values & mask

def bit_pattern_xor[
    width: Int
](
    values: SIMD[DType.uint32, width],
    pattern: SIMD[DType.uint32, width]
) -> SIMD[DType.uint32, width]:

    return values ^ pattern
