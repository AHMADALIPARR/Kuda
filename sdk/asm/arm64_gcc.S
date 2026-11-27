/* Copyright (C) 2026 AHMADALIPARR
SPDX-License-Identifier: AGPL-3.0-only */
/* arm64_gcc.S - AArch64 (GAS) native dispatch stub, Linux AAPCS64.
 * Scope: up to 8 GP (x0-x7) and 8 FP/SIMD (v0-v7) args; integer/pointer
 * return in x0. HFA returns come back in v0-v3 per PCS (pass-through here).
 * Build: cc -c asm/arm64_gcc.S -o build/arm64_call.o
 */
    .text
    .global arm64_call
arm64_call:
    stp x29, x30, [sp, #-96]!
    mov x29, sp

    mov x9, x0 /* target */
    mov x10, x1 /* gp array */
    mov x11, x2 /* fp array */

    /* GP: x0..x7 */
    ldr x0, [x10, #0]
    ldr x1, [x10, #8]
    ldr x2, [x10, #16]
    ldr x3, [x10, #24]
    ldr x4, [x10, #32]
    ldr x5, [x10, #40]
    ldr x6, [x10, #48]
    ldr x7, [x10, #56]

    /* FP: v0..v7 */
    ldr d0, [x11, #0]
    ldr d1, [x11, #8]
    ldr d2, [x11, #16]
    ldr d3, [x11, #24]
    ldr d4, [x11, #32]
    ldr d5, [x11, #40]
    ldr d6, [x11, #48]
    ldr d7, [x11, #56]

    blr x9

    ldp x29, x30, [sp], #96
    ret
