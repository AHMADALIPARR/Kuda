// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
/* c_kernels.h - portable C99/C11 compute kernels, uniform C signatures */
#ifndef C_KERNELS_H
#define C_KERNELS_H
#ifdef __cplusplus
extern "C" {
#endif

float k_sumf(const float *a, unsigned n);
float k_dotf(const float *a, const float *b, unsigned n);
void k_axpyf(float alpha, const float *x, float *y, unsigned n);
unsigned k_popcnt64(unsigned long long v);
long k_syscall_probe(long nr, long a, long b, long c); /* raw syscall wrapper (Linux) */

#ifdef __cplusplus
}
#endif
#endif
