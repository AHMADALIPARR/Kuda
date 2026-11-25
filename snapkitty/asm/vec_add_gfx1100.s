; Copyright (C) 2026 AHMADALIPARR
; SPDX-License-Identifier: AGPL-3.0-only
.text
.globl vec_add
.p2align 8
.type vec_add,@function
vec_add:
    s_load_dwordx2 s[4:5], s[0:1], 0x00 ; out
    s_load_dwordx2 s[6:7], s[0:1], 0x08 ; a
    s_load_dwordx2 s[8:9], s[0:1], 0x10 ; b
    s_load_dword s10, s[0:1], 0x18 ; n (u32)
    s_waitcnt lgkmcnt(0)

    v_mov_b32 v1, v0 ; tid = lane_id()
    v_cmp_lt_u32_e32 vcc, v1, s10 ; tid < n
    s_and_saveexec_b32 s11, vcc
    s_cbranch_execz .L_exit

    v_lshlrev_b32 v4, 2, v1 ; byte offset = tid * 4
    v_mov_b32 v5, 0
    global_load_b32 v2, v[4:5], s[6:7] ; a[tid]
    global_load_b32 v3, v[4:5], s[8:9] ; b[tid]
    s_waitcnt vmcnt(0)

    v_add_f32_e32 v6, v2, v3 ; a[tid] + b[tid]
    global_store_b32 v[4:5], v6, s[4:5] ; out[tid]
.L_exit:
    s_endpgm
