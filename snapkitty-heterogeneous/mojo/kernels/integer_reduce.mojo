# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# mojo/kernels/integer_reduce.mojo

from std.simd import SIMD

def integer_vector_add[
    width: Int
](
    left: SIMD[DType.uint32, width],
    right: SIMD[DType.uint32, width]
) -> SIMD[DType.uint32, width]:

    return left + right

def integer_vector_and[
    width: Int
](
    values: SIMD[DType.uint32, width],
    mask: SIMD[DType.uint32, width]
) -> SIMD[DType.uint32, width]:

    return values & mask
