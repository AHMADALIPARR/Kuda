# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# mojo/kernels/matrix_ops.mojo

from std.simd import SIMD

def matrix_vector4(
    row0: SIMD[DType.float32, 4],
    row1: SIMD[DType.float32, 4],
    row2: SIMD[DType.float32, 4],
    row3: SIMD[DType.float32, 4],
    vector: SIMD[DType.float32, 4]
) -> SIMD[DType.float32, 4]:

    return SIMD[DType.float32, 4](
        (row0 * vector).reduce_add(),
        (row1 * vector).reduce_add(),
        (row2 * vector).reduce_add(),
        (row3 * vector).reduce_add()
    )
