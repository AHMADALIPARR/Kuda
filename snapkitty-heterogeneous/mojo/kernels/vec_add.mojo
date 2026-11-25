# Copyright (C) 2026 AHMADALIPARR
# SPDX-License-Identifier: AGPL-3.0-only
fn vec_add_kernel[dtype: DType](a: UnsafePointer[Scalar[dtype]], b: UnsafePointer[Scalar[dtype]], out: UnsafePointer[Scalar[dtype]], n: Int):
    let tid = block_idx.x * block_dim.x + thread_idx.x
    if tid < n:
        out[tid] = a[tid] + b[tid]

fn main():
    print("Mojo kernel stub")
