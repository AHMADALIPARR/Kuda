/* Copyright (C) 2026 AHMADALIPARR
 * SPDX-License-Identifier: AGPL-3.0-only
 */
/* SnapKitty B kernel with explicit width & address-space extensions */
vec_add(__global f32* mut out, __global f32* a, __global f32* b, __reg u32 n) {
    extrn lane_id;
    auto u32 tid;

    tid = lane_id();
    if (tid < n) {
        out[tid] = a[tid] + b[tid];
    }
}
