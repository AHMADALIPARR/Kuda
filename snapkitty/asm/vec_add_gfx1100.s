; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
.text
.globl vec_add_kernel
.p2align 2
.type vec_add_kernel,@function

vec_add_kernel:
    v_cmp_lt_u32_e32 vcc_lo, v1, s10
    s_and_saveexec_b32 s11, vcc_lo
    s_cbranch_execz .Lend

    global_load_b32 v2, v[4:5], s[6:7]
    global_load_b32 v3, v[4:5], s[8:9]
    s_waitcnt vmcnt(0)

    v_add_u32 v6, v2, v3
    global_store_b32 v[4:5], v6, s[4:5]

.Lend:
    s_endpgm
.size vec_add_kernel, .-vec_add_kernel
