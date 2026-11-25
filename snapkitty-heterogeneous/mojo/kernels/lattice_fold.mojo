# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
# mojo/kernels/lattice_fold.mojo

from std.simd import SIMD

def lattice_fold[
    width: Int
](
    north: SIMD[DType.float32, width],
    south: SIMD[DType.float32, width],
    east: SIMD[DType.float32, width],
    west: SIMD[DType.float32, width],
    center: SIMD[DType.float32, width]
) -> SIMD[DType.float32, width]:

    return (
        north +
        south +
        east +
        west +
        center
    ) * SIMD[DType.float32, width](0.2)
