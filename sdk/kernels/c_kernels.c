// Copyright (C) 2026 AHMADALIPARR
// SPDX-License-Identifier: AGPL-3.0-only
#include "c_kernels.h"

float k_sumf(const float *a, unsigned n) {
    float s = 0.0f;
    for (unsigned i = 0; i < n; i++) s += a[i];
    return s;
}

float k_dotf(const float *a, const float *b, unsigned n) {
    float s = 0.0f;
    for (unsigned i = 0; i < n; i++) s += a[i] * b[i];
    return s;
}

void k_axpyf(float alpha, const float *x, float *y, unsigned n) {
    for (unsigned i = 0; i < n; i++) y[i] += alpha * x[i];
}

unsigned k_popcnt64(unsigned long long v) {
    unsigned c = 0;
    while (v) { v &= v - 1; c++; }
    return c;
}

#if defined(__linux__) && defined(__GNUC__)
__attribute__((naked)) long k_syscall_probe(long nr, long a, long b, long c) {
    __asm__ volatile (
#if defined(__x86_64__)
        "movq %rdi, %rax\n\t"
        "movq %rsi, %rdi\n\t"
        "movq %rdx, %rsi\n\t"
        "movq %rcx, %rdx\n\t"
        "syscall\n\t"
        "ret\n\t"
#elif defined(__aarch64__)
        "mov x8, x0\n\t"
        "mov x0, x1\n\t"
        "mov x1, x2\n\t"
        "mov x2, x3\n\t"
        "svc #0\n\t"
        "ret\n\t"
#endif
    );
}
#else
long k_syscall_probe(long nr, long a, long b, long c) {
    (void)nr; (void)a; (void)b; (void)c;
    return -1; /* non-Linux: privileged path disabled */
}
#endif
